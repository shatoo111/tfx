#include "core/OverlayController.h"
#include "effects/SnowEffect.h"
#include "effects/SeasonalEffect.h"
#include "effects/RainEffect.h"
#include "effects/SakuraEffect.h"
#include "effects/RetroEffect.h"
#include "effects/AquariumEffect.h"
#include "effects/BlackHoleEffect.h"
#include "effects/CatEffect.h"
#include "effects/ParadeEffect.h"
#include "renderer/EffectItem.h"
#include <QGuiApplication>
#include <QImage>
#include <QProcess>
#include <QScreen>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QtGui/qtestsupport_gui.h>
#include <QtTest>
#include <optional>

namespace {
class TestPlatform final : public te::NativePlatform {
public:
    te::WindowSnapshot target{1, QRectF(50, 50, 640, 420), true, true, true, {}};
    te::WindowSnapshot sample() override { return target; }
    void configureOverlay(QWindow&) override {}
    void placeOverlay(QWindow& window, const te::WindowSnapshot& snapshot) override {
        window.setGeometry(snapshot.bounds.toRect());
    }
    QPointF cursorInOverlay(const te::WindowSnapshot&, const QWindow&) const override { return {-1, -1}; }
    void watch(std::function<void()>) override {}
    bool autoStartEnabled() const override { return false; }
    bool setAutoStart(bool, QString&) override { return true; }
};
}

class RendererTests final : public QObject {
    Q_OBJECT
private slots:
    void themeFramePacing_data() {
        QTest::addColumn<int>("theme");
        QTest::addColumn<int>("fps");
        QTest::newRow("snow") << int(te::EffectKind::Snow) << 60;
        QTest::newRow("rain-economy") << int(te::EffectKind::Rain) << 30;
        QTest::newRow("rain") << int(te::EffectKind::Rain) << 60;
        QTest::newRow("sakura") << int(te::EffectKind::Sakura) << 60;
        QTest::newRow("leaves") << int(te::EffectKind::Leaves) << 60;
        QTest::newRow("united-states") << int(te::EffectKind::UnitedStates) << 60;
        QTest::newRow("stardust") << int(te::EffectKind::Stardust) << 60;
    }
    void themeFramePacing() {
        if (!qEnvironmentVariableIsSet("TFX_TEST_REAL_TELEGRAM"))
            QSKIP("Requires an exposed Telegram Desktop window.");
        QFETCH(int, theme);
        QFETCH(int, fps);
        auto platform = te::createNativePlatform();
        te::WindowTracker tracker(*platform);
        te::OverlayController controller(*platform, tracker);
        te::EffectConfig config;
        config.kind = static_cast<te::EffectKind>(theme);
        config.fps = fps;
        config.intensity = 2;
        config.rainIntensity = 2;
        config.sakuraIntensity = 2;
        config.ambientIntensity = 2;
        config.flightPause = 2;
        config.flightLimit = te::maxFlights;
        config.branchSide = 2;
        config.wind = 0.28F;
        config.grassHeight = 57;
        config.lightning = true;
        config.flashInterval = 5;
        controller.configure(config);
        tracker.start();
        QTRY_COMPARE_WITH_TIMEOUT(controller.state(), te::OverlayController::State::Running, 5000);
        QQuickWindow* overlay = nullptr;
        for (auto* window : QGuiApplication::allWindows())
            if (window->title() == "Telegram Effects Overlay") overlay = qobject_cast<QQuickWindow*>(window);
        QVERIFY(overlay && overlay->isExposed());
        QElapsedTimer elapsed;
        QEventLoop sample;
        std::vector<qint64> frameTimes;
        const auto markFrame = [&] { frameTimes.push_back(elapsed.nsecsElapsed()); };
        connect(overlay, &QQuickWindow::frameSwapped, &sample, markFrame, Qt::QueuedConnection);
        QTimer::singleShot(5000, Qt::PreciseTimer, &sample, &QEventLoop::quit);
        elapsed.start();
        sample.exec();
        const double actual = double(frameTimes.size()) * 1e9 / double(elapsed.nsecsElapsed());
        QVERIFY(frameTimes.size() >= 2);
        std::vector<qint64> intervals;
        for (std::size_t i = 1; i < frameTimes.size(); ++i)
            intervals.push_back(frameTimes[i] - frameTimes[i - 1]);
        std::ranges::sort(intervals);
        const auto p95 = intervals[(intervals.size() - 1) * 95 / 100];
        qInfo("%dx%d, DPR %.2f: %.1f fps, p95 %.1f ms (target %d, screen %.0f Hz).",
              overlay->width(), overlay->height(), overlay->devicePixelRatio(), actual,
              double(p95) / 1e6, fps, overlay->screen()->refreshRate());
        QVERIFY(actual >= std::min(double(fps), overlay->screen()->refreshRate()) * 0.85);
        QVERIFY(actual <= fps * 1.05);
    }
    void realTelegram() {
        if (!qEnvironmentVariableIsSet("TFX_TEST_REAL_TELEGRAM"))
            QSKIP("Requires an official Telegram Desktop window");
        auto platform = te::createNativePlatform();
        te::WindowTracker tracker(*platform);
        te::OverlayController controller(*platform, tracker);
        tracker.start();
        const QString launch = qEnvironmentVariable("TFX_TEST_LAUNCH_TELEGRAM");
        if (!launch.isEmpty()) {
            QCOMPARE(controller.state(), te::OverlayController::State::Missing);
            QProcess client;
            client.setProgram(launch);
            client.setArguments({QStringLiteral("-noupdate")});
            client.setStandardInputFile(QProcess::nullDevice());
            client.setStandardOutputFile(QProcess::nullDevice());
            client.setStandardErrorFile(QProcess::nullDevice());
            QVERIFY(client.startDetached());
        }
        QTRY_VERIFY_WITH_TIMEOUT(tracker.snapshot().canOverlay(), 45000);
#ifdef Q_OS_WIN
        if (qEnvironmentVariableIsSet("TFX_TEST_WINDOW_CHANGES")) {
            const auto hwnd = reinterpret_cast<HWND>(tracker.snapshot().id);
            struct Restore {
                HWND hwnd;
                WINDOWPLACEMENT placement{sizeof(WINDOWPLACEMENT)};
                ~Restore() { SetWindowPlacement(hwnd, &placement); }
            } restore{hwnd};
            QVERIFY(GetWindowPlacement(hwnd, &restore.placement));
            ShowWindow(hwnd, SW_RESTORE);
            const auto start = tracker.snapshot().bounds;
            QVERIFY(SetWindowPos(hwnd, nullptr, int(start.x())+32, int(start.y())+24, 820, 600,
                                 SWP_NOACTIVATE | SWP_NOZORDER));
            const auto aligned = [&] {
                RECT target{};
                if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &target, sizeof(target)))) return false;
                for (auto* w : QGuiApplication::allWindows()) {
                    if (w->title() != "Telegram Effects Overlay" || !w->isVisible()) continue;
                    const auto layer = reinterpret_cast<HWND>(w->winId());
                    RECT rect{};
                    GetWindowRect(layer, &rect);
                    return EqualRect(&rect, &target) && GetWindow(hwnd, GW_HWNDPREV) == layer;
                }
                return false;
            };
            QTRY_VERIFY_WITH_TIMEOUT(aligned(), 3000);
            ShowWindow(hwnd, SW_MAXIMIZE);
            QTRY_VERIFY_WITH_TIMEOUT(aligned(), 3000);
            ShowWindow(hwnd, SW_MINIMIZE);
            QTRY_COMPARE(controller.state(), te::OverlayController::State::Hidden);
            ShowWindow(hwnd, SW_RESTORE);
            QTRY_COMPARE(controller.state(), te::OverlayController::State::Running);
            QTRY_VERIFY_WITH_TIMEOUT(aligned(), 3000);
        }
#endif
        te::EffectConfig config;
        for (auto kind : {te::EffectKind::Snow, te::EffectKind::Rain, te::EffectKind::Sakura, te::EffectKind::Leaves, te::EffectKind::UnitedStates, te::EffectKind::Stardust,
                          te::EffectKind::Retro, te::EffectKind::BlackHole, te::EffectKind::Cat, te::EffectKind::Aquarium}) {
            config.kind = kind;
            controller.configure(config);
            QTRY_COMPARE_WITH_TIMEOUT(controller.state(), te::OverlayController::State::Running, 5000);
            QQuickWindow* overlay = nullptr;
            for (auto* window : QGuiApplication::allWindows())
                if (window->title() == "Telegram Effects Overlay") overlay = qobject_cast<QQuickWindow*>(window);
            QVERIFY(overlay);
            const auto target = tracker.snapshot().bounds.size();
#ifdef Q_OS_WIN
            const auto scale = overlay->devicePixelRatio();
#else
            const auto scale = 1.;
#endif
            QTRY_VERIFY(qAbs(overlay->width()*scale-target.width()) <= 2 &&
                        qAbs(overlay->height()*scale-target.height()) <= 2);
            QSignalSpy frames(overlay, &QQuickWindow::frameSwapped);
            QTRY_VERIFY_WITH_TIMEOUT(frames.size() >= 5, 5000);
            // Read only our own alpha surface, never the desktop or Telegram.
            const auto image = overlay->grabWindow().convertToFormat(QImage::Format_ARGB32);
            QVERIFY(image.hasAlphaChannel() && !image.isNull());
            int ink = 0, clear = 0;
            for (int y = 0; y < image.height(); ++y) {
                const auto* row = reinterpret_cast<const QRgb*>(image.constScanLine(y));
                for (int x = 0; x < image.width(); ++x) {
                    if (qAlpha(row[x]) > 8) ++ink;
                    if (!qAlpha(row[x])) ++clear;
                }
            }
            QVERIFY(ink > 20 && clear > 20);
        }
        if (qEnvironmentVariableIsSet("TFX_TEST_PROFILE")) {
            config.kind = te::EffectKind::Snow;
            config.economy = qEnvironmentVariable("TFX_TEST_PROFILE") == "economy";
            controller.configure(config);
            QTRY_COMPARE(controller.state(), te::OverlayController::State::Running);
            qInfo("Profile window begins");
            QTest::qWait(25000);
            QCOMPARE(controller.state(), te::OverlayController::State::Running);
        }
        config.enabled = false;
        controller.configure(config);
        QCOMPARE(controller.state(), te::OverlayController::State::Off);
    }
    void startsFromSavedTheme_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<bool>("lateTelegram");
        for(int kind=0;kind<3;++kind) for(bool late:{false,true}) {
            const auto label=QByteArray::number(kind)+(late ? "-telegram-later":"-already-open");
            QTest::newRow(label.constData()) << kind << late;
        }
    }
    void startsFromSavedTheme() {
        if (QGuiApplication::platformName() == "offscreen") QSKIP("Requires native desktop.");
        QFETCH(int, kind);
        QFETCH(bool, lateTelegram);
        TestPlatform platform;
        platform.target.active = false;
        const auto ready=platform.target;
        if(lateTelegram) platform.target={};
        te::WindowTracker tracker(platform);
        te::OverlayController controller(platform,tracker);
        te::EffectConfig config; config.kind=static_cast<te::EffectKind>(kind);
        controller.configure(config); // Saved theme, before any Telegram geometry exists.
        QQuickWindow* overlay=nullptr;
        for(auto* window:QGuiApplication::allWindows())
            if(window->title()=="Telegram Effects Overlay") overlay=qobject_cast<QQuickWindow*>(window);
        QVERIFY(overlay);
        QSignalSpy frames(overlay,&QQuickWindow::frameSwapped);
        tracker.start();
        if(lateTelegram) {
            QVERIFY(!overlay->isVisible());
            platform.target=ready; // Telegram starts; no notification is delivered.
        }
        QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible() && !frames.isEmpty(),1000);
        QCOMPARE(overlay->size(),QSize(640,420));
        for(int cycle=0;cycle<3;++cycle) {
            platform.target.visible=false;tracker.refresh();
            QVERIFY(!overlay->isVisible());
            frames.clear();platform.target.visible=true; // No native event: polling must resume.
            QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible() && !frames.isEmpty(),500);
        }
        frames.clear();
        overlay->hide(); // Native/Qt visibility can change without a Telegram snapshot change.
        QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible() && !frames.isEmpty(),500);
        config.enabled=false; controller.configure(config);
        controller.clearSnow();
        platform.target.visible=false; tracker.refresh();
        QCOMPARE(controller.state(), te::OverlayController::State::Off);
    }
    void startsSakuraAndKeepsAnimatingWithoutFocus() {
#ifdef Q_OS_MACOS
        if (QGuiApplication::platformName() != "cocoa")
#else
        if (QGuiApplication::platformName() != "windows")
#endif
            QSKIP("Requires a native desktop session.");
        TestPlatform platform;
        te::WindowTracker tracker(platform);
        te::OverlayController controller(platform, tracker);
        te::EffectConfig config;
        config.kind = te::EffectKind::Sakura;
        controller.configure(config);
        QQuickWindow* overlay = nullptr;
        for (auto* window : QGuiApplication::allWindows()) {
            if (window->title() == "Telegram Effects Overlay")
                overlay = qobject_cast<QQuickWindow*>(window);
        }
        QVERIFY(overlay);

        tracker.start();
        QTRY_VERIFY(overlay->isVisible());
        QTRY_VERIFY(controller.state() == te::OverlayController::State::Running);
        if (overlay->rendererInterface()->graphicsApi() == QSGRendererInterface::Software)
            QSKIP("Requires the default accelerated scene graph.");
        for (auto kind : {te::EffectKind::Rain, te::EffectKind::Sakura, te::EffectKind::Snow}) {
            platform.target.active = false;
            tracker.refresh();
            QVERIFY(overlay->isVisible());
            config.kind = kind;
            controller.configure(config);
            for (auto* w : QGuiApplication::allWindows())
                if (w->title() == "Telegram Effects Overlay") overlay = qobject_cast<QQuickWindow*>(w);
            QSignalSpy frames(overlay, &QQuickWindow::frameSwapped);
            QVERIFY(overlay->isVisible());
            frames.clear();
            QTRY_VERIFY(!frames.isEmpty());
            platform.target.visible = false;
            tracker.refresh();
            QVERIFY(!overlay->isVisible());
            frames.clear();
            // The window becomes visible again without a native notification.
            platform.target.visible = true;
            QTRY_VERIFY_WITH_TIMEOUT(overlay->isVisible() && !frames.isEmpty(), 500);
        }
    }
    void rapidSwitchAndCancelRecovery() {
        TestPlatform platform;
        te::WindowTracker tracker(platform);
        te::OverlayController controller(platform, tracker);
        tracker.start();
        te::EffectConfig config;
        for (auto kind : {te::EffectKind::Snow, te::EffectKind::Rain, te::EffectKind::Sakura}) {
            config.kind = kind;
            controller.configure(config);
        }
        config.reducedMotion = true;
        controller.configure(config);
        QTRY_COMPARE(controller.state(), te::OverlayController::State::Off);
        config.reducedMotion = false;
        config.enabled = false;
        controller.configure(config);
        QTest::qWait(150);
        QCOMPARE(controller.state(), te::OverlayController::State::Off);
        config.enabled = true;
        controller.configure(config);
        QTRY_COMPARE(controller.state(), te::OverlayController::State::Running);
        for (int attempt = 0; attempt < 4; ++attempt) {
            QQuickWindow* current = nullptr;
            for (auto* window : QGuiApplication::allWindows())
                if (window->title() == "Telegram Effects Overlay") current = qobject_cast<QQuickWindow*>(window);
            QVERIFY(current);
            current->sceneGraphError(QQuickWindow::ContextNotAvailable, QStringLiteral("test error"));
            if (attempt < 3) {
                QTRY_COMPARE(controller.state(), te::OverlayController::State::Recovering);
                QTRY_COMPARE(controller.state(), te::OverlayController::State::Running);
            } else QTRY_COMPARE(controller.state(), te::OverlayController::State::Failed);
        }
        QSignalSpy polls(&tracker, &te::WindowTracker::sampled);
        QTest::qWait(160);
        QCOMPARE(polls.size(), 0);
    }
    void separateFlightPaths() {
        te::EffectConfig config;
        config.kind = te::EffectKind::Stardust;
        config.flightPause = 2;
        config.flightLimit = te::maxFlights;
        te::SeasonalEffect effect(config.kind,42);
        effect.configure(config);
        bool sawThree = false;
        int previousCount = 0, quietFrames = 0;
        for (int step = 0; step < 4000; ++step) {
            const QSize size = step % 120 < 60 ? QSize(240,720) : QSize(900,140);
            effect.resize(size.width(),size.height());
            effect.advance(.05F,{});
            int count = 0;
            std::array<float,te::maxFlights> low, high;
            low.fill(float(size.height())); high.fill(-1);
            for (const auto& p : effect.frame().illustrations) {
                if (p.art == te::Artwork::Tail) ++count;
                // Bound the entire rotated sprite.
                for (float u : {0.F,1.F}) for (float v : {0.F,1.F}) {
                    const float y = p.y + (u-p.pivotX)*p.width*std::sin(p.angle)
                                        + (v-p.pivotY)*p.height*std::cos(p.angle);
                    low.at(p.flightSlot) = std::min(low.at(p.flightSlot), y);
                    high.at(p.flightSlot) = std::max(high.at(p.flightSlot), y);
                }
            }
            QVERIFY(count <= config.flightLimit);
            for (int i=0; i<te::maxFlights; ++i)
                for (int j=i+1; j<te::maxFlights; ++j)
                    QVERIFY(high[i] < low[j]);
            if (count && !previousCount && quietFrames)
                QVERIFY(std::abs(quietFrames*.05F-config.flightPause) < .11F);
            if (count) quietFrames = 0;
            else if (previousCount || quietFrames) ++quietFrames;
            if (count == 3 && !sawThree) {
                sawThree = true;
                config.flightLimit = 1;
                config.flightPause = 5;
                effect.configure(config);
            }
            previousCount = count;
        }
        QVERIFY(sawThree);
    }
    void paradeAnswersPresses() {
        if (QGuiApplication::platformName()=="offscreen") QSKIP("Requires accelerated desktop.");
        te::ParadeEffect effect(42);
        te::EffectConfig config;
        config.kind = te::EffectKind::UnitedStates;
        config.flightLimit = 3;
        config.flightPause = 2;
        effect.configure(config);
        QQuickWindow window;
        window.resize(680,410);
        window.setColor(QColor("#26323c"));
        auto* item = new te::EffectItem(effect,window.contentItem());
        item->setSize(window.size());
        item->advance(.016F,{}); // The overlay shows its first frame with the window.
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const auto onStage = [&]() -> std::optional<QPointF> {
            for (std::size_t i = 0; i < effect.guestCount(); ++i) {
                const auto p = effect.guestPoint(i);
                if (p.x > 120 && p.x < 540) return QPointF(p.x,p.y);
            }
            return {};
        };
        for (int i=0; i<4000 && !onStage(); ++i) effect.advance(.05F,{});
        QVERIFY(onStage() && item->pressable());
        item->update();
        QTest::qWait(80);
        const auto calm = window.grabWindow();
        // Everything stays in the strip above the bottom edge.
        const auto background = calm.pixelColor(calm.width()/2,2);
        const int strip = calm.height() - int((te::edgeStrip + 2) * calm.height() / window.height());
        for (int y = 0; y < strip; y += 3)
            for (int x = 0; x < calm.width(); x += 5) QCOMPARE(calm.pixelColor(x,y),background);
        QVERIFY(!item->pressAt({340,40}));
        const auto target = *onStage();
        QVERIFY(item->pressAt(target));
        QVERIFY(!item->pressAt(target)); // One salute at a time.
        QVERIFY(effect.rocketCount() >= 3 && effect.confettiCount() >= 9);
        for (int i=0; i<12; ++i) item->advance(.05F,{});
        QTest::qWait(80);
        const auto salute = window.grabWindow();
        const auto folder = qEnvironmentVariable("TFX_THEME_FRAMES");
        if (!folder.isEmpty()) QVERIFY(calm.save(folder+"/parade.png") && salute.save(folder+"/parade-salute.png"));
        QVERIFY(salute != calm);
        for (int y = 0; y < strip; y += 3)
            for (int x = 0; x < salute.width(); x += 5) QCOMPARE(salute.pixelColor(x,y),background);
        config.interaction = false;
        effect.configure(config);
        QVERIFY(!item->pressable());
    }
    void illustratedThemes_data() {
        QTest::addColumn<int>("theme");
        QTest::addColumn<int>("moment");
        QTest::newRow("autumn") << int(te::EffectKind::Leaves) << 0;
        QTest::newRow("comet") << int(te::EffectKind::Stardust) << 1;
        QTest::newRow("three-comets") << int(te::EffectKind::Stardust) << 5;
        QTest::newRow("pair") << int(te::EffectKind::Stardust) << 4;
        QTest::newRow("star") << int(te::EffectKind::Stardust) << 2;
    }
    void illustratedThemes() {
        if (QGuiApplication::platformName()=="offscreen") QSKIP("Requires accelerated desktop.");
        QFETCH(int,theme);
        QFETCH(int,moment);
        te::EffectConfig config;
        config.kind = static_cast<te::EffectKind>(theme);
        config.flightLimit = moment == 5 ? 3 : 2;
        te::SeasonalEffect effect(config.kind,42);
        effect.configure(config);
        QQuickWindow window;
        window.resize(680,410);
        window.setColor(QColor("#26323c"));
        auto* item = new te::EffectItem(effect,window.contentItem());
        item->setSize(window.size());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const auto ready = [&] {
            if (moment == 4 || moment == 5) {
                const auto& parts = effect.frame().illustrations;
                return std::count_if(parts.begin(),parts.end(),[](const auto& p) {
                    return p.art==te::Artwork::Tail && p.width>50 && p.opacity>.5F;
                }) == (moment == 4 ? 2 : 3);
            }
            for (const auto& p : effect.frame().illustrations) {
                if (moment==0 && p.art==te::Artwork::Bough) return true;
                if (moment==1 && p.art==te::Artwork::Tail && p.width>50 && p.opacity>.5) return true;
                if (moment==2 && p.art==te::Artwork::Rays && p.height>18) return true;
            }
            return false;
        };
        for (int i=0;i<6000 && !ready();++i) effect.advance(.085F,{});
        QVERIFY(ready());
        item->update();
        QTest::qWait(100);
        auto first = window.grabWindow();
        QVERIFY(!first.isNull());
        const auto folder = qEnvironmentVariable("TFX_THEME_FRAMES");
        const auto name = QString::fromLatin1(QTest::currentDataTag());
        if (!folder.isEmpty()) QVERIFY(first.save(folder+"/"+name+".png"));
        for (int i=0;i<17;++i) effect.advance(1.F/60,{});
        item->update();
        QTest::qWait(100);
        auto second = window.grabWindow();
        QVERIFY(first!=second);
        if (!folder.isEmpty()) QVERIFY(second.save(folder+"/"+name+"-later.png"));
        if (qEnvironmentVariableIsSet("TFX_THEME_PROFILE")) {
            QEventLoop loop;
            QElapsedTimer elapsed;
            int frames = 0;
            elapsed.start();
            auto connection = connect(&window,&QQuickWindow::frameSwapped,&loop,[&] {
                ++frames;
                effect.advance(1.F/60,{});
                item->update();
            }, Qt::QueuedConnection);
            QTimer::singleShot(2500,&loop,&QEventLoop::quit);
            item->update();
            loop.exec();
            disconnect(connection);
            qInfo("Rendered %.1f frames/s",1000.*frames/elapsed.elapsed());
        }
        effect.clear();
        QVERIFY(effect.frame().illustrations.empty());
        QVERIFY(effect.frame().ambient.empty());
    }
    void sakuraCorners_data() {
        QTest::addColumn<QSize>("viewport");
        QTest::newRow("narrow") << QSize(400, 300);
        QTest::newRow("wide") << QSize(1120, 600);
    }
    void sakuraCorners() {
        if (QGuiApplication::platformName() == "offscreen") QSKIP("Requires accelerated desktop.");
        QFETCH(QSize, viewport);
        te::SakuraEffect sakura(57);
        te::EffectConfig config;
        config.kind = te::EffectKind::Sakura;
        config.branchWidth = 160;
        sakura.configure(config);
        QQuickWindow window;
        window.setColor(Qt::transparent);
        window.resize(viewport);
        auto* item = new te::EffectItem(sakura, window.contentItem());
        item->setSize(QSizeF(viewport));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (window.rendererInterface()->graphicsApi() == QSGRendererInterface::Software) QSKIP("Requires GPU.");
        const auto capture = window.grabWindow();
        QVERIFY(!capture.isNull() && capture.hasAlphaChannel());
        const auto image = capture.convertToFormat(QImage::Format_ARGB32);
        int ink[2]{};
        for (int y = 0; y < image.height(); ++y) {
            const auto* pixels = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                if (qAlpha(pixels[x]) < 12) continue;
                const bool left = x < image.width() / 4 + 2;
                const bool right = x >= image.width() * 3 / 4 - 2;
                QVERIFY2((left || right) && y <= image.height() / 3 + 2, "Branch artwork must stay within its top corner.");
                ++ink[right];
            }
        }
        QVERIFY(ink[0] > ink[1] && ink[1] > 100);
        const auto path = qEnvironmentVariable("TE_SAKURA_CORNERS");
        if (!path.isEmpty()) QVERIFY(image.save(path + QLatin1Char('-') + QLatin1String(QTest::currentDataTag()) + ".png"));
    }
    void rendersVectorScenes_data() {
        QTest::addColumn<int>("kind");
        QTest::addColumn<int>("moment");
        QTest::newRow("snow-new-year") << int(te::EffectKind::Snow) << 0;
        QTest::newRow("snow-plain") << int(te::EffectKind::Snow) << 1;
        QTest::newRow("snow-deep") << int(te::EffectKind::Snow) << 2;
        QTest::newRow("hole-calm") << int(te::EffectKind::BlackHole) << 0;
        QTest::newRow("hole-traveller") << int(te::EffectKind::BlackHole) << 1;
        QTest::newRow("hole-charged") << int(te::EffectKind::BlackHole) << 2;
        QTest::newRow("hole-burst") << int(te::EffectKind::BlackHole) << 3;
        QTest::newRow("cat-sit") << int(te::EffectKind::Cat) << 0;
        QTest::newRow("cat-walk") << int(te::EffectKind::Cat) << 1;
        QTest::newRow("cat-chase") << int(te::EffectKind::Cat) << 2;
        QTest::newRow("cat-sleep") << int(te::EffectKind::Cat) << 3;
        QTest::newRow("cat-purr") << int(te::EffectKind::Cat) << 4;
        QTest::newRow("cat-swat") << int(te::EffectKind::Cat) << 5;
        QTest::newRow("cat-black") << int(te::EffectKind::Cat) << 6;
        QTest::newRow("cat-grey") << int(te::EffectKind::Cat) << 7;
        QTest::newRow("cat-white") << int(te::EffectKind::Cat) << 8;
        QTest::newRow("aquarium") << int(te::EffectKind::Aquarium) << 1;
        QTest::newRow("aquarium-startled") << int(te::EffectKind::Aquarium) << 2;
    }
    void rendersVectorScenes() {
        if (QGuiApplication::platformName() == "offscreen") QSKIP("Requires accelerated desktop.");
        QFETCH(int, kind);
        QFETCH(int, moment);
        const bool lesser = qEnvironmentVariableIsSet("TE_SCENE_SMALL");
        const QSize size = lesser ? QSize(560, 360) : QSize(1000, 640);
        te::EffectConfig config;
        config.kind = te::EffectKind(kind);
        config.ambientIntensity = 1;
        config.flightPause = 2;
        config.festive = moment != 1;
        if (moment == 2) config.maxDepth = 70;
        config.catCoat = moment >= 6 ? moment - 5 : 0;
        std::unique_ptr<te::IEffect> effect;
        const auto step = [&](int frames, te::CursorSample cursor = {}) { for (int i = 0; i < frames; ++i) effect->advance(1.F / 60, cursor); };
        const auto start = [&] { effect->configure(config); effect->resize(float(size.width()), float(size.height())); };
        if (config.kind == te::EffectKind::Snow) {
            effect = std::make_unique<te::SnowEffect>(12); start(); for (int i = 0; i < (moment == 2 ? 9000 : 1100); ++i) effect->advance(.1F, {});
        } else if (config.kind == te::EffectKind::BlackHole) {
            auto hole = std::make_unique<te::BlackHoleEffect>(12);
            auto* raw = hole.get(); effect = std::move(hole); start(); step(60);
            if (moment == 1) { for (int i = 0; i < 900 && !raw->travelling(); ++i) step(1); step(480); }
            if (moment >= 2) for (int i = 0; i < 95; ++i) step(1, {raw->center(), true});
            if (moment == 3) step(8);
            QCOMPARE(raw->erupting(), moment == 3);
        } else if (config.kind == te::EffectKind::Cat) {
            auto cat = std::make_unique<te::CatEffect>(12);
            auto* raw = cat.get(); effect = std::move(cat); start(); step(40);
            const te::Vec2 at{raw->x(), raw->ground() - 16};
            if (moment == 1) { raw->setMode(te::CatEffect::Mode::Walk); step(24); QCOMPARE(raw->mode(), te::CatEffect::Mode::Walk); }
            if (moment == 2) {
                const auto ball = raw->ball(0);
                for (int i = 0; i < 8; ++i) step(1, {{ball.x - 30 + 8 * float(i), ball.y}, true});
                step(30);
                QCOMPARE(raw->mode(), te::CatEffect::Mode::Chase);
            }
            if (moment == 3) { raw->setMode(te::CatEffect::Mode::Sleep); step(120); }
            if (moment == 4) { for (int i = 0; i < 90; ++i) step(1, {{at.x + 8 * std::sin(float(i) * .4F), at.y}, true}); QVERIFY(raw->heartCount() > 1); }
            if (moment == 5) { for (int i = 0; i < 60 && !raw->swatting(); ++i) step(1, {{at.x - 28 + float(i % 3) * 2, at.y - 6}, true}); step(8, {{at.x - 28, at.y - 6}, true}); QVERIFY(raw->swatting()); }
        } else {
            auto tank = std::make_unique<te::AquariumEffect>(12);
            auto* raw = tank.get(); effect = std::move(tank); start(); step(300);
            if (moment == 2) {
                const auto chest = raw->chestPoint();
                for (int i = 0; i < 60; ++i) step(1, {{chest.x, chest.y - 4}, true});
                for (int i = 0; i < 24; ++i) step(1, {raw->pufferPoint(), true});
                QVERIFY(raw->puffed() > .5F);
                QVERIFY(raw->chestOpen() > .2F);
            }
        }
        QQuickWindow window; window.setTitle("TFX — vector scenes"); window.resize(size);
        auto* item = new te::EffectItem(*effect, window.contentItem()); item->setSize(QSizeF(size));
        window.setColor(QColor("#17212b"));
        window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (window.rendererInterface()->graphicsApi() == QSGRendererInterface::Software) QSKIP("Requires GPU.");
        const auto folder = qEnvironmentVariable("TE_SCENE_FRAMES");
        for (const auto background : {QColor("#17212b"), QColor("#e6ebee")}) {
            window.setColor(background); item->update(); QTest::qWait(90);
            const auto image = window.grabWindow();
            QVERIFY(!image.isNull());
            int ink = 0, clear = 0;
            for (int y = 0; y < image.height(); y += 3) for (int x = 0; x < image.width(); x += 3) {
                const auto p = image.pixelColor(x, y);
                const int d = std::abs(p.red() - background.red()) + std::abs(p.green() - background.green()) + std::abs(p.blue() - background.blue());
                if (d > 90) ++ink;
                if (d < 60) ++clear;
            }
            QVERIFY2(ink > 60, "The scene must be visible on light and dark chats.");
            QVERIFY2(clear > ink, "The scene must leave the chat readable.");
            if (!folder.isEmpty())
                QVERIFY(image.save(folder + "/" + QString::fromLatin1(QTest::currentDataTag()) + (background.lightness() > 128 ? "-light.png" : ".png")));
        }
        window.hide();
    }
    void rendersRetroHeroes() {
        if (QGuiApplication::platformName() == "offscreen") QSKIP("Requires accelerated desktop.");
        // Exact palette colours survive only with nearest-neighbour sampling.
        const std::array<QRgb,3> signature{0xff2f7de1, 0xff3a6fd8, 0xff2aa3e8};
        for (int hero = 0; hero < 3; ++hero) {
            te::RetroEffect effect(21);
            te::EffectConfig config; config.kind = te::EffectKind::Retro; config.retroHero = hero; config.flightPause = 2;
            effect.configure(config);
            QQuickWindow window; window.setTitle("TFX — 8-bit renderer test"); window.resize(760, 420);
            window.setColor(QColor("#17212b"));
            auto* item = new te::EffectItem(effect, window.contentItem()); item->setSize({760, 420});
            effect.resize(760, 420);
            for (int i = 0; i < 2000; ++i) {
                effect.advance(1.F / 60, {});
                if (effect.stage() == te::RetroEffect::Stage::Running && effect.heroFeet().x < effect.homeX() - 120
                    && effect.heroFeet().y >= effect.ground() - .01F) break;
            }
            QCOMPARE(effect.stage(), te::RetroEffect::Stage::Running);
            window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
            if (window.rendererInterface()->graphicsApi() == QSGRendererInterface::Software) QSKIP("Requires GPU.");
            item->update(); QTest::qWait(120);
            const auto image = window.grabWindow().convertToFormat(QImage::Format_ARGB32);
            QVERIFY(!image.isNull());
            const qreal scale = qreal(image.width()) / window.width();
            int exact = 0;
            const int top = int((effect.ground() - 40) * scale);
            for (int y = std::max(0, top); y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x)
                    if (image.pixel(x, y) == signature[std::size_t(hero)]) ++exact;
            QVERIFY2(exact > 20, "The hero must be drawn crisply along the bottom edge.");
            const auto folder = qEnvironmentVariable("TE_RETRO_FRAMES");
            if (!folder.isEmpty()) QVERIFY(image.save(folder + QStringLiteral("/hero-%1.png").arg(hero)));
            window.hide();
        }
    }
    void rendersSakura() {
        if (QGuiApplication::platformName() == "offscreen") QSKIP("Requires accelerated desktop.");
        te::SakuraEffect sakura(57);
        te::EffectConfig config; config.kind = te::EffectKind::Sakura;
        sakura.configure(config);
        QQuickWindow window; window.setTitle("TFX — Sakura renderer test"); window.resize(800,600);
        auto* item = new te::EffectItem(sakura, window.contentItem()); item->setSize({800,600});
        for (int i=0;i<600;++i) sakura.advance(1.F/60.F,{});
        window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window)); QTest::qWait(150);
        if (window.rendererInterface()->graphicsApi() == QSGRendererInterface::Software) QSKIP("Requires GPU.");
        for (const auto background : {QColor("#182332"),QColor("#faf8f6")}) {
            window.setColor(background); item->update(); QTest::qWait(100);
            auto image=window.grabWindow(); QVERIFY(!image.isNull());
            int pink=0;
            for(int y=0;y<image.height();++y) for(int x=0;x<image.width();++x) {
                auto p=image.pixelColor(x,y);
                if(p.red()>p.green()+20 && p.blue()>p.green()+8) ++pink;
            }
            QVERIFY2(pink>200,"Pink petals must be legible on light and dark backgrounds.");
            const auto path=qEnvironmentVariable("TE_SAKURA_SCREENSHOT");
            if(!path.isEmpty()) QVERIFY(image.save(path+(background.lightness()>128?"-light.png":"-dark.png")));
        }
        config.sakuraBranches=false; sakura.configure(config); sakura.clear();
        window.setColor(Qt::transparent); item->update(); QTest::qWait(100);
        auto empty=window.grabWindow(); QVERIFY(empty.hasAlphaChannel());
        QCOMPARE(empty.pixelColor(empty.width()/2,empty.height()/2).alpha(),0);
        window.hide();
    }
    void rendersRainAndSwitchesEffects() {
        if (QGuiApplication::platformName() == "offscreen")
            QSKIP("Requires a real desktop and an accelerated Qt Quick backend.");
        te::RainEffect rain(57);
        te::EffectConfig config;
        config.kind = te::EffectKind::Rain;
        config.rainIntensity = 2; config.grassHeight = 90; config.wind = 0.45F;
        rain.configure(config);
        QQuickWindow window;
        window.setTitle(QStringLiteral("Telegram Effects — rain renderer test"));
        window.setColor(QColor("#172233"));
        window.resize(600, 420);
        auto* item = new te::EffectItem(rain, window.contentItem());
        item->setSize(QSizeF(600, 420));
        for (int i = 0; i < 180; ++i) rain.advance(1.F / 60.F, {});
        const auto checkSplash = [](const te::SplashVisual& splash) {
            return splash.opacity > 0.35F && splash.position.x > 4.F && splash.position.x < 596.F;
        };
        for (int i = 0; i < 60 && !std::ranges::any_of(rain.frame().splashes, checkSplash); ++i)
            rain.advance(1.F / 60.F, {});
        const auto splash = std::ranges::find_if(rain.frame().splashes, checkSplash);
        QVERIFY(splash != rain.frame().splashes.end());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::qWait(100);
        if (window.rendererInterface()->graphicsApi() == QSGRendererInterface::Software)
            QSKIP("Requires the default accelerated scene graph.");
        auto image = window.grabWindow();
        QVERIFY(!image.isNull());
        const auto dpr = image.devicePixelRatio();
        const auto splashColor = image.pixelColor(qRound(splash->position.x * dpr), qRound(splash->position.y * dpr));
        QVERIFY2(splashColor.blue() > 100, "A grass impact must render a visible pale splash at its simulated position.");
        int rainPixels = 0, grassPixels = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const auto color = image.pixelColor(x, y);
                if (y < image.height() * 0.7 && color.blue() > 70) ++rainPixels;
                if (y > image.height() * 0.75 && color.green() > color.blue() * 1.1
                    && color.green() > color.red() * 1.2) ++grassPixels;
            }
        }
        QVERIFY2(rainPixels > 100, "Raindrops must produce visible pixels.");
        QVERIFY2(grassPixels > 300, "Grass must render green blades at the lower edge.");
        const auto screenshot = qEnvironmentVariable("TE_RAIN_SCREENSHOT");
        if (!screenshot.isEmpty()) QVERIFY(image.save(screenshot));
        config.lightning = true;
        config.flashInterval = 5.F;
        rain.configure(config);
        for (int i = 0; i < 450; ++i) {
            rain.advance(1.F / 60.F, {});
            if (!rain.frame().flashes.empty() && rain.frame().flashes.front().opacity > 0.119F) break;
        }
        QVERIFY(!rain.frame().flashes.empty());
        item->update();
        const auto flashed = window.grabWindow();
        QVERIFY(!flashed.isNull());
        const auto lightningScreenshot = qEnvironmentVariable("TFX_LIGHTNING_FRAME");
        if (!lightningScreenshot.isEmpty()) QVERIFY(flashed.save(lightningScreenshot));
        const auto lightGain = [&](int x, int y) {
            int sum = 0;
            for (int dx = -12; dx <= 12; ++dx) {
                for (int dy = -3; dy <= 3; ++dy) {
                    const int px = qRound((x + dx) * dpr), py = qRound((y + dy) * dpr);
                    sum += flashed.pixelColor(px, py).blue() - image.pixelColor(px, py).blue();
                }
            }
            return sum / (25 * 7);
        };
        const auto centerGain = lightGain(426, 14);
        QVERIFY2(centerGain >= 16 && centerGain <= 30, "The peak flash must be visible and muted.");
        const auto tailGain = lightGain(426, 200);
        QVERIFY2(tailGain > 3 && tailGain < centerGain, "The sky glow must fall off softly into the scene.");
        config.lightning = false;
        rain.configure(config);
        te::SnowEffect snow(11);
        snow.configure({});
        item->setEffect(snow);
        for (int i = 0; i < 60; ++i) snow.advance(1.F / 60.F, {});
        item->update(); QTest::qWait(100);
        image = window.grabWindow();
        grassPixels = 0;
        for (int y = image.height() * 3 / 4; y < image.height(); ++y)
            for (int x = 0; x < image.width(); ++x) {
                const auto color = image.pixelColor(x, y);
                if (color.green() > color.blue() * 1.1 && color.green() > color.red() * 1.2) ++grassPixels;
            }
        QCOMPARE(grassPixels, 0);
        item->setEffect(rain);
        window.setColor(Qt::transparent);
        item->update(); QTest::qWait(100);
        const auto transparent = window.grabWindow();
        QVERIFY(transparent.hasAlphaChannel());
        QVERIFY(transparent.pixelColor(0, 0).alpha() < 5);
        window.hide();
        // Return the item to the effect that outlives the window.
        item->setEffect(rain);
    }
    void rendersSnowAndDrift() {
        if (QGuiApplication::platformName() == "offscreen")
            QSKIP("Requires a real desktop and an accelerated Qt Quick backend.");
        te::SnowEffect snow(142);
        te::EffectConfig config; config.intensity = 2; config.maxDepth = 32;
        snow.configure(config);
        QQuickWindow window;
        window.setTitle(QStringLiteral("Telegram Effects — renderer test"));
        window.setColor(QColor("#172233"));
        window.resize(600, 420);
        auto* item = new te::EffectItem(snow, window.contentItem());
        item->setSize(QSizeF(600, 420));
        for (int i = 0; i < 6000; ++i) snow.advance(1.F / 60.F, {});
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::qWait(100);
        if (window.rendererInterface()->graphicsApi() == QSGRendererInterface::Software)
            QSKIP("QSGVertexColorMaterial requires the default accelerated scene graph.");
        item->update();
        QTest::qWait(100);
        const QImage image = window.grabWindow();
        QVERIFY(!image.isNull());
        int bright = 0, bottomBright = 0;
        int airborneBright = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.red() > 130 && pixel.green() > 130) {
                    ++bright;
                    if (y > image.height() * 0.92) ++bottomBright;
                    else ++airborneBright;
                }
            }
        }
        QVERIFY2(bright > 100, "The particle geometry must actually produce bright pixels.");
        QVERIFY2(airborneBright > 20, "Falling particles must render separately from the drift.");
        QVERIFY2(bottomBright > 100, "The drift geometry must render at the lower edge.");
        const auto screenshot = qEnvironmentVariable("TE_RENDER_SCREENSHOT");
        if (!screenshot.isEmpty()) QVERIFY(image.save(screenshot));
        window.setColor(Qt::transparent);
        window.update();
        QTest::qWait(100);
        const auto transparent = window.grabWindow();
        QVERIFY(transparent.hasAlphaChannel());
        int clearPixels = 0;
        for (int y = 0; y < transparent.height() / 2; y += 4)
            for (int x = 0; x < transparent.width(); x += 4)
                if (transparent.pixelColor(x, y).alpha() < 5) ++clearPixels;
        QVERIFY2(clearPixels > 1000, "The effect must preserve a transparent background between particles.");
        window.hide();
    }
};
int main(int argc, char** argv) {
    QQuickWindow::setDefaultAlphaBuffer(true);
    QGuiApplication app(argc, argv);
    RendererTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "renderer_tests.moc"
