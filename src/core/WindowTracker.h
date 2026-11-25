#pragma once
#include "platform/NativePlatform.h"
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace te {
class WindowTracker final : public QObject {
    Q_OBJECT
public:
    explicit WindowTracker(NativePlatform& platform, QObject* parent = nullptr);
    const WindowSnapshot& snapshot() const { return snapshot_; }
    void start();
    void refresh();
    void setFastPolling(bool enabled);
signals:
    void changed();
    void sampled();
private:
    void poll();
    void updatePollingInterval();
    NativePlatform& platform_;
    WindowSnapshot snapshot_;
    QTimer timer_;
    QElapsedTimer lastEvent_, lastChange_, lastSample_;
    bool fastPolling_ = true;
};
}
