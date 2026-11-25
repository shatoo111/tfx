#include "settings/Settings.h"

namespace te {
namespace {
// On disk, 4 is retired, 5 remains stardust, 6 is the United States and 7 is 8-bit;
// later themes continue from 8. UI indices stay contiguous.
constexpr int usaProfile = 6, retroProfile = 7;
int profileId(EffectKind kind) {
    if (kind == EffectKind::UnitedStates) return usaProfile;
    if (kind >= EffectKind::Retro) return retroProfile + int(kind) - int(EffectKind::Retro);
    return int(kind);
}
EffectKind savedKind(int id) {
    if (id == usaProfile) return EffectKind::UnitedStates;
    const int later = id - retroProfile + int(EffectKind::Retro);
    if (id >= retroProfile && later <= int(EffectKind::Aquarium)) return static_cast<EffectKind>(later);
    if (id == 4 || id < 0 || id > int(EffectKind::Stardust)) return EffectKind::Snow;
    return static_cast<EffectKind>(id);
}
}
Settings::Settings(const QString& path, QObject* parent) : QObject(parent) {
    if (path.isEmpty()) store_ = std::make_unique<QSettings>();
    else store_ = std::make_unique<QSettings>(path, QSettings::IniFormat);
    store_->remove("themes/4");
    config_.enabled = store_->value("effect/enabled", true).toBool();
    config_.kind = savedKind(store_->value("effect/kind", 0).toInt());
    config_.intensity = store_->value("snow/intensity", 1).toInt();
    config_.wind = store_->value("snow/wind", 0.15).toFloat();
    config_.accumulation = store_->value("snow/accumulation", true).toBool();
    config_.maxDepth = store_->value("snow/maxDepth", 48).toFloat();
    config_.interaction = store_->value("snow/interaction", true).toBool();
    config_.fps = store_->value("render/fps", 60).toInt();
    config_.rainIntensity = store_->value("rain/intensity", 1).toInt();
    config_.grass = store_->value("rain/grass", true).toBool();
    config_.grassHeight = store_->value("rain/grassHeight", 48).toFloat();
    config_.lightning = store_->value("rain/lightning", false).toBool();
    config_.flashInterval = store_->value("rain/flashInterval", 15).toFloat();
    config_.sakuraIntensity = store_->value("sakura/intensity", 1).toInt();
    config_.sakuraAngle = store_->value("sakura/angle", 15).toFloat();
    config_.sakuraBranches = store_->value("sakura/branches", true).toBool();
    config_.branchSide = store_->value("sakura/side", config_.branchSide).toInt();
    config_.branchWidth = store_->value("sakura/width", 78).toFloat();
    config_.branchOpacity = store_->value("sakura/opacity", 1).toFloat();
    if (config_.branchOpacity == 0.F) {
        // Restore the original decoration for profiles saved with invisible branches.
        config_.branchWidth = EffectConfig{}.branchWidth;
        config_.branchOpacity = EffectConfig{}.branchOpacity;
    }
    config_.economy = store_->value("render/economy", false).toBool();
    config_.retroHero = store_->value("retro/hero", 0).toInt();
    config_.catCoat = store_->value("cat/coat", 0).toInt();
    config_.festive = store_->value("snow/festive", true).toBool();
    config_.normalize();
    loadProfile();
    saveTimer_.setSingleShot(true); saveTimer_.setInterval(250);
    connect(&saveTimer_, &QTimer::timeout, this, [this] { flush(); });
}
Settings::~Settings() { flush(); }
void Settings::modified() { saveTimer_.start(); emit changed(); }
void Settings::setEnabled(bool value) { if (config_.enabled != value) { config_.enabled = value; modified(); } }
void Settings::setEffect(int value) {
    const auto kind = value >= 0 && value <= int(EffectKind::Aquarium) ? EffectKind(value) : EffectKind::Snow;
    if (config_.kind != kind) {
        saveProfile();
        config_.kind = kind;
        loadProfile();
        modified();
    }
}
void Settings::setIntensity(int value) {
    value = std::clamp(value, 0, 2);
    auto& intensity = config_.kind >= EffectKind::Leaves ? config_.ambientIntensity : config_.kind == EffectKind::Sakura ? config_.sakuraIntensity : config_.kind == EffectKind::Rain ? config_.rainIntensity : config_.intensity;
    if (intensity != value) { intensity = value; modified(); }
}
void Settings::setFlightPause(int seconds) {
    seconds = std::clamp(seconds, 2, 60);
    if (seconds != config_.flightPause) { config_.flightPause = seconds; modified(); }
}
void Settings::setFlightLimit(int limit) {
    limit = std::clamp(limit, 1, maxFlights);
    if (limit != config_.flightLimit) { config_.flightLimit = limit; modified(); }
}
void Settings::setRetroHero(int hero) {
    hero = std::clamp(hero, 0, 2);
    if (hero != config_.retroHero) { config_.retroHero = hero; modified(); }
}
void Settings::setCatCoat(int coat) {
    coat = std::clamp(coat, 0, 3);
    if (coat != config_.catCoat) { config_.catCoat = coat; modified(); }
}
void Settings::setFestive(bool value) {
    if (config_.festive != value) { config_.festive = value; modified(); }
}
void Settings::setWind(double value) {
    const float normalized = std::isfinite(value) ? float(std::clamp(value, -1., 1.)) : 0.F;
    if (config_.wind != normalized) { config_.wind = normalized; modified(); }
}
void Settings::setAccumulation(bool value) {
    if (config_.accumulation != value) { config_.accumulation = value; modified(); }
}
void Settings::setMaxDepth(double value) {
    const float normalized = std::isfinite(value) ? float(std::clamp(value, 0., 160.)) : 48.F;
    if (config_.maxDepth != normalized) { config_.maxDepth = normalized; modified(); }
}
void Settings::setInteraction(bool value) {
    if (config_.interaction != value) { config_.interaction = value; modified(); }
}
void Settings::setFps(int value) {
    value = value == 30 ? 30 : 60;
    if (config_.fps != value) { config_.fps = value; modified(); }
}
void Settings::setGrass(bool value) {
    if (config_.grass != value) { config_.grass = value; modified(); }
}
void Settings::setGrassHeight(double value) {
    const float normalized = std::isfinite(value) ? float(std::clamp(value, 0., 160.)) : 48.F;
    if (config_.grassHeight != normalized) { config_.grassHeight = normalized; modified(); }
}
void Settings::setLightning(bool value) {
    if (config_.lightning != value) { config_.lightning = value; modified(); }
}
void Settings::setFlashInterval(double value) {
    const float normalized = std::isfinite(value) ? float(std::clamp(value, 5., 60.)) : 15.F;
    if (config_.flashInterval != normalized) { config_.flashInterval = normalized; modified(); }
}
void Settings::setSakuraAngle(double value) {
    const float angle = std::isfinite(value) ? float(std::clamp(value, -65., 65.)) : 15.F;
    if (config_.sakuraAngle != angle) { config_.sakuraAngle = angle; modified(); }
}
void Settings::setSakuraBranches(bool value) {
    if (config_.sakuraBranches != value) { config_.sakuraBranches = value; modified(); }
}
void Settings::saveProfile() {
    const auto key = QStringLiteral("themes/%1/").arg(profileId(config_.kind));
    if (config_.kind == EffectKind::UnitedStates || config_.kind == EffectKind::Stardust || config_.kind == EffectKind::Retro
        || config_.kind == EffectKind::BlackHole) {
        store_->setValue(key + "flightPause", config_.flightPause);
        store_->setValue(key + "flightLimit", config_.flightLimit);
    }
    store_->setValue(key + "wind", config_.wind);
    store_->setValue(key + "fps", config_.fps);
    store_->setValue(key + "interaction", config_.interaction);
    store_->setValue(key + "intensity", config_.ambientIntensity);
}
void Settings::loadProfile() {
    const auto key = QStringLiteral("themes/%1/").arg(profileId(config_.kind));
    config_.wind = store_->value(key + "wind", store_->value("snow/wind", 0.15)).toFloat();
    config_.fps = store_->value(key + "fps", store_->value("render/fps", 60)).toInt();
    config_.interaction = store_->value(key + "interaction", store_->value("snow/interaction", true)).toBool();
    const bool parade = config_.kind == EffectKind::UnitedStates;
    const bool retro = config_.kind == EffectKind::Retro;
    config_.flightPause = store_->value(key + "flightPause", parade ? 12 : retro ? 6 : config_.kind == EffectKind::BlackHole ? 12 : 17).toInt();
    config_.flightLimit = store_->value(key + "flightLimit", 2).toInt();
    config_.ambientIntensity = store_->value(key + "intensity", 1).toInt();
    config_.normalize();
}
void Settings::setEconomy(bool value) {
    if (config_.economy != value) { config_.economy = value; modified(); }
}
void Settings::setBranchSide(int value) { config_.branchSide = std::clamp(value, 0, 2); modified(); }
void Settings::setBranchWidth(double value) { config_.branchWidth = float(value); config_.normalize(); modified(); }
void Settings::setBranchOpacity(double value) { config_.branchOpacity = float(value); config_.normalize(); modified(); }
void Settings::applyPreset(int preset) {
    if (preset < 0 || preset > 2) return;
    setIntensity(preset);
    config_.fps = preset == 0 ? 30 : 60;
    config_.wind = preset == 0 ? 0.05F : preset == 1 ? 0.15F : 0.45F;
    if (config_.kind == EffectKind::Sakura) {
        config_.branchOpacity = 0.4F + 0.3F*preset;
        config_.branchSide = preset == 2 ? 2 : 1;
    }
    modified();
}
bool Settings::flush() {
    saveProfile();
    store_->setValue("sakura/side", config_.branchSide);
    store_->setValue("sakura/width", config_.branchWidth);
    store_->setValue("sakura/opacity", config_.branchOpacity);
    store_->setValue("render/economy", config_.economy);
    store_->setValue("retro/hero", config_.retroHero);
    store_->setValue("cat/coat", config_.catCoat);
    store_->setValue("snow/festive", config_.festive);
    saveTimer_.stop();
    store_->setValue("effect/enabled", config_.enabled);
    store_->setValue("effect/kind", profileId(config_.kind));
    store_->setValue("snow/intensity", config_.intensity);
    store_->setValue("snow/accumulation", config_.accumulation);
    store_->setValue("snow/maxDepth", config_.maxDepth);
    store_->setValue("rain/intensity", config_.rainIntensity);
    store_->setValue("rain/grass", config_.grass);
    store_->setValue("rain/grassHeight", config_.grassHeight);
    store_->setValue("rain/lightning", config_.lightning);
    store_->setValue("rain/flashInterval", config_.flashInterval);
    store_->setValue("sakura/intensity", config_.sakuraIntensity);
    store_->setValue("sakura/angle", config_.sakuraAngle);
    store_->setValue("sakura/branches", config_.sakuraBranches);
    store_->sync();
    if (store_->status() != QSettings::NoError) {
        emit saveFailed(QStringLiteral("Не удалось сохранить настройки. Проверьте доступ к пользовательскому каталогу."));
        return false;
    }
    return true;
}
}
