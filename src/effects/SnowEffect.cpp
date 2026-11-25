#include "effects/SnowEffect.h"
#include "effects/VectorCanvas.h"
#include <array>
#include <numbers>

namespace te {
SnowEffect::SnowEffect(std::uint32_t seed) : rng_(seed) {
    particles_.reserve(particleLimit);
    frame_.particles.reserve(particleLimit);
}
float SnowEffect::random(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng_); }
void SnowEffect::configure(EffectConfig config) {
    config.normalize();
    config_ = config;
    field_.setMaximum(std::min(config_.maxDepth, height_ * 0.25F));
    publish();
}
void SnowEffect::setLoadScale(float scale) {
    loadScale_ = std::isfinite(scale) ? std::clamp(scale, 0.3F, 1.F) : 1.F;
}
void SnowEffect::resize(float width, float height) {
    width = std::isfinite(width) ? std::clamp(width, 0.F, 32768.F) : 0;
    height = std::isfinite(height) ? std::clamp(height, 0.F, 32768.F) : 0;
    if (width_ > 0 && height_ > 0) {
        for (auto& p : particles_) { p.position.x *= width / width_; p.position.y *= height / height_; }
    }
    width_ = width; height_ = height;
    field_.resize(width);
    festive_.resize(width, height);
    field_.setMaximum(std::min(config_.maxDepth, height * 0.25F));
    resetCursor();
    publish();
}
std::size_t SnowEffect::targetCount() const {
    constexpr float densities[] = {68, 155, 440};
    const float area = width_ * height_ / 720000.F;
    return std::size_t(std::clamp(densities[config_.intensity] * area * loadScale_, 0.F, float(particleLimit - 100)));
}
SnowEffect::Particle SnowEffect::spawn(bool distributed) {
    const float depth = random(0, 1);
    const Vec2 position{random(0, width_), distributed ? random(0, height_) : -8.F};
    const float phase = random(0, 2 * std::numbers::pi_v<float>);
    return {position,
        std::lerp(0.75F, 3.5F, depth * depth),
        std::lerp(22.F, 87.F, depth) * (config_.intensity == 2 ? 1.55F : 1.F),
        random(6, 22), random(0.4F, 0.88F), 0, 120,
        std::sin(phase), std::cos(phase), std::sin(phase * 2.F), std::cos(phase * 2.F),
        {}, false, false};
}
void SnowEffect::advance(float seconds, CursorSample cursor) {
    if (!std::isfinite(seconds) || seconds <= 0 || width_ <= 0 || height_ <= 0) return;
    seconds = std::min(seconds, 0.1F); // No catch-up storm on wake / debugger resume.
    if (!std::isfinite(cursor.position.x) || !std::isfinite(cursor.position.y)) cursor.inside = false;
    if (!config_.interaction) cursor.inside = false;
    Vec2 velocity{};
    if (cursor.inside && previousCursor_.inside) {
        velocity.x = std::clamp((cursor.position.x - previousCursor_.position.x) / seconds, -2200.F, 2200.F);
        velocity.y = std::clamp((cursor.position.y - previousCursor_.position.y) / seconds, -2200.F, 2200.F);
    }
    previousCursor_ = cursor;
    if (!primed_ && config_.enabled) {
        const auto target = targetCount();
        for (std::size_t i = 0; i < target; ++i) particles_.push_back(spawn(true));
        primed_ = true;
    }
    const int steps = std::max(1, int(std::ceil(seconds * 120.F)));
    const float dt = seconds / float(steps);
    const float cursorSpeed = cursor.inside ? std::hypot(velocity.x, velocity.y) : 0.F;
    for (int i = 0; i < steps; ++i) step(dt, cursor, velocity, cursorSpeed);
    if (config_.festive) festive_.advance(seconds, cursor);
    publish(seconds);
}
void SnowEffect::step(float dt, CursorSample cursor, Vec2 velocity, float cursorSpeed) {
    time_ = std::fmod(time_ + dt, 10000.F);
    if (cursorSpeed > 25.F && !field_.empty() &&
        cursor.position.y > height_ - field_.heightAt(cursor.position.x) - 20.F) {
        const float removed = field_.sweep(cursor.position.x, 30, dt * std::min(cursorSpeed * 0.23F, 180.F));
        if (removed > 0) textureAge_ = 1;
        if (removed > 0.02F && particles_.size() < particleLimit) {
            auto p = spawn(false);
            p.position = {cursor.position.x + random(-20, 20), height_ - field_.heightAt(cursor.position.x) - random(2, 8)};
            p.impulse = {velocity.x * 0.1F + random(-65, 65), random(-155, -65)};
            p.radius = random(1, 2.4F); p.debris = true; p.age = 0.3F; p.lifetime = random(0.5F, 1.4F);
            particles_.push_back(p);
        }
    }
    std::size_t retained = 0;
    const auto target = targetCount();
    const float primary = time_ * 0.75F, secondary = time_ * 1.81F;
    const float primarySin = std::sin(primary), primaryCos = std::cos(primary);
    const float secondarySin = std::sin(secondary), secondaryCos = std::cos(secondary);
    const float flakeDamping = std::exp(-dt * 2.8F), debrisDamping = std::exp(-dt * 1.2F);
    const float highestSurface = height_ - (config_.accumulation ? std::min(config_.maxDepth, height_ * 0.25F) : 0.F);
    const float cursorForce = std::min(cursorSpeed, 800.F) * 3.F;
    for (auto& p : particles_) {
        p.age += dt;
        if (!p.debris && !p.retiring && ++retained > target) p.retiring = true;
        if (!config_.enabled || p.retiring) p.opacity = std::max(0.F, p.opacity - dt * 1.5F);
        if (cursorSpeed > 10.F) {
            const float dx = p.position.x - cursor.position.x;
            const float dy = p.position.y - cursor.position.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (distanceSquared < 76.F * 76.F && distanceSquared > 0.25F) {
                const float distance = std::sqrt(distanceSquared);
                const float impulse = (1.F - distance / 76.F) * cursorForce * dt / distance;
                p.impulse.x += dx * impulse;
                p.impulse.y += dy * impulse * 0.3F;
            }
        }
        p.impulse.x = std::clamp(p.impulse.x, -300.F, 300.F);
        p.impulse.y = std::clamp(p.impulse.y, -220.F, 220.F);
        // Expand sin(time + phase) using terms shared across the whole particle batch.
        const float drift = config_.wind * (55 + p.speed)
            + (primarySin * p.cosPhase + primaryCos * p.sinPhase) * p.sway
            + (secondarySin * p.cosDoublePhase + secondaryCos * p.sinDoublePhase) * 3.F;
        p.position.x += (drift + p.impulse.x) * dt;
        p.position.y += (p.speed + p.impulse.y) * dt;
        const float damping = p.debris ? debrisDamping : flakeDamping;
        p.impulse.x *= damping; p.impulse.y *= damping;
        if (p.position.x < -8) p.position.x += width_ + 16;
        if (p.position.x > width_ + 8) p.position.x -= width_ + 16;
        if (p.position.y + p.radius < highestSurface) continue;
        const float surface = config_.accumulation ? field_.heightAt(p.position.x) : 0;
        if (p.position.y + p.radius >= height_ - surface) {
            if (config_.enabled && config_.accumulation && !p.debris && !p.retiring)
                field_.deposit(p.position.x, p.radius * 0.17F);
            p.opacity = 0;
        }
    }
    std::erase_if(particles_, [](const Particle& p) { return p.opacity <= 0 || p.age >= p.lifetime; });
    if (config_.enabled) {
        const auto living = std::count_if(particles_.begin(), particles_.end(), [](const Particle& p) { return !p.debris && !p.retiring; });
        const auto missing = target > std::size_t(living) ? target - std::size_t(living) : 0;
        const float travelTime = std::max(1.F, height_ / (config_.intensity == 2 ? 85.F : 55.F));
        emission_ = std::min(emission_ + dt * std::max(8.F, float(target) / travelTime * 1.35F), float(target));
        const auto count = std::min({missing, std::size_t(emission_), particleLimit - particles_.size()});
        for (std::size_t i = 0; i < count; ++i) particles_.push_back(spawn(false));
        emission_ -= float(count);
    } else { emission_ = 0; }
    field_.advance(dt, !config_.enabled || !config_.accumulation);
}
void SnowEffect::publish(float seconds) {
    frame_.particles.clear();
    for (const auto& p : particles_) {
        const float fadeIn = std::clamp(p.age / 0.75F, 0.F, 1.F);
        const float fadeOut = p.debris ? std::clamp((p.lifetime - p.age) / 0.4F, 0.F, 1.F) : 1.F;
        frame_.particles.push_back({p.position, p.radius, p.opacity * fadeIn * fadeOut});
    }
    frame_.snow.clear();
    if (!field_.empty()) {
        const auto values = field_.heights();
        const float spacing = field_.spacing();
        frame_.snow.reserve(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) frame_.snow.push_back({float(i) * spacing, values[i]});
    }
    textureAge_ += seconds;
    if (textureAge_ < 1.F / 15.F && !frame_.snow.empty()) return;
    textureAge_ = 0;
    ++frame_.textureVersion;
    frame_.underlay.clear(); frame_.detail.clear();
    if (frame_.snow.empty()) return;
    const auto depth = [this](float x) { return field_.heightAt(x); };
    const bool festive = config_.enabled && config_.festive;
    if (festive) festive_.drawFigures(frame_.underlay, depth);
    drawSnowbank();
    if (festive) festive_.drawLights(frame_.detail, depth);
}
// Packed snow: a bluish body with wind-blown layers, a fluffy crest, grains, hollows and glints.
// Every mark is anchored to the window, so fresh snow buries it instead of dragging it along.
void SnowEffect::drawSnowbank() {
    VectorCanvas canvas(frame_.detail);
    const float step = std::clamp(width_ / 160, 5.F, 14.F);
    const int columns = int(std::ceil(width_ / step));
    const auto wobble = [](float x, float seed) {
        return .5F * std::sin(x * .021F + seed) + .32F * std::sin(x * .057F + seed * 2.3F) + .18F * std::sin(x * .13F + seed * 4.1F);
    };
    // Layers: each one starts deeper than the previous and is a little bluer.
    constexpr std::array<float, 3> layers{.2F, .48F, .74F};
    for (std::size_t layer = 0; layer < layers.size(); ++layer) {
        const Paint paint = rgb(0x7fa8d6, .13F + .05F * float(layer)), crest = rgb(0xffffff, .42F);
        Vec2 last{};
        for (int i = 0; i <= columns; ++i) {
            const float x = std::min(width_, float(i) * step), h = field_.heightAt(x);
            const float y = height_ - h * (1 - layers[layer]) + h * .1F * wobble(x, 1.7F + 2.1F * float(layer));
            const Vec2 p{x, std::clamp(y, height_ - h + std::min(2.F, h), height_)};
            if (i > 0 && h > 2) {
                canvas.quad(last, paint, p, paint, {p.x, height_}, paint, {last.x, height_}, paint);
                // A lit lip along the wind-facing parts of the layer.
                const float lit = std::clamp((last.y - p.y) * .35F + .3F + .5F * wobble(x, 6 + float(layer)), 0.F, 1.F);
                if (h > 7 && lit > .05F) canvas.stroke({last, p}, 1.1F, 1.1F, faded(crest, lit), false);
            }
            last = p;
        }
    }
    // Fluffy crest: overlapping tufts of different size.
    const float tuft = std::clamp(width_ / 150, 6.F, 11.F);
    for (int i = 0; float(i) * tuft < width_ + tuft; ++i) {
        const float x = (float(i) + (hash01(i, 1) - .5F) * .9F) * tuft, h = field_.heightAt(x);
        if (h < 1.2F) continue;
        const float grow = std::min(1.F, h / 9), r = (1.7F + 3.1F * hash01(i, 2)) * grow;
        canvas.ellipse({x, height_ - h + r * .42F}, r * 1.45F, r, rgb(0xffffff, .97F), rgb(0xf3f9ff, .92F));
    }
    // Grains, hollows and glints on a fixed grid inside the bank.
    const float cell = std::max(7.F, std::sqrt(width_ * std::max(8.F, config_.maxDepth) / 950));
    const int columnsOfGrain = int(width_ / cell) + 1;
    for (int cx = 0; cx < columnsOfGrain; ++cx) {
        const float top = field_.heightAt((float(cx) + .5F) * cell);
        for (int cy = 0; float(cy) * cell < top; ++cy) {
            const float x = (float(cx) + hash01(cx, cy, 1)) * cell, rise = (float(cy) + hash01(cx, cy, 2)) * cell;
            const float cover = field_.heightAt(x) - rise; // Snow above this mark.
            if (cover < 2.5F || rise < 1) continue;
            const float show = std::min(1.F, (cover - 2.5F) / 3), y = height_ - rise, pick = hash01(cx, cy, 3);
            if (pick < .34F) {
                canvas.speck({x, y}, 1.9F + 1.9F * hash01(cx, cy, 4), 1, rgb(0x7ba3d0, .72F * show));
                canvas.speck({x + .3F, y + 1.2F}, 1.7F, .7F, rgb(0xffffff, .75F * show));
            } else if (pick < .8F) {
                const float r = .85F + .8F * hash01(cx, cy, 5);
                canvas.speck({x, y}, r, r, rgb(0xffffff, show));
            } else {
                const float twinkle = std::pow(.5F + .5F * std::sin(time_ * (1.1F + hash01(cx, cy, 6)) + 6.3F * hash01(cx, cy, 7)), 7.F);
                const float r = (1.1F + 1.7F * hash01(cx, cy, 8)) * (.5F + .5F * twinkle), k = r * .22F;
                if (twinkle < .06F) { canvas.speck({x, y}, r * .6F, r * .6F, rgb(0xffffff, .5F * show)); continue; }
                canvas.polygon({{x, y - r}, {x + k, y - k}, {x + r, y}, {x + k, y + k}, {x, y + r}, {x - k, y + k}, {x - r, y}, {x - k, y - k}},
                               rgb(0xffffff, show * (.25F + .75F * twinkle)));
            }
        }
    }
}
void SnowEffect::settle(float depth) {
    if (width_ <= 0 || !config_.accumulation) return;
    for (int pass = 0; pass < 400 && field_.volume() < depth * width_; ++pass)
        for (float x = float(pass % 3) * 4; x <= width_; x += 12) field_.deposit(x, .35F + .4F * hash01(pass, int(x)));
    publish();
}
bool SnowEffect::needsAnimation() const { return config_.enabled || !particles_.empty() || !field_.empty(); }
void SnowEffect::clear() { field_.clear(); publish(); }
}
