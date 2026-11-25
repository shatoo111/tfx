#include "core/WindowTracker.h"
#include <QMetaObject>
#include <QPointer>

namespace te {
namespace {
constexpr qint64 settleMs = 700;   // A window that has not moved for this long is at rest.
constexpr qint64 burstMs = 4000;   // After a native event: windows become usable a little later.
constexpr qint64 restMs = 120;     // Polling of a resting window.
}
WindowTracker::WindowTracker(NativePlatform& platform, QObject* parent) : QObject(parent), platform_(platform) {
    timer_.setTimerType(Qt::PreciseTimer);
    connect(&timer_, &QTimer::timeout, this, &WindowTracker::poll);
    // Native callbacks may be on a foreign thread. Never call Qt UI code directly there.
    const QPointer<WindowTracker> guard(this);
    platform_.watch([guard] {
        if (guard) QMetaObject::invokeMethod(guard, [guard] {
            if (!guard) return;
            guard->lastEvent_.start();
            guard->refresh();
        }, Qt::QueuedConnection);
    });
}
void WindowTracker::start() { refresh(); timer_.start(); }
void WindowTracker::setFastPolling(bool enabled) {
    fastPolling_ = enabled;
    updatePollingInterval();
}
void WindowTracker::updatePollingInterval() {
    // Events wake the tracker at once; polling only covers what they miss.
    const bool expecting = lastEvent_.isValid() && lastEvent_.elapsed() < burstMs;
    const int interval = !fastPolling_ ? 750
        : snapshot_.canOverlay() ? 33 : expecting ? 100 : snapshot_.running ? 250 : 500;
    if (timer_.interval() != interval) timer_.setInterval(interval);
}
void WindowTracker::poll() {
    // A resting window is sampled rarely; a pressed button or a recent change restores the full rate.
    if (fastPolling_ && snapshot_.canOverlay() && lastChange_.isValid() && lastChange_.elapsed() > settleMs
        && lastSample_.isValid() && lastSample_.elapsed() < restMs && !platform_.pointerPressed()) return;
    refresh();
}
void WindowTracker::refresh() {
    lastSample_.start();
    auto next = platform_.sample();
    const bool different = next.id != snapshot_.id || next.bounds != snapshot_.bounds
        || next.running != snapshot_.running || next.active != snapshot_.active
        || next.visible != snapshot_.visible || next.detail != snapshot_.detail;
    const bool needsPlacement = next.needsPlacement;
    if (different || needsPlacement || !lastChange_.isValid()) lastChange_.start();
    snapshot_ = std::move(next);
    // Events provide immediate invalidation; low frequency polling covers missed notifications.
    updatePollingInterval();
    if (different || needsPlacement) emit changed();
    emit sampled();
}
}
