#pragma once
#include "effects/IEffect.h"

namespace te {
class SnowField final : public ICollisionSurface {
public:
    void resize(float width);
    void setMaximum(float maximum);
    float heightAt(float x) const override;
    void deposit(float x, float amount) override;
    float sweep(float x, float radius, float strength);
    void advance(float dt, bool melting);
    void clear();
    bool empty() const;
    float volume() const;
    std::span<const float> heights() const { return heights_; }
    float spacing() const;
private:
    float localMaximum(std::size_t index) const;
    float width_ = 0;
    float maximum_ = 48;
    bool hasSnow_ = false;
    std::vector<float> heights_, scratch_, ceilingFactors_;
};
}
