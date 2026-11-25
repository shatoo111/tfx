#include "core/OverlayController.h"
#include "effects/EffectFactory.h"
#include "renderer/EffectItem.h"
#include <QChronoTimer>
#include <QGuiApplication>
#include <QSGRendererInterface>
#include <chrono>
#include <functional>
#include <QDateTime>
#include <QSysInfo>
#ifdef Q_OS_WIN
#include <QWinEventNotifier>
#include <windows.h>
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#endif

namespace te {
class OverlayFrameTimer {
public:
    explicit OverlayFrameTimer(std::function<void()> tick) {
        fallback_.setTimerType(Qt::PreciseTimer);
        fallback_.setSingleShot(true);
        QObject::connect(&fallback_, &QChronoTimer::timeout, &fallback_, tick);
#ifdef Q_OS_WIN
        handle_ = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                         TIMER_MODIFY_STATE | SYNCHRONIZE);
        if (handle_) {
            notifier_.setHandle(handle_);
            QObject::connect(&notifier_, &QWinEventNotifier::activated, &notifier_, [this, tick] {
                if (!armed_) return;
                armed_ = false;
                tick();
            });
        }
#endif
    }
    ~OverlayFrameTimer() {
        stop();
#ifdef Q_OS_WIN
        if (handle_) CloseHandle(handle_);
#endif
    }
    void start(std::chrono::nanoseconds delay) {
#ifdef Q_OS_WIN
        if (handle_) {
            LARGE_INTEGER due{};
            due.QuadPart = -std::max<qint64>(1, (delay.count() + 99) / 100);
            // Keep background overlays on cadence when Windows coarsens multimedia timers.
            if (SetWaitableTimer(handle_, &due, 0, nullptr, nullptr, FALSE)) {
                fallback_.stop();
                armed_ = true;
                notifier_.setEnabled(true);
                return;
            }
            armed_ = false;
            notifier_.setEnabled(false);
        }
#endif
        fallback_.setInterval(delay);
        fallback_.start();
    }
    void stop() {
        fallback_.stop();
#ifdef Q_OS_WIN
        if (handle_) {
            armed_ = false;
            notifier_.setEnabled(false);
            CancelWaitableTimer(handle_);
        }
#endif
    }
    bool isActive() const {
#ifdef Q_OS_WIN
        if (armed_) return true;
#endif
        return fallback_.isActive();
    }
private:
    QChronoTimer fallback_;
#ifdef Q_OS_WIN
    QWinEventNotifier notifier_;
    HANDLE handle_ = nullptr;
    bool armed_ = false;
#endif
};

OverlayController::OverlayController(NativePlatform& platform, WindowTracker& tracker, QObject* parent)
    : QObject(parent), platform_(platform), tracker_(tracker), effect_(createEffect(EffectKind::Snow)),
      timer_(std::make_unique<OverlayFrameTimer>([this] { tick(); })) {
#ifdef Q_OS_MACOS
    native_ = QGuiApplication::platformName() == QStringLiteral("cocoa");
#else
    native_ = QGuiApplication::platformName() == QStringLiteral("windows");
#endif
    retry_.setSingleShot(true);
    connect(&retry_, &QTimer::timeout, this, [this] {
        if (!window_) createLayer();
        synchronize();
    });
    createLayer();
    connect(&tracker_, &WindowTracker::changed, this, &OverlayController::synchronize);
    // Recover even when Telegram's snapshot is unchanged: its layer can be
    // hidden independently, or lose rendering resources during a Space/wake transition.
    connect(&tracker_, &WindowTracker::sampled, this, &OverlayController::ensureRunning);
    configure(config_);
}
OverlayController::~OverlayController() {
    timer_->stop();
    // The window and scene graph must die before the engine referenced by EffectItem.
    retireLayer();
}

void OverlayController::record(const QString& event) {
    if (history_.size() >= 160) history_.removeFirst();
    history_.append(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) + " " + event);
}
QString OverlayController::diagnostics() const {
    return QStringLiteral("TFX %1 | Qt %2 | %3\n%4\n%5")
        .arg(QCoreApplication::applicationVersion(), qVersion(), QSysInfo::prettyProductName(),
             status_, history_.join('\n'));
}
void OverlayController::transition(State state, const QString& reason) {
    if (state_ == state && status_ == reason) return;
    state_ = state;
    status_ = reason;
    record(reason);
    emit statusChanged();
}
void OverlayController::retireLayer() {
    watchPress(0);
    ++generation_;
    timer_->stop();
    healthy_.invalidate();
    pendingFrame_ = false;
    platform_.setRenderingActive(false);
    if (window_) {
        window_->disconnect(this);
        window_->hide();
        window_.reset();
    }
    item_ = nullptr;
}
void OverlayController::createLayer() {
    window_ = std::make_unique<QQuickWindow>();
    window_->setTitle(QStringLiteral("Telegram Effects Overlay"));
    window_->setFlags(Qt::Tool | Qt::FramelessWindowHint
        | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint);
    window_->setColor(Qt::transparent);
    window_->setPersistentGraphics(false); window_->setPersistentSceneGraph(false);
    window_->create();
    if (native_) platform_.configureOverlay(*window_);
    item_ = new EffectItem(*effect_, window_->contentItem());
    const auto resize = [this] {
        item_->setSize(QSizeF(window_->size()));
    };
    connect(window_.get(), &QWindow::widthChanged, this, resize);
    connect(window_.get(), &QWindow::heightChanged, this, resize);
    resize();
    const auto generation = generation_;
    connect(window_.get(), &QQuickWindow::frameSwapped, this, [this, generation] {
        if (generation != generation_ || !timer_->isActive() || !config_.enabled) return;
        pendingFrame_ = false;
        lastFrame_.restart();
        if (!healthy_.isValid()) healthy_.start();
        if (healthy_.elapsed() >= 10000) retries_ = 0;
        transition(State::Running, QStringLiteral("Эффект работает"));
    }, Qt::QueuedConnection);
    connect(window_.get(), &QQuickWindow::sceneGraphInitialized, this, [this, generation] {
        if (generation != generation_ || !window_) return;
        if (window_->rendererInterface()->graphicsApi() == QSGRendererInterface::Software)
            recover(QStringLiteral("Требуется аппаратная графика"));
    }, Qt::QueuedConnection);
    connect(window_.get(), &QQuickWindow::sceneGraphError, this,
        [this, generation](QQuickWindow::SceneGraphError, const QString& message) {
            if (generation == generation_) recover(message);
        }, Qt::QueuedConnection);
}
void OverlayController::recover(const QString& reason) {
    if (!config_.enabled || config_.reducedMotion || !tracker_.snapshot().canOverlay()) return;
    record(QStringLiteral("Ошибка графики: ") + reason);
    retireLayer();
    failure_ = reason;
    if (++retries_ > 3) {
        tracker_.setFastPolling(false);
        transition(State::Failed, QStringLiteral("Слой остановлен: %1. Выключите и включите эффект для повтора.").arg(reason));
        return;
    }
    transition(State::Recovering, QStringLiteral("Восстановление %1/3: %2").arg(retries_).arg(reason));
    retry_.start(250 * retries_);
}
void OverlayController::configure(EffectConfig config) {
    config.normalize();
    if (config.economy) config.fps = 30;
    const bool switched = config.kind != config_.kind;
    const bool toggled = config.enabled != config_.enabled;
    const bool fpsChanged = config.fps != config_.fps;
    if (switched || toggled) {
        record(QStringLiteral("Тема %1, эффект %2").arg(int(config.kind)).arg(config.enabled));
        retry_.stop();
        retireLayer();
        retries_ = 0;
        failure_.clear();
        effect_ = createEffect(config.kind);
        loadScale_ = 1;
        ticks_ = lateTicks_ = 0;
    }
    config_ = config;
    effect_->configure(config);
    effect_->setLoadScale(loadScale_ * (config.economy ? 0.5F : 1.F));
    if (!window_ && failure_.isEmpty()) createLayer();
    if (fpsChanged && timer_->isActive()) restartFrameTimer();
    if (item_) item_->update();
    synchronize();
}
void OverlayController::clearSnow() {
    effect_->clear();
    if (item_) item_->update();
    synchronize();
}
void OverlayController::synchronize() {
    const auto& snapshot = tracker_.snapshot();
    tracker_.setFastPolling(config_.enabled && !config_.reducedMotion && retries_ <= 3);
    if (!config_.enabled || config_.reducedMotion || !snapshot.canOverlay() || !native_) {
        watchPress(0);
        timer_->stop();
        healthy_.invalidate();
        if (window_) window_->hide();
        platform_.setRenderingActive(false);
        effect_->resetCursor();
        if (!config_.enabled) transition(State::Off, QStringLiteral("Эффект выключен"));
        else if (config_.reducedMotion) transition(State::Off, QStringLiteral("Пауза: в системе включено уменьшение движения"));
        else if (!native_) transition(State::Failed, QStringLiteral("Слою нужна нативная графическая сессия"));
        else if (!snapshot.running) transition(State::Missing, QStringLiteral("Ожидание Telegram"));
        else transition(State::Hidden, snapshot.detail);
        return;
    }
    if (!window_) {
        if (retry_.isActive()) transition(State::Recovering, QStringLiteral("Восстановление %1/3: %2").arg(retries_).arg(failure_));
        else if (!failure_.isEmpty())
            transition(State::Failed, QStringLiteral("Слой остановлен: %1. Выключите и включите эффект для повтора.").arg(failure_));
        return;
    }
    if (!snapshot.active || pressTarget_ != snapshot.id) watchPress(0);
    platform_.placeOverlay(*window_, snapshot);
    item_->setSize(QSizeF(window_->size()));
    if (loggedBounds_ != snapshot.bounds || loggedDpr_ != window_->devicePixelRatio()) {
        loggedBounds_ = snapshot.bounds;
        loggedDpr_ = window_->devicePixelRatio();
        record(QStringLiteral("Окно %1,%2 %3×%4 DPR %5").arg(loggedBounds_.x()).arg(loggedBounds_.y())
            .arg(loggedBounds_.width()).arg(loggedBounds_.height()).arg(loggedDpr_));
    }
    if (targetId_ != snapshot.id) { effect_->clear(); effect_->resetCursor(); targetId_ = snapshot.id; }
    if (!window_->isVisible()) {
        effect_->advance(1.F / float(config_.fps), {});
        item_->update();
        window_->show();
        platform_.setRenderingActive(true);
        platform_.configureOverlay(*window_);
        platform_.placeOverlay(*window_, snapshot);
        window_->update();
    }
    if (state_ == State::Hidden && platform_.layerOccluded()) return;
    schedule();
}
void OverlayController::schedule() {
    if (!timer_->isActive()) {
        transition(State::Preparing, QStringLiteral("Подготовка графического слоя"));
        lastFrame_.start();
        pendingFrame_ = false;
        restartFrameTimer();
    }
}
void OverlayController::restartFrameTimer() {
    elapsed_.start();
    lastTickNanos_ = 0;
    nextTickNanos_ = 1'000'000'000LL / config_.fps;
    timer_->start(std::chrono::nanoseconds(nextTickNanos_));
}
void OverlayController::ensureRunning() {
    if (!native_ || !window_ || !config_.enabled || config_.reducedMotion || !tracker_.snapshot().canOverlay()) return;
    if (window_->isVisible() && platform_.layerOccluded()
        && (!timer_->isActive() || lastFrame_.elapsed() > 1800)) {
        watchPress(0);
        timer_->stop();
        healthy_.invalidate();
        transition(State::Hidden, QStringLiteral("Пауза: окно перекрыто"));
    } else if (!window_->isVisible() || !timer_->isActive()) {
        synchronize();
    } else if (lastFrame_.elapsed() > 1800) {
        recover(QStringLiteral("Более 1,8 с без кадров"));
    }
}
void OverlayController::watchPress(quintptr target) {
    if (pressTarget_ == target) return;
    pressTarget_ = target;
    const auto watchId = ++pressWatchId_;
    if (!target) {
        platform_.watchPrimaryPress(0, {});
        return;
    }
    platform_.watchPrimaryPress(target, [this, target, watchId](QPointF screen) {
        // Finish the native input callback before the effect answers.
        QMetaObject::invokeMethod(this, [this, target, watchId, screen] {
            const auto& snapshot = tracker_.snapshot();
            if (pressWatchId_ != watchId || snapshot.id != target || !snapshot.active
                || !snapshot.canOverlay() || !snapshot.bounds.contains(screen) || !timer_->isActive() || !item_) return;
            const auto local = screen - snapshot.bounds.topLeft();
            item_->pressAt({local.x() * item_->width() / snapshot.bounds.width(),
                            local.y() * item_->height() / snapshot.bounds.height()});
        }, Qt::QueuedConnection);
    });
}
void OverlayController::tick() {
    if (!tracker_.snapshot().canOverlay() || !window_) { synchronize(); return; }
    const auto now = elapsed_.nsecsElapsed();
    const float dt = float(double(now - lastTickNanos_) / 1'000'000'000.);
    lastTickNanos_ = now;
    // Preserve the fractional millisecond across Windows timer wakeups.
    // If the GUI stalls, resume at the normal cadence instead of issuing a burst.
    const qint64 period = 1'000'000'000LL / config_.fps;
    nextTickNanos_ += period;
    if (nextTickNanos_ - now < period / 2) nextTickNanos_ = now + period;
    timer_->start(std::chrono::nanoseconds(nextTickNanos_ - now));
    if (pendingFrame_ || dt > (1.65F / float(config_.fps))) ++lateTicks_;
    if (++ticks_ >= config_.fps * 2) {
        if (lateTicks_ > ticks_ / 5) loadScale_ = std::max(0.3F, loadScale_ - 0.15F);
        else if (lateTicks_ < ticks_ / 20) loadScale_ = std::min(1.F, loadScale_ + 0.05F);
        effect_->setLoadScale(loadScale_ * (config_.economy ? 0.5F : 1.F)); ticks_ = lateTicks_ = 0;
    }
    const auto& snapshot = tracker_.snapshot();
    watchPress(snapshot.active && item_->pressable() ? snapshot.id : 0);
    const bool interaction = snapshot.active && config_.interaction;
    const auto point = interaction ? platform_.cursorInOverlay(snapshot, *window_) : QPointF();
    const bool inside = interaction && point.x() >= 0 && point.y() >= 0 && point.x() <= item_->width() && point.y() <= item_->height();
    item_->advance(dt, {{float(point.x()), float(point.y())}, inside});
    pendingFrame_ = true;
    if (!effect_->needsAnimation()) synchronize();
}
}
