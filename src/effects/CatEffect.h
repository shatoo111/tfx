#pragma once
#include "effects/IEffect.h"
#include <random>

namespace te {
// A small pixel cat on the bottom edge: it strolls, naps, chases yarn, watches the cursor,
// swats at it and purrs when stroked.
class CatEffect final : public IEffect {
public:
    enum class Mode { Sit, Walk, Chase, Sleep };
    explicit CatEffect(unsigned seed = std::random_device{}());
    void configure(EffectConfig config) override;
    void resize(float width, float height) override;
    void advance(float seconds, CursorSample cursor) override;
    void setLoadScale(float) override {}
    void resetCursor() override { tracked_ = false; }
    void clear() override;
    bool needsAnimation() const override { return config_.enabled && width_ > 0 && height_ > 0; }
    const EffectFrame& frame() const override { return frame_; }
    Mode mode() const { return mode_; }
    float x() const { return x_; }
    float ground() const { return height_ - 1; }
    float purr() const { return purr_; }
    bool swatting() const { return swat_ > 0; }
    std::size_t heartCount() const { return hearts_.size(); }
    std::size_t ballCount() const { return balls_.size(); }
    Vec2 ball(std::size_t i) const { return {balls_[i].x, ground() - balls_[i].lift - 8}; }
    void setMode(Mode mode); // Poses the cat for previews and tests.
private:
    struct Ball { float x = 0, lift = 0, vx = 0, vy = 0, turn = 0; int color = 0; };
    struct Heart { Vec2 point; float age, drift; };
    void moveBalls(float dt, CursorSample cursor, Vec2 velocity);
    void think(float dt, CursorSample cursor, float cursorSpeed);
    void draw();
    float range(float lo, float hi);
    EffectConfig config_;
    EffectFrame frame_;
    std::vector<Ball> balls_;
    std::vector<Heart> hearts_;
    Mode mode_ = Mode::Sit;
    float width_ = 0, height_ = 0, time_ = 0;
    float x_ = 0, target_ = 0, facing_ = -1, stride_ = 0, modeAge_ = 0, rest_ = 4, idle_ = 0;
    float blink_ = 0, nextBlink_ = 2, purr_ = 0, heartWait_ = 0, swat_ = 0, swatWait_ = 0, boredom_ = 0, batWait_ = 0;
    int bats_ = 0, chased_ = -1;
    float swatSide_ = 1;
    Vec2 previous_{};
    bool tracked_ = false;
    std::mt19937 rng_;
};
}
