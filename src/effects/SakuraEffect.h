#pragma once
#include "effects/IEffect.h"
#include <random>

namespace te {
class SakuraEffect final : public IEffect {
public:
    explicit SakuraEffect(std::uint32_t seed = std::random_device{}());
    void configure(EffectConfig config) override;
    void resize(float width, float height) override;
    void advance(float seconds, CursorSample cursor) override;
    void setLoadScale(float scale) override;
    void resetCursor() override {} // Repulsion depends on position, never stale velocity.
    void clear() override;
    bool needsAnimation() const override { return config_.enabled && width_ > 0 && height_ > 0; }
    const EffectFrame& frame() const override { return frame_; }
private:
    struct Phase {
        explicit Phase(float angle) : sine(std::sin(angle)), cosine(std::cos(angle)) {}
        float sineWith(Phase offset) const {
            return sine * offset.cosine + cosine * offset.sine;
        }
        float sine, cosine;
    };
    struct Petal { Vec2 position, impulse; float radius; Phase phase; float speed, rotation, shade; };
    struct BranchSpritePose { Vec2 center; float angle; };
    float random(float min, float max);
    void spawn();
    void publish();
    void rebuildBranches();
    float branchSway(bool right) const;
    Vec2 branchPoint(Vec2 center, float corner, Phase sway) const;
    EffectConfig config_;
    EffectFrame frame_;
    std::vector<BranchSpritePose> branchPoses_;
    std::vector<Petal> petals_;
    std::mt19937 random_;
    bool primed_ = false;
    float width_ = 0, height_ = 0, time_ = 0, credit_ = 0, load_ = 1;
};
}
