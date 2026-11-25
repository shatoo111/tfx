#include "settings/Settings.h"
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <array>
#include <utility>

class SettingsTests final : public QObject {
    Q_OBJECT
private slots:
    void retiredProfileReturnsToSnow() {
        QTemporaryDir dir;
        const auto path = dir.filePath("retired.ini");
        {
            QSettings raw(path,QSettings::IniFormat);
            raw.setValue("effect/kind",4);
            raw.setValue("themes/4/wind",-.9);
            raw.setValue("themes/5/wind",.7);
        }
        {
            te::Settings settings(path);
            QCOMPARE(settings.effect(),0);
            settings.setEffect(int(te::EffectKind::Stardust));
            QVERIFY(std::abs(settings.wind()-.7)<.001);
            settings.setEffect(int(te::EffectKind::UnitedStates));
            QVERIFY(std::abs(settings.wind()-.15)<.001);
        }
        QSettings raw(path,QSettings::IniFormat);
        QCOMPARE(raw.value("effect/kind").toInt(),6);
        QVERIFY(!raw.contains("themes/4/wind"));
        te::Settings restored(path);
        QCOMPARE(restored.effect(),int(te::EffectKind::UnitedStates));
    }
    void storesRetroThemeSeparately() {
        QTemporaryDir dir;
        const auto path = dir.filePath("retro.ini");
        {
            te::Settings settings(path);
            settings.setEffect(int(te::EffectKind::Stardust));
            settings.setFlightPause(40);
            settings.setEffect(int(te::EffectKind::Retro));
            QCOMPARE(settings.flightPause(), 6); // Its own default, not the comets' pause.
            settings.setFlightPause(12);
            settings.setRetroHero(1);
            settings.setRetroHero(7);
            QCOMPARE(settings.retroHero(), 2);
            settings.setRetroHero(1);
        }
        QSettings raw(path, QSettings::IniFormat);
        QCOMPARE(raw.value("effect/kind").toInt(), 7);
        QCOMPARE(raw.value("themes/7/flightPause").toInt(), 12);
        QCOMPARE(raw.value("themes/5/flightPause").toInt(), 40);
        te::Settings restored(path);
        QCOMPARE(restored.effect(), int(te::EffectKind::Retro));
        QCOMPARE(restored.retroHero(), 1);
        QCOMPARE(restored.flightPause(), 12);
        QCOMPARE(int(restored.config().kind), int(te::EffectKind::Retro));
    }
    void storesVectorThemes() {
        QTemporaryDir dir;
        const auto path = dir.filePath("vector.ini");
        {
            te::Settings settings(path);
            QVERIFY(settings.festive());
            settings.setFestive(false);
            settings.setCatCoat(9);
            QCOMPARE(settings.catCoat(), 3);
            settings.setEffect(int(te::EffectKind::BlackHole));
            QCOMPARE(settings.flightPause(), 12);
            settings.setFlightPause(20);
            settings.setIntensity(2);
            settings.setEffect(int(te::EffectKind::Cat));
            QCOMPARE(settings.intensity(), 1);
            settings.setEffect(99);
            QCOMPARE(settings.effect(), 0);
        }
        // Disk identifiers never collide with earlier themes.
        const std::array<std::pair<te::EffectKind,int>,4> ids{{{te::EffectKind::Retro,7},{te::EffectKind::BlackHole,8},{te::EffectKind::Cat,9},{te::EffectKind::Aquarium,10}}};
        for (const auto& [kind, id] : ids) {
            { te::Settings settings(path); settings.setEffect(int(kind)); }
            QCOMPARE(QSettings(path, QSettings::IniFormat).value("effect/kind").toInt(), id);
            te::Settings restored(path);
            QCOMPARE(restored.effect(), int(kind));
            QCOMPARE(restored.festive(), false);
            QCOMPARE(restored.catCoat(), 3);
            if (kind == te::EffectKind::BlackHole) { QCOMPARE(restored.flightPause(), 20); QCOMPARE(restored.intensity(), 2); }
        }
    }
    void recoversInvisibleSakuraBranches() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto path = dir.filePath("legacy.ini");
        {
            QSettings legacy(path, QSettings::IniFormat);
            legacy.setValue("sakura/opacity", 0);
            legacy.setValue("sakura/width", 24);
            legacy.setValue("sakura/side", 0);
        }
        te::Settings settings(path);
        QCOMPARE(settings.branchOpacity(), 1.);
        QCOMPARE(settings.branchWidth(), 78.);
        QCOMPARE(settings.branchSide(), 0);
        settings.setBranchOpacity(0);
        QCOMPARE(settings.branchOpacity(), settings.minBranchOpacity());
    }
    void independentThemeProfiles() {
        QTemporaryDir dir;
        const auto path = dir.filePath("profiles.ini");
        {
            te::Settings settings(path);
            for (int theme = 0; theme < 6; ++theme) {
                settings.setEffect(theme);
                settings.applyPreset(theme % 3);
                settings.setWind(theme * 0.1);
                settings.setInteraction(theme % 2 == 0);
            }
            settings.setEconomy(true);
            settings.setBranchSide(2);
            settings.setBranchOpacity(0.25);
            settings.setBranchWidth(56);
        }
        te::Settings restored(path);
        QVERIFY(restored.economy());
        QCOMPARE(restored.branchSide(), 2);
        QCOMPARE(restored.branchOpacity(), 0.25);
        QCOMPARE(restored.branchWidth(), 56.);
        for (int theme = 0; theme < 6; ++theme) {
            restored.setEffect(theme);
            QCOMPARE(restored.intensity(), theme % 3);
            QVERIFY(std::abs(restored.wind()-theme*0.1) < 0.001);
            QCOMPARE(restored.interaction(), theme % 2 == 0);
            QCOMPARE(restored.fps(), theme % 3 == 0 ? 30 : 60);
        }
    }
    void flightProfiles() {
        QTemporaryDir dir;
        const auto path = dir.filePath("flights.ini");
        {
            te::Settings settings(path);
            settings.setEffect(int(te::EffectKind::UnitedStates));
            QCOMPARE(settings.flightPause(), 12);
            QCOMPARE(settings.flightLimit(), 2);
            settings.setFlightPause(8);
            settings.setFlightLimit(3);
            settings.setEffect(int(te::EffectKind::Stardust));
            QCOMPARE(settings.flightLimit(), 2);
            settings.setFlightPause(100);
            settings.setFlightLimit(-1);
        }
        te::Settings restored(path);
        QCOMPARE(restored.flightPause(), 60);
        QCOMPARE(restored.flightLimit(), 1);
        restored.setEffect(int(te::EffectKind::UnitedStates));
        QCOMPARE(restored.flightPause(), 8);
        QCOMPARE(restored.flightLimit(), 3);
    }
    void sakuraRoundTrip() {
        QTemporaryDir dir; const auto path = dir.filePath("sakura.ini");
        {
            te::Settings settings(path);
            settings.setIntensity(2); settings.setWind(-0.4);
            settings.setEffect(2); settings.setIntensity(0);
            settings.setSakuraAngle(-45); settings.setSakuraBranches(false);
            QVERIFY(settings.flush());
        }
        te::Settings restored(path);
        QCOMPARE(restored.effect(), 2); QCOMPARE(restored.intensity(), 0);
        QCOMPARE(restored.sakuraAngle(), -45.); QVERIFY(!restored.sakuraBranches());
        restored.setEffect(0); QCOMPARE(restored.intensity(), 2);
        QVERIFY(std::abs(restored.wind()+0.4)<0.001);
        restored.setSakuraAngle(900); QCOMPARE(restored.sakuraAngle(), 65.);
        restored.setEffect(99); QCOMPARE(restored.effect(), 0);
    }
    void roundTrip() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("settings.ini");
        {
            te::Settings settings(path);
            settings.setEnabled(false); settings.setIntensity(2); settings.setWind(-0.8);
            settings.setAccumulation(false); settings.setMaxDepth(73);
            settings.setInteraction(false); settings.setFps(30);
            QVERIFY(settings.flush());
        }
        te::Settings restored(path);
        QVERIFY(!restored.enabled()); QCOMPARE(restored.intensity(), 2);
        QVERIFY(std::abs(restored.wind() + 0.8) < 0.0001);
        QVERIFY(!restored.accumulation()); QCOMPARE(restored.maxDepth(), 73.);
        QVERIFY(!restored.interaction()); QCOMPARE(restored.fps(), 30);
    }
    void corruptedValues() {
        QTemporaryDir directory; const auto path = directory.filePath("invalid.ini");
        {
            QSettings raw(path, QSettings::IniFormat);
            raw.setValue("snow/intensity", -100); raw.setValue("snow/wind", 999);
            raw.setValue("snow/maxDepth", 100000); raw.setValue("render/fps", 1000); raw.sync();
        }
        te::Settings settings(path);
        QCOMPARE(settings.intensity(), 0); QCOMPARE(settings.wind(), 1.);
        QCOMPARE(settings.maxDepth(), 160.); QCOMPARE(settings.fps(), 60);
        QSignalSpy spy(&settings, &te::Settings::changed);
        settings.setFps(500); QCOMPARE(spy.count(), 0);
        settings.setFps(30); QCOMPARE(spy.count(), 1);
    }
    void rainAndLegacySettings() {
        QTemporaryDir directory;
        const auto path = directory.filePath("settings.ini");
        {
            QSettings legacy(path, QSettings::IniFormat);
            legacy.setValue("snow/intensity", 2);
            legacy.setValue("snow/maxDepth", 73);
            legacy.setValue("snow/accumulation", false);
            legacy.setValue("snow/wind", -0.4);
        }
        {
            te::Settings settings(path);
            QCOMPARE(settings.effect(), 0);
            QCOMPARE(settings.intensity(), 2);
            QCOMPARE(settings.maxDepth(), 73.);
            settings.setEffect(1);
            QCOMPARE(settings.intensity(), 1);
            settings.setIntensity(0);
            settings.setGrassHeight(95);
            settings.setGrass(false);
            settings.setLightning(true);
            settings.setFlashInterval(25);
            settings.setEffect(0);
            QCOMPARE(settings.intensity(), 2);
            QVERIFY(!settings.accumulation());
            settings.setEffect(1);
            QVERIFY(settings.flush());
        }
        te::Settings restored(path);
        QCOMPARE(restored.effect(), 1);
        QCOMPARE(restored.intensity(), 0);
        QCOMPARE(restored.grassHeight(), 95.);
        QVERIFY(!restored.grass());
        QVERIFY(restored.lightning());
        QCOMPARE(restored.flashInterval(), 25.);
        QVERIFY(std::abs(restored.wind() + 0.4) < 0.0001);
        restored.setEffect(0);
        QCOMPARE(restored.intensity(), 2);
        QCOMPARE(restored.maxDepth(), 73.);
    }
};
QTEST_GUILESS_MAIN(SettingsTests)
#include "settings_tests.moc"
