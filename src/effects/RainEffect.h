#pragma once
#include "effects/IEffect.h"
#include <random>

namespace te {
class RainEffect final : public IEffect {
public:
    static constexpr std::size_t dropLimit = 960;
    static constexpr std::size_t bladeLimit = 300;
    static constexpr std::size_t splashLimit = 48;
    explicit RainEffect(std::uint32_t seed = std::random_device{}());
    void configure(EffectConfig config) override;
    void resize(float width, float height) override;
    void advance(float seconds, CursorSample cursor) override;
    void setLoadScale(float scale) override;
    void resetCursor() override { previousCursor_.inside = false; }
    void clear() override;
    bool needsAnimation() const override;
    const EffectFrame& frame() const override { return frame_; }
private:
    struct Drop { Vec2 position; float speed, size, opacity; bool highlight; };
    struct Blade {
        float x, height, lean, sinPhase, cosPhase;
        float bend = 0, velocity = 0;
    };
    struct Splash { Vec2 impact; float age; };
    float random(float low, float high);
    std::size_t targetCount() const;
    float grassDepth() const;
    Drop spawn(bool distributed);
    void step(float dt, CursorSample cursor, Vec2 cursorVelocity);
    void updateLightning(float dt);
    void publish();
    EffectConfig config_;
    std::mt19937 rng_;
    std::vector<Drop> drops_;
    std::vector<Blade> blades_;
    std::vector<Splash> splashes_;
    EffectFrame frame_;
    CursorSample previousCursor_;
    float width_ = 0, height_ = 0, time_ = 0, loadScale_ = 1;
    float grassOpacity_ = 0, flashWait_ = 15, flashAge_ = 1;
    bool primed_ = false;
};
}
