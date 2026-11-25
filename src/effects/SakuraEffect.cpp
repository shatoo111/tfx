#include "effects/SakuraEffect.h"
#include <numbers>

namespace te {
namespace {
constexpr float pi = std::numbers::pi_v<float>;
constexpr std::size_t petalLimit = 180;
constexpr float cursorRadius = 65.F;
constexpr float artWidth = 240.F, artHeight = 170.F;
struct Bloom {
    Artwork art;
    Vec2 center;
    float size, angle;
};
using enum Artwork;
// Coordinates are the tips of the ink twigs in the matching 240 × 170 SVG.
constexpr Bloom leftBlooms[] = {
    {CherryBloom,{78,26},23,-.2F}, {CherryRose,{106,43},22,.42F},
    {CherryBud,{119,12},14,.42F}, {CherryBud,{120,18},12,-.32F},
    {CherryRose,{71,132},26,-.58F}, {CherryBloom,{96,122},18,.47F},
    {CherryBud,{57,111},13,-.62F}, {CherryBud,{119,114},12,-.4F},
    {CherryRose,{128,144},30,.2F}, {CherryBloom,{160,143},24,.74F},
    {CherryBloom,{161,40},31,-.32F}, {CherryRose,{180,80},19,.35F},
    {CherryRose,{207,57},27,.11F}, {CherryBud,{214,73},12,.51F},
    {CherryBloom,{182,137},22,-.43F}, {CherryRose,{219,106},25,.21F},
    {CherryBud,{229,87},12,.63F}, {CherryBud,{218,119},13,-.27F}
};
constexpr Bloom rightBlooms[] = {
    {CherryRose,{72,18},23,-.32F}, {CherryBloom,{106,21},21,.25F},
    {CherryBud,{117,10},14,.5F}, {CherryBud,{49,21},12,-.45F},
    {CherryBloom,{83,90},24,.05F}, {CherryRose,{142,149},26,.48F},
    {CherryBud,{113,154},12,-.35F}, {CherryBud,{150,120},17,.55F},
    {CherryBloom,{161,27},27,.11F}, {CherryRose,{196,55},23,-.2F},
    {CherryBud,{203,35},14,.48F}, {CherryBud,{221,82},13,.58F},
    {CherryRose,{189,126},24,-.31F}, {CherryBloom,{217,126},25,.29F},
    {CherryBud,{227,115},12,.58F}
};
}
SakuraEffect::SakuraEffect(std::uint32_t seed) : random_(seed) {
    petals_.reserve(petalLimit);
    frame_.petals.reserve(petalLimit);
}
float SakuraEffect::random(float min, float max) {
    return std::uniform_real_distribution<float>(min, max)(random_);
}
void SakuraEffect::configure(EffectConfig config) {
    config.normalize();
    const bool branchesChanged = config.enabled != config_.enabled
        || config.sakuraBranches != config_.sakuraBranches || config.branchSide != config_.branchSide
        || config.branchWidth != config_.branchWidth || config.branchOpacity != config_.branchOpacity;
    config_ = config;
    if (branchesChanged) rebuildBranches();
    publish();
}
void SakuraEffect::resize(float width, float height) {
    const float w = std::isfinite(width) ? std::clamp(width, 0.F, 16384.F) : 0;
    const float h = std::isfinite(height) ? std::clamp(height, 0.F, 16384.F) : 0;
    if (w == width_ && h == height_) return;
    if (width_ > 0 && height_ > 0) {
        const float scaleX = w / width_, scaleY = h / height_;
        for (auto& p : petals_) { p.position.x *= scaleX; p.position.y *= scaleY; }
    }
    width_ = w; height_ = h;
    if (w == 0 || h == 0) { petals_.clear(); primed_ = false; }
    rebuildBranches();
    publish();
}
void SakuraEffect::setLoadScale(float scale) {
    load_ = std::isfinite(scale) ? std::clamp(scale, 0.3F, 1.F) : 1.F;
}
void SakuraEffect::clear() { petals_.clear(); credit_ = 0; primed_ = false; publish(); }
float SakuraEffect::branchSway(bool right) const {
    return .015F * std::sin(time_ * .82F + (right ? 1.3F : 0.F));
}
Vec2 SakuraEffect::branchPoint(Vec2 center, float corner, Phase sway) const {
    const float x = center.x - corner;
    return {corner + x * sway.cosine - center.y * sway.sine,
            x * sway.sine + center.y * sway.cosine};
}
void SakuraEffect::spawn() {
    const Vec2 start{random(0, width_), -10.F};
    petals_.push_back({start, {}, random(3.5F, 6.F), Phase(random(0, 2*pi)),
        random(22, 38), random(-pi, pi), random(0, 1)});
}
void SakuraEffect::advance(float seconds, CursorSample cursor) {
    if (!needsAnimation() || !std::isfinite(seconds) || seconds <= 0) return;
    const float rates[] = {3.F, 6.F, 10.F};
    const float rate = rates[config_.sakuraIntensity] * std::clamp(width_ / 900.F, 0.4F, 2.F) * load_;
    const auto cap = std::size_t(petalLimit * load_);
    const float tangent = std::tan(config_.sakuraAngle * pi / 180.F);
    if (!primed_) {
        // Seed different stages of the fall so the opening population has no single exit time.
        const float drift = std::abs(tangent) * 30.F;
        const float fallTime = height_ / 30.F;
        const float lifetime = fallTime / (1.F + fallTime * drift / width_);
        const auto count = std::clamp(std::size_t(std::round(rate * lifetime)), std::size_t(1), cap);
        for (std::size_t i = 0; i < count; ++i) {
            spawn();
            petals_.back().position.y = height_ * (float(i) + random(0.15F, 0.85F)) / float(count);
        }
        primed_ = true;
    }
    // Substeps keep cursor response and trajectories stable at both 30 and 60 fps.
    const bool repel = config_.interaction && cursor.inside
        && std::isfinite(cursor.position.x) && std::isfinite(cursor.position.y);
    float remaining = std::min(seconds, 0.1F);
    while (remaining > 0.00001F) {
        const float dt = std::min(remaining, 1.F / 120.F); remaining -= dt; time_ += dt;
        credit_ += dt * rate;
        while (credit_ >= 1) { if (petals_.size() < cap) spawn(); credit_ -= 1; }
        const float damping = std::exp(-2.4F * dt);
        const Phase drift(time_ * 1.2F), bob(time_ * .8F), spin(time_);
        for (auto& p : petals_) {
            if (repel) {
                const float dx = p.position.x - cursor.position.x, dy = p.position.y - cursor.position.y;
                const float distanceSquared = dx * dx + dy * dy;
                if (distanceSquared < cursorRadius * cursorRadius) {
                    const float distance = std::sqrt(distanceSquared);
                    const float force = (1 - distance / cursorRadius) * 220.F;
                    p.impulse.x += (distance > 0.01F ? dx / distance : 1.F) * force * dt;
                    p.impulse.y += (distance > 0.01F ? dy / distance : 0.F) * force * dt;
                }
            }
            p.impulse.x *= damping; p.impulse.y *= damping;
            p.position.x += (p.speed * tangent + p.phase.sineWith(drift) * 9.F + p.impulse.x) * dt;
            p.position.y += (p.speed + p.phase.sineWith(bob) * 4.F + p.impulse.y) * dt;
            p.rotation += dt * (0.22F + p.phase.sineWith(spin) * 0.3F);
        }
        std::erase_if(petals_, [&](const Petal& p) {
            return p.position.y > height_ + 14 || p.position.y < -80 || p.position.x < -80 || p.position.x > width_ + 80;
        });
    }
    publish();
}
void SakuraEffect::rebuildBranches() {
    frame_.illustrations.clear();
    branchPoses_.clear();
    if (!config_.enabled || !config_.sakuraBranches || width_ <= 0 || height_ <= 0) return;
    const float fit = std::min(width_ * 0.25F / artWidth, height_ / (3.F * artHeight));
    const float adjustment = std::sqrt((config_.branchWidth - 24.F) / (160.F - 24.F));
    const float scale = fit * std::lerp(0.48F, 1.F, adjustment);
    const auto draw = [&](Artwork branch, float corner, float direction, float size, std::span<const Bloom> blooms) {
        frame_.illustrations.push_back({branch, corner, 0.F, direction * artWidth * size,
            artHeight * size, 0.F, 0.F, 0.F, config_.branchOpacity});
        branchPoses_.push_back({{corner, 0.F}, 0.F});
        for (const auto& bloom : blooms) {
            const Vec2 center{corner + direction * bloom.center.x * size, bloom.center.y * size};
            const float diameter = bloom.size * size;
            const bool bud = bloom.art == CherryBud;
            frame_.illustrations.push_back({bloom.art, center.x, center.y,
                diameter * (bud ? 0.8F : 1.F), diameter, 0.5F, 0.5F,
                direction * bloom.angle, config_.branchOpacity});
            branchPoses_.push_back({center, direction * bloom.angle});
        }
    };
    if (config_.branchSide != 1) draw(SakuraLeft, 0.F, 1.F, scale, leftBlooms);
    if (config_.branchSide != 0) draw(SakuraRight, width_, -1.F, scale * 0.85F, rightBlooms);
}
void SakuraEffect::publish() {
    frame_.petals.clear();
    if (!config_.enabled) return;
    if (!branchPoses_.empty()) {
        const float angle[] = {branchSway(false), branchSway(true)};
        const Phase sway[] = {Phase(angle[0]), Phase(angle[1])};
        for (std::size_t i = 0; i < branchPoses_.size(); ++i) {
            const auto& rest = branchPoses_[i];
            const bool right = rest.center.x > width_ * .5F;
            const auto position = branchPoint(rest.center, right ? width_ : 0.F, sway[right]);
            auto& sprite = frame_.illustrations[i];
            sprite.x = position.x;
            sprite.y = position.y;
            sprite.angle = rest.angle + angle[right];
        }
    }
    const Phase flutter(time_ * 1.4F);
    for (const auto& p : petals_) {
        const float fade = std::clamp((height_ + 8 - p.position.y) / 24.F, 0.F, 1.F);
        frame_.petals.push_back({p.position, p.radius, p.rotation,
            0.55F + 0.45F * std::abs(p.phase.sineWith(flutter)), p.shade, 0.85F * fade});
    }
}
}
