#include <functional>
#include "renderer/IllustratedPreview.h"
#include "renderer/SakuraPreview.h"
#include <qqml.h>
#include "settings/Settings.h"
#include <QApplication>
#include <QDesktopServices>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickStyle>
#include <QFontInfo>
#include <QRawFont>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtTest>

class ReleaseFixture final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString version MEMBER version NOTIFY changed)
    Q_PROPERTY(QString current READ current CONSTANT)
    Q_PROPERTY(QUrl page READ page CONSTANT)
public:
    QString version;
    QString current() const { return QStringLiteral("0.5.24"); }
    QUrl page() const { return QUrl(QStringLiteral("https://example.test/#download")); }
    Q_INVOKABLE void refresh() {}
    Q_INVOKABLE void dismiss() { version.clear(); emit changed(); }
    void offer(const QString& next) { version = next; emit changed(); }
signals:
    void changed();
};
// Only the platform-facing controller is replaced in this UI unit test.
// Production always constructs AppController + the real NativePlatform.
class UiControllerFixture final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap updater READ updater CONSTANT)
    Q_PROPERTY(QObject* release READ release CONSTANT)
    Q_PROPERTY(QString status READ status CONSTANT)
    Q_PROPERTY(bool reducedMotion READ reducedMotion CONSTANT)
    Q_PROPERTY(bool connected READ connected CONSTANT)
    Q_PROPERTY(bool autoStart MEMBER autoStart NOTIFY changed)
    Q_PROPERTY(QString error READ error CONSTANT)
public:
    QVariantMap updater() const { return {{"busy",false},{"available",false},{"needsToken",false},{"notes",""},{"message",""}}; }
    QObject* release() { return &notice; }
    ReleaseFixture notice;
    bool autoStart = false;
    int clears = 0;
    int quits = 0;
    QString status() const { return QStringLiteral("Ожидание Telegram"); }
    bool reducedMotion() const { return false; }
    bool connected() const { return false; }
    QString error() const { return {}; }
    Q_INVOKABLE void clearSnow() { ++clears; }
    Q_INVOKABLE void quit() { ++quits; }
signals:
    void changed();
};
// Stands in for the system when a link is opened.
class LinkCatcher final : public QObject {
    Q_OBJECT
public:
    QList<QUrl> opened;
    LinkCatcher() { for (const auto* scheme : {"tg", "https"}) QDesktopServices::setUrlHandler(QString::fromLatin1(scheme), this, "catchLink"); }
    ~LinkCatcher() override { for (const auto* scheme : {"tg", "https"}) QDesktopServices::unsetUrlHandler(QString::fromLatin1(scheme)); }
    Q_INVOKABLE void catchLink(const QUrl& url) { opened.push_back(url); }
};
class UiTests final : public QObject {
    Q_OBJECT
private slots:
    void loadAndBind() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        te::Settings settings(temporary.filePath("settings.ini"));
        UiControllerFixture controller;
        QQmlApplicationEngine engine;
        QStringList errors;
        connect(&engine, &QQmlEngine::warnings, this, [&errors](const QList<QQmlError>& warnings) {
            for (const auto& warning : warnings) errors.push_back(warning.toString());
        });
        engine.setInitialProperties({
            {QStringLiteral("preferences"), QVariant::fromValue<QObject*>(&settings)},
            {QStringLiteral("controller"), QVariant::fromValue<QObject*>(&controller)},
            {QStringLiteral("width"), 1120},
            {QStringLiteral("initiallyHidden"), false}});
        engine.load(QUrl::fromLocalFile(QStringLiteral(TE_SOURCE_DIR "/src/ui/Main.qml")));
        QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(errors.join('\n')));
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window);
        QTest::qWait(150);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        const auto font = window->property("font").value<QFont>();
        QCOMPARE(QFontInfo(font).family(), QStringLiteral("JetBrains Mono"));
        for (const auto* family : {"JetBrains Mono", "TFX Pixel"}) {
            QFont face(font);
            face.setFamily(QString::fromLatin1(family));
            QCOMPARE(QFontInfo(face).family(), QString::fromLatin1(family));
            const auto raw = QRawFont::fromFont(face);
            for (const auto letter : QStringLiteral("Дождливое настроение ЁёЖжЩщЯяЙй"))
                QVERIFY(raw.supportsCharacter(letter));
        }
        // Settings are longer than the window: bring an item to the middle before clicking it.
        const auto reveal = [window](QQuickItem* item) {
            auto* scroll = window->findChild<QQuickItem*>("settingsScroll");
            auto* viewport = qobject_cast<QQuickItem*>(scroll->property("contentItem").value<QObject*>());
            const double most = std::max(0., viewport->property("contentHeight").toDouble() - viewport->height());
            const double offset = item->mapToScene(item->boundingRect().center()).y() - window->height() / 2.;
            viewport->setProperty("contentY", std::clamp(viewport->property("contentY").toDouble() + offset, 0., most));
            QTest::qWait(20);
        };
        auto* toggle = window->findChild<QQuickItem*>("effectSwitch");
        QVERIFY(toggle);
        QVERIFY(toggle->property("checked").toBool());
        toggle->setProperty("checked", false);
        QVERIFY(QMetaObject::invokeMethod(toggle, "toggled"));
        QVERIFY(!settings.enabled());
        auto* wind = window->findChild<QQuickItem*>("windSlider");
        QVERIFY(wind);
        wind->setProperty("value", -0.6);
        QVERIFY(QMetaObject::invokeMethod(wind, "moved"));
        QVERIFY(std::abs(settings.wind() + 0.6) < 0.001);
        settings.setEnabled(true);
        auto* flightPause = window->findChild<QQuickItem*>("flightPauseSlider");
        auto* intensityControls = window->findChild<QQuickItem*>("intensityControls");
        QVERIFY(flightPause && intensityControls);
        for (auto theme : {te::EffectKind::UnitedStates, te::EffectKind::Stardust}) {
            settings.setEffect(int(theme));
            QVERIFY(flightPause->isVisible());
            QVERIFY(intensityControls->isVisible());
            flightPause->setProperty("value", 9);
            QVERIFY(QMetaObject::invokeMethod(flightPause, "moved"));
            QCOMPARE(settings.flightPause(), 9);
        }
        auto* retroControls = window->findChild<QQuickItem*>("retroControls");
        // Repeater delegates are reachable through the item tree, not QObject children.
        std::function<QQuickItem*(QQuickItem*, const QString&)> findItem = [&](QQuickItem* item, const QString& name) -> QQuickItem* {
            if (item->objectName() == name) return item;
            for (auto* child : item->childItems())
                if (auto* found = findItem(child, name)) return found;
            return nullptr;
        };
        auto* robot = retroControls ? findItem(retroControls, "retroHero2") : nullptr;
        QVERIFY(retroControls && robot);
        QVERIFY(!retroControls->isVisible());
        settings.setEffect(int(te::EffectKind::Retro));
        QVERIFY(retroControls->isVisible() && intensityControls->isVisible() && !flightPause->isVisible());
        QVERIFY(QMetaObject::invokeMethod(robot, "clicked"));
        QCOMPARE(settings.retroHero(), 2);
        auto* catControls = window->findChild<QQuickItem*>("catControls");
        auto* holeControls = window->findChild<QQuickItem*>("blackHoleControls");
        auto* sceneHint = window->findChild<QQuickItem*>("sceneHint");
        auto* festive = window->findChild<QQuickItem*>("festiveSwitch");
        auto* festiveRow = window->findChild<QQuickItem*>("festiveRow");
        QVERIFY(catControls && holeControls && sceneHint && festive && festiveRow);
        for (auto kind : {te::EffectKind::BlackHole, te::EffectKind::Cat, te::EffectKind::Aquarium}) {
            settings.setEffect(int(kind));
            QCOMPARE(holeControls->isVisible(), kind == te::EffectKind::BlackHole);
            QCOMPARE(catControls->isVisible(), kind == te::EffectKind::Cat);
            QCOMPARE(wind->isVisible(), kind != te::EffectKind::Cat);
            QVERIFY(sceneHint->isVisible() && intensityControls->isVisible() && !retroControls->isVisible() && !festiveRow->isVisible());
        }
        settings.setEffect(int(te::EffectKind::Cat));
        auto* white = findItem(catControls, "catCoat3");
        QVERIFY(white);
        QVERIFY(QMetaObject::invokeMethod(white, "clicked"));
        QCOMPARE(settings.catCoat(), 3);
        settings.setEffect(int(te::EffectKind::Snow));
        QVERIFY(festiveRow->isVisible() && !sceneHint->isVisible());
        QVERIFY(QMetaObject::invokeMethod(festive, "toggle"));
        QVERIFY(QMetaObject::invokeMethod(festive, "toggled"));
        QVERIFY(!settings.festive());
        QVERIFY(!flightPause->isVisible() && !retroControls->isVisible());
        settings.setWind(0.15);
        auto* picker = window->findChild<QQuickItem*>("themePicker");
        QVERIFY(picker);
        QVERIFY(QMetaObject::invokeMethod(picker, "activated", Q_ARG(int, 1)));
        QCOMPARE(settings.effect(), 1);
        QCOMPARE(window->findChild<QObject*>("surfaceRow")->property("title").toString(), QStringLiteral("Трава"));
        auto* surface = window->findChild<QQuickItem*>("surfaceSwitch");
        QVERIFY(surface);
        surface->setProperty("checked", false);
        QVERIFY(QMetaObject::invokeMethod(surface, "toggled"));
        QVERIFY(!settings.grass());
        QVERIFY(settings.accumulation());
        auto* height = window->findChild<QQuickItem*>("surfaceHeightSlider");
        QVERIFY(height);
        settings.setGrass(true);
        height->setProperty("value", 90);
        QVERIFY(QMetaObject::invokeMethod(height, "moved"));
        QCOMPARE(settings.grassHeight(), 90.);
        QCOMPARE(settings.maxDepth(), 48.);
        auto* lightning = window->findChild<QQuickItem*>("lightningSwitch");
        QVERIFY(lightning);
        lightning->setProperty("checked", true);
        QVERIFY(QMetaObject::invokeMethod(lightning, "toggled"));
        QVERIFY(settings.lightning());
        auto* interval = window->findChild<QQuickItem*>("flashIntervalSlider");
        QVERIFY(interval);
        interval->setProperty("value", 20);
        QVERIFY(QMetaObject::invokeMethod(interval, "moved"));
        QCOMPARE(settings.flashInterval(), 20.);
        QVERIFY(QMetaObject::invokeMethod(picker, "activated", Q_ARG(int, 2)));
        QCOMPARE(settings.effect(), 2);
        for (auto name : {"branchWidthSlider", "branchOpacitySlider"}) {
            auto* slider = window->findChild<QQuickItem*>(name);
            QVERIFY(slider);
            QTRY_VERIFY(slider->width() > 100);
            reveal(slider);
            for (double position : {0.2, 0.8}) {
                const auto point = slider->mapToScene({slider->width()*position, slider->height()/2}).toPoint();
                QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point);
                const double value = (slider->property("value").toDouble()-slider->property("from").toDouble())
                    / (slider->property("to").toDouble()-slider->property("from").toDouble());
                QVERIFY(std::abs(value-position) < 0.10);
                QCOMPARE(slider->property("value").toDouble(), slider->objectName() == "branchWidthSlider"
                         ? settings.branchWidth() : settings.branchOpacity());
            }
        }
        QCOMPARE(window->findChild<QObject*>("surfaceRow")->property("title").toString(), QStringLiteral("Ветки сакуры"));
        QVERIFY(!height->isVisible());
        QVERIFY(!window->findChild<QQuickItem*>("clearSnowButton")->isVisible());
        wind->setProperty("value", -42); QVERIFY(QMetaObject::invokeMethod(wind, "moved"));
        QCOMPARE(settings.sakuraAngle(), -42.);
        QVERIFY(std::abs(settings.wind()-0.15)<0.001);
        surface->setProperty("checked", false); QVERIFY(QMetaObject::invokeMethod(surface, "toggled"));
        QVERIFY(!settings.sakuraBranches()); QVERIFY(settings.grass()); QVERIFY(settings.accumulation());
        QVERIFY(QMetaObject::invokeMethod(picker, "activated", Q_ARG(int, 0)));
        QCOMPARE(settings.effect(), 0);
        QCOMPARE(window->findChild<QObject*>("surfaceRow")->property("title").toString(), QStringLiteral("Сугробы"));
        QVERIFY(!window->findChild<QQuickItem*>("stormCard")->isVisible());
        auto* fps = window->findChild<QQuickItem*>("fps30Button");
        QVERIFY(fps);
        reveal(fps);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                          fps->mapToScene(QPointF(fps->width() / 2, fps->height() / 2)).toPoint());
        QCOMPARE(settings.fps(), 30);
        fps->forceActiveFocus();
        settings.setFps(60);
        QTest::keyClick(window, Qt::Key_Space);
        QCOMPARE(settings.fps(), 30);
        settings.setFps(60);
        auto* fps60 = window->findChild<QQuickItem*>("fps60Button");
        QVERIFY(fps60);
        settings.setEconomy(true);
        QVERIFY(fps->property("selected").toBool());
        QVERIFY(!fps60->property("selected").toBool());
        QVERIFY(!fps->isEnabled() && !fps60->isEnabled());
        QCOMPARE(settings.fps(), 60);
        settings.setEconomy(false);
        QVERIFY(fps60->isEnabled() && fps60->property("selected").toBool());
        auto* clear = window->findChild<QQuickItem*>("clearSnowButton");
        QVERIFY(clear);
        QVERIFY(QMetaObject::invokeMethod(clear, "clicked"));
        QCOMPARE(controller.clears, 1);
        auto* interaction = window->findChild<QQuickItem*>("interactionSwitch");
        QVERIFY(interaction);
        interaction->setProperty("checked", false);
        QVERIFY(QMetaObject::invokeMethod(interaction, "toggled"));
        QVERIFY(!settings.interaction());
        auto* autoStart = window->findChild<QQuickItem*>("autoStartSwitch");
        QVERIFY(autoStart);
        autoStart->setProperty("checked", true);
        QVERIFY(QMetaObject::invokeMethod(autoStart, "toggled"));
        QVERIFY(controller.autoStart);
        QVERIFY(window->flags().testFlag(Qt::FramelessWindowHint));
        QVERIFY(window->findChild<QQuickItem*>("minimizeWindowButton"));
        LinkCatcher links;
        const QUrl channel(QStringLiteral("tg://resolve?domain=tfxsoft"));
        for (const auto* name : {"channelButton", "channelLinkButton"}) {
            auto* link = window->findChild<QQuickItem*>(name);
            QVERIFY2(link && link->isVisible(), name);
            if (link->objectName() == "channelLinkButton") reveal(link);
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, link->mapToScene(link->boundingRect().center()).toPoint());
            QTRY_COMPARE(links.opened.size(), 1);
            QCOMPARE(links.opened.takeFirst(), channel);
        }
        // A new version appears at the bottom, opens the site and can be put off.
        auto* bar = window->findChild<QQuickItem*>("releaseBar");
        auto* scrollArea = window->findChild<QQuickItem*>("settingsScroll");
        QVERIFY(bar && scrollArea && !bar->isVisible());
        const double fullHeight = scrollArea->height();
        controller.notice.offer(QStringLiteral("0.5.25"));
        QTRY_VERIFY(bar->isVisible() && bar->height() > 80);
        QVERIFY(scrollArea->height() < fullHeight - 60);
        QCOMPARE(window->findChild<QQuickItem*>("releaseTitle")->property("text").toString(), QStringLiteral("Доступна TFX 0.5.25"));
        auto* releaseButton = window->findChild<QQuickItem*>("releaseButton");
        auto* laterButton = window->findChild<QQuickItem*>("releaseLaterButton");
        QVERIFY(releaseButton && laterButton);
        QVERIFY(releaseButton->mapToScene(releaseButton->boundingRect().center()).y() > window->height() - 86);
        QTest::qWait(300);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, releaseButton->mapToScene(releaseButton->boundingRect().center()).toPoint());
        QTRY_COMPARE(links.opened.size(), 1);
        QCOMPARE(links.opened.takeFirst(), controller.notice.page());
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, laterButton->mapToScene(laterButton->boundingRect().center()).toPoint());
        QTRY_VERIFY(!bar->isVisible());
        QTRY_COMPARE(scrollArea->height(), fullHeight);
        auto* closeButton = window->findChild<QQuickItem*>("closeWindowButton");
        auto* dialog = window->findChild<QObject*>("quitDialog");
        auto* cancelExit = window->findChild<QQuickItem*>("cancelExitButton");
        auto* confirmExit = window->findChild<QQuickItem*>("confirmExitButton");
        QVERIFY(closeButton && dialog && cancelExit && confirmExit);
        const auto click = [window](QQuickItem* item) {
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                              item->mapToScene(item->boundingRect().center()).toPoint());
        };
        click(closeButton);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QCOMPARE(controller.quits, 0);
        click(cancelExit);
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QCOMPARE(controller.quits, 0);
        click(closeButton);
        QTRY_VERIFY(dialog->property("visible").toBool());
        click(confirmExit);
        QTRY_COMPARE(controller.quits, 1);
        fps->setFocus(false);
        if (qEnvironmentVariableIsSet("TE_UI_WIDTH"))
            window->resize(qEnvironmentVariableIntValue("TE_UI_WIDTH"), 820);
        if (qEnvironmentVariableIsSet("TE_UI_RELEASE")) controller.notice.offer(QStringLiteral("0.5.25"));
        if (qEnvironmentVariableIsSet("TE_UI_RAIN")) {
            settings.setEffect(1);
            settings.setIntensity(2);
            settings.setWind(0.28);
            settings.setGrassHeight(57);
        }
        if (qEnvironmentVariableIsSet("TE_UI_THEME")) {
            settings.setEnabled(true);
            settings.setFestive(true);
            settings.setCatCoat(0);
            settings.setEffect(qEnvironmentVariableIntValue("TE_UI_THEME"));
            QTest::qWait(900);
        }
        if (qEnvironmentVariableIsSet("TE_UI_RETRO")) {
            settings.setEffect(int(te::EffectKind::Retro));
            settings.setRetroHero(qEnvironmentVariableIntValue("TE_UI_RETRO"));
        }
        if (qEnvironmentVariableIsSet("TE_UI_SAKURA")) {
            settings.setEffect(2);
            settings.setBranchSide(2);
            settings.setSakuraBranches(true);
            settings.setBranchWidth(78);
            settings.setBranchOpacity(1);
        }
        if (qEnvironmentVariableIsSet("TE_UI_SCROLL")) reveal(window->findChild<QQuickItem*>(qEnvironmentVariable("TE_UI_SCROLL")));
        else window->findChild<QQuickItem*>("settingsScroll")->property("contentItem").value<QObject*>()->setProperty("contentY", 0);
        QTest::qWait(qEnvironmentVariableIntValue("TE_UI_WAIT") + 200);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        const auto screenshot = qEnvironmentVariable("TE_UI_SCREENSHOT");
        if (!screenshot.isEmpty()) {
            if (qEnvironmentVariableIsSet("TE_UI_CLOSE")) {
                click(closeButton);
                QTRY_VERIFY(dialog->property("visible").toBool());
                QTest::qWait(180);
            }
            const QImage image = window->grabWindow();
            QVERIFY(!image.isNull());
            QVERIFY(image.save(screenshot));
            qInfo("Settings capture: %dx%d, device pixel ratio %.2f.",
                  image.width(), image.height(), window->devicePixelRatio());
        }
        window->close();
    }
    void animatesWeatherPreview() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        te::Settings settings(temporary.filePath("settings.ini"));
        UiControllerFixture controller;
        QQmlApplicationEngine engine;
        engine.setInitialProperties({
            {QStringLiteral("preferences"), QVariant::fromValue<QObject*>(&settings)},
            {QStringLiteral("controller"), QVariant::fromValue<QObject*>(&controller)}});
        engine.load(QUrl::fromLocalFile(QStringLiteral(TE_SOURCE_DIR "/src/ui/Main.qml")));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto* preview = engine.rootObjects().first()->findChild<QQuickItem*>("weatherPreview");
        QVERIFY(preview);
        const auto capture = [preview] {
            auto result = preview->grabToImage();
            if (!result) return QImage();
            QSignalSpy ready(result.get(), &QQuickItemGrabResult::ready);
            if (!ready.wait(1000)) return QImage();
            return result->image();
        };
        settings.setSakuraBranches(false);
        settings.setEnabled(false);
        for (const auto effect : {0, 1, 2}) {
            settings.setEffect(effect);
            QTest::qWait(50);
            const auto background = capture();
            QVERIFY(!background.isNull());
            settings.setEnabled(true);
            QTest::qWait(50);
            const auto first = capture();
            QVERIFY(!first.isNull());
            QVERIFY2(first != background, "The preview must show particles as soon as it opens.");
            const auto particlesMoved = [&] {
                const auto next = capture();
                return !next.isNull() && next != first;
            };
            QTRY_VERIFY_WITH_TIMEOUT(particlesMoved(), 2000);
            settings.setEnabled(false);
            QTest::qWait(50);
            QCOMPARE(capture(), background);
        }

        // Animation frames should not repaint the static backdrop or accumulate
        // simulation time while the preview has scrolled fully out of view.
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window);
        window->resize(400, 520);
        settings.setEffect(1);
        settings.setEnabled(true);
        auto* scroll = window->findChild<QQuickItem*>("settingsScroll");
        QVERIFY(scroll);
        auto* viewport = qobject_cast<QQuickItem*>(scroll->property("contentItem").value<QObject*>());
        QVERIFY(viewport);
        QTest::qWait(60);
        const double sample = viewport->property("contentY").toDouble()
            + preview->mapToItem(viewport, QPointF()).y() - 40;
        QVERIFY(viewport->setProperty("contentY", sample));
        QTRY_VERIFY(preview->property("animationActive").toBool());
        auto* backdrop = preview->findChild<QObject*>("weatherPreviewBackdrop");
        QVERIFY(backdrop);
        QSignalSpy backdropPaints(backdrop, SIGNAL(painted()));
        QVERIFY(backdropPaints.isValid());
        QVERIFY(!capture().isNull()); // Let the theme change and resize finish painting.
        backdropPaints.clear();
        const auto time = [preview] { return preview->property("animationTime").toDouble(); };
        const double visibleTime = time();
        QTRY_VERIFY(time() > visibleTime + 0.05);
        QCOMPARE(backdropPaints.count(), 0);

        const double bottom = viewport->property("contentHeight").toDouble() - viewport->height();
        QVERIFY(bottom > sample + preview->height());
        QVERIFY(viewport->setProperty("contentY", bottom));
        QTRY_VERIFY(!preview->property("animationActive").toBool());
        const double pausedTime = time();
        QTest::qWait(120);
        QCOMPARE(time(), pausedTime);
        QVERIFY(viewport->setProperty("contentY", sample));
        QTRY_VERIFY(preview->property("animationActive").toBool());
        QTRY_VERIFY(time() > pausedTime);
    }
    void responsiveLayout_data() {
        QTest::addColumn<int>("width");
        QTest::addColumn<int>("effect");
        for (const int width : {400, 600, 859, 860, 1120}) {
            for (const int effect : {0, 1, 2}) {
                const auto name = QByteArray::number(width) + (effect == 2 ? "-sakura" : effect ? "-rain" : "-snow");
                QTest::newRow(name.constData()) << width << effect;
            }
        }
    }
    void responsiveLayout() {
        QFETCH(int, width);
        QFETCH(int, effect);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        te::Settings settings(temporary.filePath("settings.ini"));
        settings.setEffect(effect);
        UiControllerFixture controller;
        controller.notice.offer(QStringLiteral("0.5.25"));
        QQmlApplicationEngine engine;
        QStringList errors;
        connect(&engine, &QQmlEngine::warnings, this, [&errors](const QList<QQmlError>& warnings) {
            for (const auto& warning : warnings) errors.push_back(warning.toString());
        });
        engine.setInitialProperties({
            {QStringLiteral("preferences"), QVariant::fromValue<QObject*>(&settings)},
            {QStringLiteral("controller"), QVariant::fromValue<QObject*>(&controller)},
            {QStringLiteral("width"), width}});
        engine.load(QUrl::fromLocalFile(QStringLiteral(TE_SOURCE_DIR "/src/ui/Main.qml")));
        QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(errors.join('\n')));
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window);
        QTest::qWait(100);
        for (const auto* name : {"themePicker", "effectSwitch", "minimizeWindowButton", "closeWindowButton",
                                "weatherCard", "surfaceCard", "windSlider",
                                "fps30Button", "fps60Button", "quitButton", "channelLinkButton",
                                "releaseBar", "releaseButton", "releaseLaterButton"}) {
            auto* item = window->findChild<QQuickItem*>(name);
            QVERIFY2(item, name);
            const auto bounds = item->mapRectToScene(item->boundingRect());
            QVERIFY2(bounds.width() > 0 && bounds.left() >= 0 && bounds.right() <= width, name);
        }
        auto* weather = window->findChild<QQuickItem*>("weatherCard");
        auto* surface = window->findChild<QQuickItem*>("surfaceCard");
        auto* scrollBar = window->findChild<QQuickItem*>("settingsScrollBar");
        QVERIFY(scrollBar);
        auto* thumb = qobject_cast<QQuickItem*>(scrollBar->property("contentItem").value<QObject*>());
        QVERIFY(thumb);
        for (double position : {0., 1. - scrollBar->property("size").toDouble()}) {
            QVERIFY(scrollBar->setProperty("position", std::max(0., position)));
            const auto bounds = thumb->mapRectToScene(thumb->boundingRect());
            QVERIFY(bounds.right() <= width - 8);
            QVERIFY(bounds.bottom() <= window->height() - 12);
        }
        const auto weatherBounds = weather->mapRectToScene(weather->boundingRect());
        const auto surfaceBounds = surface->mapRectToScene(surface->boundingRect());
        if (width < 860) QVERIFY(surfaceBounds.top() >= weatherBounds.bottom());
        else QVERIFY(surfaceBounds.left() >= weatherBounds.right());
        // Resizing the live window must also restore the wide layout.
        window->resize(1120, 820);
        QTest::qWait(50);
        QVERIFY(surface->mapToScene(QPointF()).x() > weather->mapToScene(QPointF()).x());
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        window->close();
    }
};
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    qmlRegisterType<te::IllustratedPreview>("Tfx.Scene",1,0,"IllustratedPreview");
    qmlRegisterType<te::SakuraPreview>("Tfx.Scene",1,0,"SakuraPreview");
    QQuickStyle::setStyle("Basic");
    UiTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "ui_tests.moc"
