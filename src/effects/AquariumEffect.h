#pragma once
#include "effects/IEffect.h"
#include <random>

namespace te {
// A few small pixel sea creatures along the bottom edge: fish pass by and flee, a puffer inflates,
// a crab patrols and a chest in the corner opens under the cursor. The chat itself stays dry.
class AquariumEffect final : public IEffect {
public:
    explicit AquariumEffect(unsigned seed = std::random_device{}());
    void configure(EffectConfig config) override;
    void resize(float width, float height) override;
    void advance(float seconds, CursorSample cursor) override;
    void setLoadScale(float) override {}
    void resetCursor() override { tracked_ = false; }
    void clear() override;
    bool needsAnimation() const override { return config_.enabled && width_ > 0 && height_ > 0; }
    const EffectFrame& frame() const override { return frame_; }
    std::size_t fishCount() const { return fish_.size(); }
    Vec2 fishPoint(std::size_t i) const { return fish_[i].point; }
    float puffed() const;
    Vec2 pufferPoint() const;
    float chestOpen() const { return chest_; }
    Vec2 chestPoint() const { return {width_ - 40, height_ - 9}; }
    std::size_t bubbleCount() const { return bubbles_.size(); }
private:
    enum class Kind { Clown, Tang, Puffer, Minnow };
    struct Fish { Kind kind; Vec2 point; float speed, boost, direction, lane, phase, puff; int tint; };
    struct Bubble { Vec2 point; float speed, phase, age; bool big; };
    void populate();
    void bubble(Vec2 point);
    void draw();
    float range(float lo, float hi);
    EffectConfig config_;
    EffectFrame frame_;
    std::vector<Fish> fish_;
    std::vector<Bubble> bubbles_;
    float width_ = 0, height_ = 0, time_ = 0, chest_ = 0, chestBubble_ = 0, stream_ = 0;
    float crab_ = 0, crabDirection_ = 1, crabStep_ = 0, crabRest_ = 0;
    Vec2 previous_{};
    bool tracked_ = false;
    std::mt19937 rng_;
};
}
