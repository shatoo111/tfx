#include "renderer/IllustratedPreview.h"
#include "renderer/SakuraPreview.h"
#include <qqml.h>
#include "core/AppController.h"
#include "settings/Settings.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QTimer>
#include <QTemporaryDir>

int main(int argc, char** argv) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QQuickWindow::setDefaultAlphaBuffer(true);
    QSurfaceFormat format; format.setAlphaBufferSize(8); format.setSwapInterval(1);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("TelegramEffects"));
    app.setOrganizationDomain(QStringLiteral("telegrameffects.org"));
    app.setApplicationName(QStringLiteral("TelegramEffects"));
    app.setApplicationDisplayName(QStringLiteral("TFX"));
    app.setApplicationVersion(QStringLiteral(TE_APP_VERSION));
    app.setQuitOnLastWindowClosed(false);
    qmlRegisterType<te::IllustratedPreview>("Tfx.Scene",1,0,"IllustratedPreview");
    qmlRegisterType<te::SakuraPreview>("Tfx.Scene",1,0,"SakuraPreview");
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Local weather effects for the official Telegram Desktop."));
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({QStringLiteral("background"), QStringLiteral("Start in the tray / menu bar.")});
    parser.addOption({QStringLiteral("smoke-test"), QStringLiteral("Load the Qt UI, then exit after two seconds.")});
#ifdef Q_OS_MACOS
    const QCommandLineOption updateReadyFile(QStringLiteral("update-ready-file"),
        QStringLiteral("Acknowledge the first settings frame to the updater."), QStringLiteral("path"));
    parser.addOption(updateReadyFile);
#endif
    parser.process(app);
    const bool smokeTest = parser.isSet("smoke-test");
    if (smokeTest) QStandardPaths::setTestModeEnabled(true);
    const auto statePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!QDir().mkpath(statePath)) {
        QMessageBox::critical(nullptr, QStringLiteral("Telegram Effects"),
            QStringLiteral("Не удалось создать пользовательский каталог приложения: %1").arg(statePath));
        return 1;
    }
    QLockFile lock(statePath + QStringLiteral("/instance.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0)) {
        if (lock.error() != QLockFile::LockFailedError) {
            QMessageBox::critical(nullptr, QStringLiteral("Telegram Effects"),
                QStringLiteral("Не удалось создать файл блокировки. Проверьте доступ к каталогу приложения."));
            return 1;
        }
        if (!parser.isSet("background")) QMessageBox::information(nullptr, QStringLiteral("Telegram Effects"),
            QStringLiteral("Приложение уже запущено. Откройте настройки через значок в трее или строке меню."));
        return 0;
    }
    QTemporaryDir smokeSettings;
    te::Settings settings(smokeTest ? smokeSettings.filePath(QStringLiteral("settings.ini")) : QString());
    te::AppController controller(settings, smokeTest);
    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("preferences"), QVariant::fromValue<QObject*>(&settings)},
        {QStringLiteral("controller"), QVariant::fromValue<QObject*>(&controller)},
        {QStringLiteral("initiallyHidden"), parser.isSet("background") && controller.hasTray()}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("TelegramEffects"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) return 1;
    auto* settingsWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if (!settingsWindow) return 1;
#ifdef Q_OS_MACOS
    if (parser.isSet(updateReadyFile)) {
        const auto path = parser.value(updateReadyFile);
        QObject::connect(settingsWindow, &QQuickWindow::frameSwapped, &app, [path] {
            QSaveFile ready(path);
            if (!ready.open(QIODevice::WriteOnly) || ready.write("ready\n") != 6 || !ready.commit())
                QCoreApplication::exit(1);
        }, Qt::SingleShotConnection);
    }
#endif
    settingsWindow->setIcon(app.windowIcon());
    QObject::connect(&controller, &te::AppController::showSettingsRequested, settingsWindow, [settingsWindow] {
        settingsWindow->showNormal(); settingsWindow->raise(); settingsWindow->requestActivate();
    });
    QObject::connect(settingsWindow, &QQuickWindow::closing, &app, [&controller] {
        if (!controller.hasTray()) controller.quit();
    });
    // Windows may transfer focus after notifying Qt that settings lost focus or were minimized.
    QObject::connect(settingsWindow, &QWindow::activeChanged,
                     &controller, &te::AppController::refreshTarget, Qt::QueuedConnection);
    QObject::connect(settingsWindow, &QWindow::visibilityChanged,
                     &controller, &te::AppController::refreshTarget, Qt::QueuedConnection);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &settings, [&settings] { settings.flush(); });
    controller.start();
    if (parser.isSet("smoke-test")) QTimer::singleShot(2000, &app, &QCoreApplication::quit);
    return app.exec();
}
