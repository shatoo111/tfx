#include "effects/EffectFactory.h"
#include "effects/AquariumEffect.h"
#include "effects/BlackHoleEffect.h"
#include "effects/CatEffect.h"
#include "effects/ParadeEffect.h"
#include "effects/RainEffect.h"
#include "effects/RetroEffect.h"
#include "effects/SnowEffect.h"
#include "effects/SakuraEffect.h"
#include "effects/SeasonalEffect.h"

namespace te {
std::unique_ptr<IEffect> createEffect(EffectKind kind) {
    switch (kind) {
    case EffectKind::Leaves:
    case EffectKind::Stardust: return std::make_unique<SeasonalEffect>(kind);
    case EffectKind::UnitedStates: return std::make_unique<ParadeEffect>();
    case EffectKind::Retro: return std::make_unique<RetroEffect>();
    case EffectKind::BlackHole: return std::make_unique<BlackHoleEffect>();
    case EffectKind::Cat: return std::make_unique<CatEffect>();
    case EffectKind::Aquarium: return std::make_unique<AquariumEffect>();
    case EffectKind::Sakura: return std::make_unique<SakuraEffect>();
    case EffectKind::Rain: return std::make_unique<RainEffect>();
    case EffectKind::Snow: return std::make_unique<SnowEffect>();
    }
    return std::make_unique<SnowEffect>();
}
}
