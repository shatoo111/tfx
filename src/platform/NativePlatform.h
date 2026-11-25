#pragma once
#include <QPointF>
#include <QRectF>
#include <QString>
#include <functional>
#include <memory>

class QWindow;
namespace te {
// Native coordinates: Windows physical pixels; macOS Quartz global points, top-left origin.
struct WindowSnapshot {
    quintptr id = 0;
    QRectF bounds;
    bool running = false;
    bool visible = false;
    bool active = false;
    QString detail;
    bool needsPlacement = false;
    bool canOverlay() const { return id && visible && bounds.width() > 0 && bounds.height() > 0; }
};
class NativePlatform {
public:
    virtual ~NativePlatform() = default;
    virtual WindowSnapshot sample() = 0;
    virtual void configureOverlay(QWindow& window) = 0;
    virtual void placeOverlay(QWindow& window, const WindowSnapshot& snapshot) = 0;
    virtual QPointF cursorInOverlay(const WindowSnapshot& snapshot, const QWindow& overlay) const = 0;
    // Report primary presses in the same native screen coordinates as WindowSnapshot::bounds.
    virtual void watchPrimaryPress(quintptr, std::function<void(QPointF)>) {}
    virtual void watch(std::function<void()> changed) = 0;
    // Whether a window may be on the move right now (a pressed mouse button, say).
    virtual bool pointerPressed() const { return true; }
    virtual bool layerOccluded() const { return false; }
    // Keep the process at full speed while the overlay animates, even when the
    // settings window is minimized or closed to the tray (App Nap / EcoQoS).
    virtual void setRenderingActive(bool) {}
    virtual bool reduceMotion() const { return false; }
    virtual bool autoStartEnabled() const = 0;
    virtual bool setAutoStart(bool enabled, QString& error) = 0;
};
std::unique_ptr<NativePlatform> createNativePlatform();
}
