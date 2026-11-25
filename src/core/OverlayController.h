#pragma once
#include "core/WindowTracker.h"
#include "effects/IEffect.h"
#include <QElapsedTimer>
#include <QQuickWindow>
#include <QTimer>
#include <memory>

namespace te {
class EffectItem;
class OverlayFrameTimer;
class OverlayController final : public QObject {
    Q_OBJECT
public:
    OverlayController(NativePlatform& platform, WindowTracker& tracker, QObject* parent = nullptr);
    ~OverlayController() override;
    void configure(EffectConfig config);
    void clearSnow();
    enum class State { Off, Missing, Hidden, Preparing, Running, Recovering, Failed };
    State state() const { return state_; }
    QString status() const { return status_; }
    QString diagnostics() const;
signals:
    void statusChanged();
private:
    void synchronize();
    void tick();
    void watchPress(quintptr target);
    void createLayer();
    void retireLayer();
    void recover(const QString& reason);
    void record(const QString& event);
    void transition(State state, const QString& reason);
    void schedule();
    void restartFrameTimer();
    void ensureRunning();
    NativePlatform& platform_;
    WindowTracker& tracker_;
    std::unique_ptr<IEffect> effect_;
    std::unique_ptr<QQuickWindow> window_;
    EffectItem* item_ = nullptr;
    std::unique_ptr<OverlayFrameTimer> timer_;
    QElapsedTimer elapsed_;
    qint64 lastTickNanos_ = 0, nextTickNanos_ = 0;
    QElapsedTimer lastFrame_;
    EffectConfig config_;
    bool pendingFrame_ = false, native_ = false;
    State state_ = State::Off;
    QString status_, failure_;
    QStringList history_;
    QRectF loggedBounds_;
    qreal loggedDpr_ = 0;
    int retries_ = 0;
    quint64 generation_ = 0;
    QElapsedTimer healthy_;
    QTimer retry_;
    int ticks_ = 0, lateTicks_ = 0;
    float loadScale_ = 1;
    quintptr targetId_ = 0, pressTarget_ = 0;
    quint64 pressWatchId_ = 0;
};
}
