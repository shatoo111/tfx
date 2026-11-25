#pragma once
#include "effects/IEffect.h"
#include <random>

namespace te {
// A tiny pixel black hole hovers in the lower-right corner. Stars and small travellers drift
// along the bottom edge into it; a cursor resting on it feeds the hole until it bursts.
class BlackHoleEffect final : public IEffect {
public:
    explicit BlackHoleEffect(unsigned seed = std::random_device{}());
    void configure(EffectConfig config) override;
    void resize(float width, float height) override;
    void advance(float seconds, CursorSample cursor) override;
    void setLoadScale(float scale) override { load_ = std::clamp(scale, .3F, 1.F); }
    void resetCursor() override {}
    void clear() override;
    bool needsAnimation() const override { return config_.enabled && width_ > 0 && height_ > 0; }
    const EffectFrame& frame() const override { return frame_; }
    Vec2 center() const { return {width_ - 46, height_ - 21 - (int(time_ * 1.2F) % 2 ? 2.F : 0.F)}; }
    float charge() const { return charge_; }
    bool erupting() const { return burst_ > 0; }
    bool travelling() const { return travel_ >= 0; }
    std::size_t starCount() const { return stars_.size(); }
private:
    struct Star { Vec2 point; float speed, alpha; int tint; };
    struct Spark { Vec2 point, velocity; float age, life; int tint; };
    void spawn(Star& star, bool anywhere);
    void erupt(float power);
    void draw();
    void drawHole(PixelCanvas& canvas, bool front) const;
    float range(float lo, float hi);
    EffectConfig config_;
    EffectFrame frame_;
    std::vector<Star> stars_;
    std::vector<Spark> sparks_;
    float width_ = 0, height_ = 0, load_ = 1, time_ = 0, spin_ = 0;
    float charge_ = 0, full_ = 0, burst_ = 0, travel_ = -1, wait_ = 2, lane_ = 0;
    int traveller_ = 0;
    std::mt19937 rng_;
};
}
