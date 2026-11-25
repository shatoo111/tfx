#include "renderer/IllustratedPreview.h"
#include "effects/AquariumEffect.h"
#include "effects/BlackHoleEffect.h"
#include "effects/CatEffect.h"
#include "effects/ParadeEffect.h"
#include "effects/RainEffect.h"
#include "effects/RetroEffect.h"
#include "effects/SeasonalEffect.h"
#include "effects/SnowEffect.h"

namespace te {
namespace {
bool previewable(int theme) {
    return theme == int(EffectKind::Snow) || theme == int(EffectKind::Rain) || (theme >= int(EffectKind::Leaves) && theme <= int(EffectKind::Aquarium));
}
// A fixed seed keeps the sample recognisable.
std::unique_ptr<IEffect> sample(EffectKind kind) {
    switch (kind) {
    case EffectKind::Snow: return std::make_unique<SnowEffect>(42);
    case EffectKind::Rain: return std::make_unique<RainEffect>(42);
    case EffectKind::UnitedStates: return std::make_unique<ParadeEffect>(42);
    case EffectKind::Retro: return std::make_unique<RetroEffect>(42);
    case EffectKind::BlackHole: return std::make_unique<BlackHoleEffect>(42);
    case EffectKind::Cat: return std::make_unique<CatEffect>(42);
    case EffectKind::Aquarium: return std::make_unique<AquariumEffect>(42);
    default: return std::make_unique<SeasonalEffect>(kind,42);
    }
}
// Frames played ahead so the sample opens on something worth seeing.
int warmup(EffectKind kind) {
    switch (kind) {
    case EffectKind::Snow:
    case EffectKind::Rain: return 20;
    case EffectKind::UnitedStates: return 62;
    case EffectKind::Stardust: return 44;
    case EffectKind::Retro: return 30;
    case EffectKind::BlackHole: return 45;
    case EffectKind::Aquarium: return 30;
    default: return 1;
    }
}
}
void IllustratedPreview::geometryChange(const QRectF& geometry, const QRectF& old) {
    QQuickItem::geometryChange(geometry,old);
    if (item_) item_->setSize(geometry.size());
}
void IllustratedPreview::configure(int theme, int intensity, double wind, int flightPause, int flightLimit, int hero,
                                   int coat, bool festive, bool accumulation, double depth,
                                   bool lightning, double flashInterval) {
    if (width() <= 0 || height() <= 0 || !previewable(theme)) return;
    EffectConfig config;
    config.kind = static_cast<EffectKind>(theme);
    config.ambientIntensity = config.intensity = config.rainIntensity = intensity;
    config.flightLimit = flightLimit;
    config.flightPause = flightPause;
    config.wind = float(wind);
    config.retroHero = hero;
    config.catCoat = coat;
    config.festive = festive;
    config.accumulation = accumulation;
    config.maxDepth = config.grassHeight = float(depth);
    config.grass = accumulation;
    config.lightning = lightning;
    config.flashInterval = float(flashInterval);
    const bool switched = theme != theme_;
    if (switched) {
        auto next = sample(config.kind);
        if (item_) item_->setEffect(*next);
        else item_ = new EffectItem(*next,this);
        item_->setSize(size());
        next->resize(float(width()),float(height()));
        effect_ = std::move(next);
        theme_ = theme;
    }
    effect_->configure(config);
    if (switched) {
        // Show the rare motif immediately, including when motion is disabled.
        const int frames = warmup(config.kind);
        if (config.kind == EffectKind::Snow) static_cast<SnowEffect&>(*effect_).settle(config.maxDepth * .35F);
        for (int i=0;i<frames;++i) effect_->advance(.1F,{});
    }
    item_->advance(0.F, {});
}

void IllustratedPreview::advance(double seconds, QPointF cursor) {
    if (item_) item_->advance(float(seconds), {{float(cursor.x()),float(cursor.y())}, contains(cursor)});
}
}
