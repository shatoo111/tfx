#pragma once
#include "effects/IEffect.h"
#include <QObject>
#include <QSettings>
#include <QTimer>
#include <memory>

namespace te {
class Settings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(int effect READ effect WRITE setEffect NOTIFY changed)
    Q_PROPERTY(int intensity READ intensity WRITE setIntensity NOTIFY changed)
    Q_PROPERTY(int flightPause READ flightPause WRITE setFlightPause NOTIFY changed)
    Q_PROPERTY(int flightLimit READ flightLimit WRITE setFlightLimit NOTIFY changed)
    Q_PROPERTY(int retroHero READ retroHero WRITE setRetroHero NOTIFY changed)
    Q_PROPERTY(int catCoat READ catCoat WRITE setCatCoat NOTIFY changed)
    Q_PROPERTY(bool festive READ festive WRITE setFestive NOTIFY changed)
    Q_PROPERTY(double wind READ wind WRITE setWind NOTIFY changed)
    Q_PROPERTY(bool accumulation READ accumulation WRITE setAccumulation NOTIFY changed)
    Q_PROPERTY(double maxDepth READ maxDepth WRITE setMaxDepth NOTIFY changed)
    Q_PROPERTY(bool interaction READ interaction WRITE setInteraction NOTIFY changed)
    Q_PROPERTY(int fps READ fps WRITE setFps NOTIFY changed)
    Q_PROPERTY(double sakuraAngle READ sakuraAngle WRITE setSakuraAngle NOTIFY changed)
    Q_PROPERTY(bool sakuraBranches READ sakuraBranches WRITE setSakuraBranches NOTIFY changed)
    Q_PROPERTY(bool economy READ economy WRITE setEconomy NOTIFY changed)
    Q_PROPERTY(int branchSide READ branchSide WRITE setBranchSide NOTIFY changed)
    Q_PROPERTY(double branchWidth READ branchWidth WRITE setBranchWidth NOTIFY changed)
    Q_PROPERTY(double minBranchOpacity READ minBranchOpacity CONSTANT)
    Q_PROPERTY(double branchOpacity READ branchOpacity WRITE setBranchOpacity NOTIFY changed)
    Q_PROPERTY(bool grass READ grass WRITE setGrass NOTIFY changed)
    Q_PROPERTY(double grassHeight READ grassHeight WRITE setGrassHeight NOTIFY changed)
    Q_PROPERTY(bool lightning READ lightning WRITE setLightning NOTIFY changed)
    Q_PROPERTY(double flashInterval READ flashInterval WRITE setFlashInterval NOTIFY changed)
public:
    explicit Settings(const QString& iniPath = {}, QObject* parent = nullptr);
    ~Settings() override;
    EffectConfig config() const { return config_; }
    bool enabled() const { return config_.enabled; }
    int effect() const { return static_cast<int>(config_.kind); }
    int intensity() const { return config_.kind >= EffectKind::Leaves ? config_.ambientIntensity : config_.kind == EffectKind::Sakura ? config_.sakuraIntensity : config_.kind == EffectKind::Rain ? config_.rainIntensity : config_.intensity; }
    double sakuraAngle() const { return config_.sakuraAngle; }
    bool sakuraBranches() const { return config_.sakuraBranches; }
    void setSakuraAngle(double value);
    void setSakuraBranches(bool value);
    int flightPause() const { return config_.flightPause; }
    int flightLimit() const { return config_.flightLimit; }
    void setFlightPause(int seconds);
    void setFlightLimit(int limit);
    int retroHero() const { return config_.retroHero; }
    void setRetroHero(int hero);
    int catCoat() const { return config_.catCoat; }
    void setCatCoat(int coat);
    bool festive() const { return config_.festive; }
    void setFestive(bool value);
    double wind() const { return config_.wind; }
    bool accumulation() const { return config_.accumulation; }
    double maxDepth() const { return config_.maxDepth; }
    bool interaction() const { return config_.interaction; }
    int fps() const { return config_.fps; }
    bool grass() const { return config_.grass; }
    double grassHeight() const { return config_.grassHeight; }
    bool lightning() const { return config_.lightning; }
    double flashInterval() const { return config_.flashInterval; }
    bool economy() const { return config_.economy; }
    int branchSide() const { return config_.branchSide; }
    double branchWidth() const { return config_.branchWidth; }
    double minBranchOpacity() const { return EffectConfig::minBranchOpacity; }
    double branchOpacity() const { return config_.branchOpacity; }
    void setEconomy(bool value);
    void setBranchSide(int value);
    void setBranchWidth(double value);
    void setBranchOpacity(double value);
    Q_INVOKABLE void applyPreset(int preset);
    void setEffect(int value);
    void setEnabled(bool value);
    void setIntensity(int value);
    void setWind(double value);
    void setAccumulation(bool value);
    void setMaxDepth(double value);
    void setInteraction(bool value);
    void setFps(int value);
    void setGrass(bool value);
    void setGrassHeight(double value);
    void setLightning(bool value);
    void setFlashInterval(double value);
    bool flush();
signals:
    void changed();
    void saveFailed(const QString& message);
private:
    void modified();
    void loadProfile();
    void saveProfile();
    EffectConfig config_;
    std::unique_ptr<QSettings> store_;
    QTimer saveTimer_;
};
}
