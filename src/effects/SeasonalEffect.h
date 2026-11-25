#pragma once
#include "effects/IEffect.h"
#include <random>
#include <array>

namespace te {
class SeasonalEffect final : public IEffect {
public:
    explicit SeasonalEffect(EffectKind kind, unsigned seed = std::random_device{}());
    void configure(EffectConfig config) override;
    void resize(float width, float height) override;
    void advance(float seconds, CursorSample cursor) override;
    void setLoadScale(float scale) override { load_ = std::clamp(scale, .3F, 1.F); }
    void resetCursor() override {}
    void clear() override;
    bool needsAnimation() const override { return config_.enabled && width_ > 0 && height_ > 0; }
    const EffectFrame& frame() const override { return frame_; }
private:
    struct Leaf {
        Vec2 point;
        float size, phase, frequency, speed, spin, angle;
        Artwork art;
    };
    struct Flight {
        float age = 0, duration = 0;
        Vec2 from{}, to{};
        float phase = 0, length = 0, strength = 0;
        float tailAngle = 0, tailLength = 0;
    };
    void autumn(float dt, CursorSample cursor);
    void stardust(float dt);
    void drawFlights(float dt);
    void startFlight(Flight& flight);
    void fitCometTail(Flight& flight, float laneHeight) const;
    void drawComet(const Flight& flight, int slot, float laneHeight);
    float range(float lo, float hi);
    EffectKind kind_;
    EffectConfig config_;
    EffectFrame frame_;
    std::vector<Leaf> leaves_;
    std::array<Flight,maxFlights> flights_{};
    int groupSize_ = 0;
    float wait_ = 0;
    float width_ = 0, height_ = 0, load_ = 1, time_ = 0;
    std::mt19937 rng_;
};
}
