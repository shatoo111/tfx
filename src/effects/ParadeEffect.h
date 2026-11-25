#pragma once
#include "effects/IEffect.h"
#include <random>

namespace te {
// The United States: a small Fourth of July parade along the bottom edge. Guests cross the strip
// one after another, greet the cursor and answer a press with fireworks. The Statue of Liberty
// stands in the right corner, a flag waves in the left one.
class ParadeEffect final : public IEffect {
public:
    explicit ParadeEffect(unsigned seed = std::random_device{}());
    void configure(EffectConfig config) override;
    void resize(float width, float height) override;
    void advance(float seconds, CursorSample cursor) override;
    void setLoadScale(float) override {}
    bool pressed(Vec2 position) override;
    bool pressable() const override { return config_.interaction && !guests_.empty(); }
    void resetCursor() override {}
    void clear() override;
    bool needsAnimation() const override { return config_.enabled && width_ > 0 && height_ > 0; }
    const EffectFrame& frame() const override { return frame_; }
    std::size_t guestCount() const { return guests_.size(); }
    Vec2 guestPoint(std::size_t i) const;
    float guestCheer(std::size_t i) const { return guests_[i].cheer; }
    std::size_t rocketCount() const { return rockets_.size(); }
    std::size_t confettiCount() const { return confetti_.size(); }
private:
    enum class Kind { Eagle, Truck, Rider, Astronaut, HotDog };
    struct Guest { Kind kind; float x, direction, speed, stride, lift, climb, cheer, boost; };
    struct Rocket { Vec2 point; float top, age; int tint; };
    struct Confetto { Vec2 point, velocity; float age; int tint; };
    void parade(float dt);
    void invite(int count);
    void greet(Guest& guest);
    void launch(float x);
    void shower(Vec2 point, int count);
    void draw();
    float range(float lo, float hi);
    EffectConfig config_;
    EffectFrame frame_;
    std::vector<Guest> guests_;
    std::vector<Rocket> rockets_;
    std::vector<Confetto> confetti_;
    float width_ = 0, height_ = 0, time_ = 0, wait_ = 0, salute_ = 0;
    int next_ = 0;
    std::mt19937 rng_;
};
}
