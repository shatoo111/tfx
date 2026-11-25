#include "effects/RainEffect.h"
#include <numbers>

namespace te {
namespace {
constexpr float rainMargin = 22.F;
constexpr float windSlope = 0.65F;
constexpr float bladeSpacing = 7.F;
constexpr float splashDuration = 0.14F;
constexpr Vec2 splashMotion{-22.F, -28.F};
}

RainEffect::RainEffect(std::uint32_t seed) : rng_(seed) {
    drops_.reserve(dropLimit);
    blades_.reserve(bladeLimit);
    splashes_.reserve(splashLimit);
    frame_.rain.reserve(dropLimit);
    frame_.splashes.reserve(splashLimit);
    frame_.grass.reserve(bladeLimit);
}
float RainEffect::random(float low, float high) {
    return std::uniform_real_distribution<float>(low, high)(rng_);
}
void RainEffect::configure(EffectConfig config) {
    config.normalize();
    if (config.lightning != config_.lightning || config.flashInterval != config_.flashInterval
        || config.enabled != config_.enabled) {
        flashWait_ = config.flashInterval * random(0.75F, 1.25F);
        flashAge_ = 1;
    }
    config_ = config;
    publish();
}
void RainEffect::resize(float width, float height) {
    width = std::isfinite(width) ? std::clamp(width, 0.F, 32768.F) : 0;
    height = std::isfinite(height) ? std::clamp(height, 0.F, 32768.F) : 0;
    if (width == width_ && height == height_) return;
    if (width_ > 0 && height_ > 0) {
        for (auto& drop : drops_) {
            drop.position.x *= width / width_;
            drop.position.y *= height / height_;
        }
    }
    width_ = width;
    height_ = height;
    blades_.clear();
    const auto count = std::min(bladeLimit, std::size_t(std::ceil(width / bladeSpacing)));
    const float spacing = std::max(bladeSpacing, width / float(bladeLimit));
    for (std::size_t i = 0; i < count; ++i) {
        const auto blade = float(i);
        const float x = blade * spacing + std::sin(blade * 8.3F) * 2.F;
        const float phase = x * 0.08F;
        blades_.push_back({x, 0.35F + 0.65F * std::abs(std::sin(blade * 1.7F)),
            std::sin(blade * 2.4F) * 6.F, std::sin(phase), std::cos(phase)});
    }
    splashes_.clear();
    flashAge_ = 1;
    resetCursor();
    publish();
}
void RainEffect::setLoadScale(float scale) {
    loadScale_ = std::isfinite(scale) ? std::clamp(scale, 0.3F, 1.F) : 1.F;
}
std::size_t RainEffect::targetCount() const {
    // Same drops per logical area as the 626 × 174 settings preview.
    constexpr float density[] = {14, 28, 52};
    return std::size_t(std::clamp(width_ * height_ / (626.F * 174.F)
        * density[config_.rainIntensity] * loadScale_, 1.F, float(dropLimit)));
}
float RainEffect::grassDepth() const {
    return std::min(height_ * 0.3F, config_.grassHeight * 0.35F);
}
RainEffect::Drop RainEffect::spawn(bool distributed) {
    const bool highlight = random(0, 3) < 1;
    return {{random(-rainMargin, width_ + rainMargin), distributed ? random(-rainMargin, height_) : -rainMargin},
        190.F + float(int(random(0, 7))) * 23.F, (0.9F + float(int(random(0, 5))) * 0.45F) * 1.6F,
        highlight ? 179.F / 255.F : 121.F / 255.F, highlight};
}
void RainEffect::advance(float seconds, CursorSample cursor) {
    if (!std::isfinite(seconds) || seconds <= 0 || width_ <= 0 || height_ <= 0) return;
    seconds = std::min(seconds, 0.1F);
    cursor.inside = cursor.inside && config_.interaction
        && std::isfinite(cursor.position.x) && std::isfinite(cursor.position.y);
    Vec2 velocity{};
    if (cursor.inside && previousCursor_.inside) {
        velocity.x = std::clamp((cursor.position.x - previousCursor_.position.x) / seconds, -2200.F, 2200.F);
        velocity.y = std::clamp((cursor.position.y - previousCursor_.position.y) / seconds, -2200.F, 2200.F);
    }
    previousCursor_ = cursor;
    if (!primed_ && config_.enabled) {
        const auto target = targetCount();
        for (std::size_t i = 0; i < target; ++i) drops_.push_back(spawn(true));
        primed_ = true;
    }
    const int steps = std::max(1, int(std::ceil(seconds * 120.F)));
    for (int i = 0; i < steps; ++i) step(seconds / float(steps), cursor, velocity);
    publish();
}
void RainEffect::step(float dt, CursorSample cursor, Vec2 cursorVelocity) {
    time_ += dt;
    if (time_ >= 10000.F) time_ -= 10000.F;
    const float grassTarget = config_.enabled && config_.grass ? 1.F : 0.F;
    grassOpacity_ += std::clamp(grassTarget - grassOpacity_, -dt * 2.F, dt * 2.F);
    const float maximum = grassDepth();
    const bool grassVisible = grassOpacity_ > 0.F && maximum > 0.F;
    const bool nearGrass = cursor.inside && cursor.position.y > height_ - maximum - 35.F;
    if (grassVisible) {
        for (auto& blade : blades_) {
            float push = 0;
            const float distance = std::abs(cursor.position.x - blade.x);
            if (nearGrass && distance < 65.F) {
                const float direction = blade.x < cursor.position.x ? -1.F : 1.F;
                push = (cursorVelocity.x * 0.32F + std::abs(cursorVelocity.y) * 0.16F * direction)
                    * (1.F - distance / 65.F);
            }
            blade.velocity += (-blade.bend * 35.F - blade.velocity * 6.F + push) * dt;
            blade.bend = std::clamp(blade.bend + blade.velocity * dt, -30.F, 30.F);
        }
    }
    const auto target = config_.enabled ? targetCount() : 0;
    const float slope = config_.wind * windSlope;
    const float inverseSpacing = 1.F / std::max(bladeSpacing, width_ / float(bladeLimit));
    const float surfaceDepth = config_.grass ? maximum * 0.72F : 0.F;
    const float highestBlade = height_ - surfaceDepth;
    for (auto& splash : splashes_) splash.age += dt;
    std::erase_if(splashes_, [](const Splash& splash) { return splash.age >= splashDuration; });
    for (std::size_t i = 0; i < drops_.size(); ++i) {
        auto& drop = drops_[i];
        if (i >= target) drop.opacity = std::max(0.F, drop.opacity - dt * 0.9F);
        drop.position.x += slope * drop.speed * dt;
        drop.position.y += drop.speed * dt;
        if (drop.position.x < -rainMargin) drop.position.x += width_ + 2.F * rainMargin;
        if (drop.position.x > width_ + rainMargin) drop.position.x -= width_ + 2.F * rainMargin;
        if (drop.position.y < highestBlade) continue;
        std::size_t index = 0;
        float surface = 0.F;
        if (surfaceDepth > 0.F && !blades_.empty()) {
            index = std::min(blades_.size() - 1, std::size_t(std::max(0.F, drop.position.x * inverseSpacing + 0.5F)));
            surface = surfaceDepth * blades_[index].height;
        }
        const float impactY = height_ - surface;
        if (drop.position.y >= impactY) {
            if (config_.enabled && i < target) {
                if (surface > 0) {
                    const auto first = index > 0 ? index - 1 : index;
                    const auto last = std::min(index + 1, blades_.size() - 1);
                    for (auto hit = first; hit <= last; ++hit) {
                        auto& blade = blades_[hit];
                        const float direction = drop.position.x < blade.x ? 1.F : -1.F;
                        const float strength = hit == index ? 1.F : 0.30F;
                        blade.velocity += (13.F * direction + config_.wind * 9.F) * strength;
                    }
                }
                if (surface > 0 && splashes_.size() < splashLimit) {
                    const float age = std::clamp((drop.position.y - impactY) / drop.speed, 0.F, dt);
                    const float impactX = drop.position.x - slope * drop.speed * age;
                    if (impactX >= 0.F && impactX < width_)
                        splashes_.push_back({{impactX, impactY}, age});
                }
                drop = spawn(false);
            } else drop.opacity = 0;
        }
    }
    if (drops_.size() > target)
        std::erase_if(drops_, [](const Drop& drop) { return drop.opacity <= 0; });
    if (drops_.size() < target) {
        const auto count = std::min(target - drops_.size(), std::size_t(8));
        for (std::size_t i = 0; i < count; ++i) drops_.push_back(spawn(false));
    }
    updateLightning(dt);
}
void RainEffect::updateLightning(float dt) {
    if (!config_.enabled || !config_.lightning) { flashAge_ = 1; return; }
    flashAge_ += dt;
    flashWait_ -= dt;
    if (flashWait_ > 0) return;
    flashWait_ = config_.flashInterval * random(0.75F, 1.25F);
    flashAge_ = 0;
}
void RainEffect::publish() {
    frame_.rain.clear();
    const float slope = config_.wind * windSlope;
    const float vertical = 1.F / std::sqrt(1.F + slope * slope);
    const Vec2 direction{slope * vertical, vertical};
    for (const auto& drop : drops_) {
        frame_.rain.push_back({drop.position, direction, drop.size, drop.opacity, drop.highlight});
    }
    frame_.splashes.clear();
    for (const auto& splash : splashes_)
        frame_.splashes.push_back({{splash.impact.x + splashMotion.x * splash.age,
                                   splash.impact.y + splashMotion.y * splash.age},
                                  (154.F / 255.F) * (1.F - splash.age / splashDuration)});
    frame_.grass.clear();
    const float maximum = grassDepth();
    if (grassOpacity_ > 0 && maximum > 0) {
        const float wave = time_ * 2.4F;
        const float windLean = config_.wind * 12.F;
        const float sinWave = 4.F * std::sin(wave), cosWave = 4.F * std::cos(wave);
        for (std::size_t i = 0; i < blades_.size(); ++i) {
            const auto& blade = blades_[i];
            const float lean = windLean + blade.sinPhase * cosWave + blade.cosPhase * sinWave
                + blade.lean + blade.bend;
            frame_.grass.push_back({{blade.x, height_}, {blade.x + lean, height_ - maximum * blade.height},
                grassOpacity_, i % 3});
        }
    }
    frame_.flashes.clear();
    if (config_.lightning && config_.enabled && flashAge_ < 0.7F) {
        // Match the preview's cool, broad glow with a smooth rise and fall.
        const float fade = std::sin(std::numbers::pi_v<float> * flashAge_ / 0.7F);
        frame_.flashes.push_back({{width_ * 0.71F, 0.F}, std::max(width_ * 0.65F, height_),
                                  0.12F * fade * fade});
    }
}
bool RainEffect::needsAnimation() const {
    return config_.enabled || !drops_.empty() || !splashes_.empty() || grassOpacity_ > 0;
}
void RainEffect::clear() {
    for (auto& blade : blades_) { blade.bend = 0; blade.velocity = 0; }
    splashes_.clear();
    flashAge_ = 1;
    resetCursor();
    publish();
}
}
