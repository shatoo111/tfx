#pragma once
#include "effects/IEffect.h"
#include <memory>

namespace te {
std::unique_ptr<IEffect> createEffect(EffectKind kind);
}
