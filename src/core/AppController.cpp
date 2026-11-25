#include "core/AppController.h"
#include <QApplication>
#include <QIcon>

namespace te {
namespace { const QStringList themeNames = {"Снегопад", "Дождь", "Сакура", "Осенние листья", "The United States", "Звёздная пыль", "8 бит", "Чёрная дыра", "Котик", "Аквариум"}; }
AppController::AppController(Settings& settings, bool smokeTest, QObject* parent)
    : QObject(parent), settings_(settings), platform_(createNativePlatform()), tracker_(*platform_),
      overlay_(*platform_, tracker_), smokeTest_(smokeTest) {
    connect(&settings_, &Settings::changed, this, &AppController::applySettings);
    connect(&settings_, &Settings::saveFailed, this, &AppController::setError);
    connect(&overlay_, &OverlayController::statusChanged, this, &AppController::statusChanged);
    const QIcon icon(QStringLiteral(":/assets/tfx-icon.svg"));
    QApplication::setWindowIcon(icon); tray_.setIcon(icon);
    tray_.setToolTip(QStringLiteral("TFX"));
    menu_.addAction(QStringLiteral("Настройки…"), this, [this] { emit showSettingsRequested(); });
    auto* enabled = menu_.addAction(themeNames.at(settings_.effect()));
    enabled->setCheckable(true); enabled->setChecked(settings_.enabled());
    connect(enabled, &QAction::triggered, &settings_, &Settings::setEnabled);
    auto* clearSnow = menu_.addAction(QStringLiteral("Очистить снег"), this, &AppController::clearSnow);
    clearSnow->setVisible(settings_.effect() == 0);
    connect(&settings_, &Settings::changed, enabled, [this, enabled, clearSnow] {
        enabled->setChecked(settings_.enabled());
        enabled->setText(themeNames.at(settings_.effect()));
        clearSnow->setVisible(settings_.effect() == 0);
    });
    menu_.addSeparator();
    menu_.addAction(QStringLiteral("Завершить"), this, &AppController::quit);
    tray_.setContextMenu(&menu_);
    connect(&tray_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) emit showSettingsRequested();
    });
    if (!smokeTest_ && QSystemTrayIcon::isSystemTrayAvailable()) tray_.show();
    reducedMotion_ = platform_->reduceMotion();
    connect(&motionTimer_, &QTimer::timeout, this, [this] {
        const bool reduced = platform_->reduceMotion();
        if (reducedMotion_ == reduced) return;
        reducedMotion_ = reduced;
        applySettings();
    });
    if (!smokeTest_) motionTimer_.start(2000);
    applySettings();
}
void AppController::applySettings() {
    auto config = settings_.config();
    config.reducedMotion = reducedMotion_;
    overlay_.configure(config);
    emit statusChanged();
}
void AppController::start() {
    if (smokeTest_) return;
    tracker_.start();
    release_.start();
}
void AppController::refreshTarget() { if (!smokeTest_) tracker_.refresh(); }
QString AppController::status() const { return overlay_.status(); }
bool AppController::connected() const { return overlay_.state() == OverlayController::State::Running; }
bool AppController::autoStart() const { return platform_->autoStartEnabled(); }
void AppController::setAutoStart(bool enabled) {
    QString error;
    if (!platform_->setAutoStart(enabled, error)) setError(error);
    emit autoStartChanged();
}
void AppController::setError(const QString& error) { error_ = error; emit errorChanged(); }
void AppController::dismissError() { error_.clear(); emit errorChanged(); }
void AppController::clearSnow() { overlay_.clearSnow(); }
void AppController::quit() { settings_.flush(); QCoreApplication::quit(); }
}
