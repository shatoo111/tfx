#pragma once
#include "effects/FestiveDecor.h"
#include "effects/IEffect.h"
#include "effects/SnowField.h"
#include <random>

namespace te {
class SnowEffect final : public IEffect {
public:
    static constexpr std::size_t particleLimit = 1400;
    explicit SnowEffect(std::uint32_t seed = std::random_device{}());
    void configure(EffectConfig config) override;
    void resize(float width, float height) override;
    void advance(float seconds, CursorSample cursor) override;
    void setLoadScale(float scale) override;
    void resetCursor() override { previousCursor_.inside = false; }
    void clear() override;
    void settle(float depth); // Lays snow at once, for previews.
    bool needsAnimation() const override;
    const EffectFrame& frame() const override { return frame_; }
private:
    struct Particle {
        Vec2 position;
        float radius, speed, sway, opacity, age, lifetime;
        float sinPhase, cosPhase, sinDoublePhase, cosDoublePhase;
        Vec2 impulse;
        bool debris = false;
        bool retiring = false;
    };
    float random(float lo, float hi);
    Particle spawn(bool distributed);
    void step(float dt, CursorSample cursor, Vec2 velocity, float cursorSpeed);
    void publish(float seconds = 1);
    void drawSnowbank();
    std::size_t targetCount() const;
    EffectConfig config_;
    std::mt19937 rng_;
    std::vector<Particle> particles_;
    SnowField field_;
    FestiveDecor festive_;
    EffectFrame frame_;
    float textureAge_ = 1; // The bank's texture is redrawn fifteen times a second, or at once when swept.
    CursorSample previousCursor_;
    float width_ = 0, height_ = 0, time_ = 0, emission_ = 0, loadScale_ = 1;
    bool primed_ = false;
};
}
