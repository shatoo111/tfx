#pragma once
#include "core/OverlayController.h"
#include "settings/Settings.h"
#include "core/ReleaseNotice.h"
#include "core/ReleaseUpdater.h"
#include <QMenu>
#include <QSystemTrayIcon>

namespace te {
class AppController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* updater READ updater CONSTANT)
    Q_PROPERTY(QObject* release READ release CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool reducedMotion READ reducedMotion NOTIFY statusChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY statusChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
public:
    explicit AppController(Settings& settings, bool smokeTest = false, QObject* parent = nullptr);
    void start();
    void refreshTarget();
    QObject* updater() { return &updater_; }
    QObject* release() { return &release_; }
    QString status() const;
    bool connected() const;
    bool reducedMotion() const { return reducedMotion_; }
    bool autoStart() const;
    void setAutoStart(bool enabled);
    QString error() const { return error_; }
    bool hasTray() const { return tray_.isVisible(); }
    Q_INVOKABLE void clearSnow();
    Q_INVOKABLE void dismissError();
    Q_INVOKABLE void quit();
signals:
    void statusChanged();
    void autoStartChanged();
    void errorChanged();
    void showSettingsRequested();
private:
    void applySettings();
    void setError(const QString& error);
    ReleaseUpdater updater_;
    ReleaseNotice release_;
    Settings& settings_;
    std::unique_ptr<NativePlatform> platform_;
    WindowTracker tracker_;
    OverlayController overlay_;
    QMenu menu_;
    QSystemTrayIcon tray_;
    QString error_;
    QTimer motionTimer_;
    bool reducedMotion_ = false;
    bool smokeTest_ = false;
};
}
