#pragma once
#include "effects/IEffect.h"
#include <array>
#include <random>

namespace te {
// "8 бит": a pixel hero leaves its home in the lower-right corner and runs the full
// width of the bottom edge two or three times before diving back, while pixel coins fall.
class RetroEffect final : public IEffect {
public:
    explicit RetroEffect(unsigned seed = std::random_device{}());
    void configure(EffectConfig config) override;
    void resize(float width, float height) override;
    void advance(float seconds, CursorSample cursor) override;
    void setLoadScale(float scale) override { load_ = std::clamp(scale, .3F, 1.F); }
    void resetCursor() override {}
    void clear() override;
    bool needsAnimation() const override { return config_.enabled && width_ > 0 && height_ > 0; }
    const EffectFrame& frame() const override { return frame_; }

    enum class Stage { Home, LeavingHome, Running, GoingHome };
    Stage stage() const { return stage_; }
    Vec2 heroFeet() const { return {heroX_, ground() - heroLift_}; }
    float laneEnd() const;   // Left turning point.
    float turnBack() const;  // Right turning point just before home.
    int lapsLeft() const { return laps_; }
    float homeX() const;
    float ground() const { return height_ - 1; }
    static constexpr float pixel = 2;     // Logical points per art pixel for the hero and its home.
    static constexpr float coinPixel = 1.5F;

private:
    struct Coin { Vec2 point; float speed, phase, spin, sway; };
    struct Burst { Vec2 point; float age = -1; Artwork art = Artwork::PixelSparkle; float direction = 1; };
    void startTrip();
    void hop(float speed);
    void land();
    void spawnCoin(Coin& coin, bool anywhere);
    void burst(Artwork art, Vec2 point, float direction = 1);
    void moveHero(float dt, CursorSample cursor);
    void moveCoins(float dt, CursorSample cursor);
    void draw();
    void push(Artwork art, float centerX, float bottomY, float scale, bool mirror = false);
    float range(float lo, float hi);
    Artwork heroFrame() const;
    EffectConfig config_;
    EffectFrame frame_;
    std::vector<Coin> coins_;
    std::array<Burst, 12> bursts_{};
    Stage stage_ = Stage::Home;
    float heroX_ = 0, heroLift_ = 0, heroVx_ = 0, heroVy_ = 0;
    bool airborne_ = false, facingRight_ = false;
    int laps_ = 0;
    float runClock_ = 0, wait_ = 0, hopCooldown_ = 0;
    float width_ = 0, height_ = 0, load_ = 1, time_ = 0;
    std::mt19937 rng_;
};
}
