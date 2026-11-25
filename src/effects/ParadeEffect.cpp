#include "effects/ParadeEffect.h"
#include "effects/SceneSprites.h"
#include <cmath>

namespace te {
namespace {
constexpr float pixel = PixelCanvas::pixel;
constexpr float burstReach = 11; // Fireworks open inside the strip.
constexpr std::array<std::array<std::uint32_t, 2>, 3> sparks{{{0xff5a5f, 0xffb3b5}, {0xffffff, 0xdfe9ff}, {0x4f8dff, 0xa9c8ff}}};
}
ParadeEffect::ParadeEffect(unsigned seed) : rng_(seed) { clear(); }
float ParadeEffect::range(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng_); }
void ParadeEffect::configure(EffectConfig config) {
    config.normalize();
    if (time_ > 0 && config.flightPause != config_.flightPause) wait_ *= float(config.flightPause) / float(config_.flightPause);
    config_ = config;
    if (!config_.enabled) clear();
}
void ParadeEffect::resize(float w, float h) {
    if (w == width_ && h == height_) return;
    if (width_ > 0 && w > 0) {
        for (auto& g : guests_) g.x *= w / width_;
        for (auto& r : rockets_) { r.point.x *= w / width_; r.point.y += h - height_; r.top += h - height_; }
        for (auto& c : confetti_) { c.point.x *= w / width_; c.point.y += h - height_; }
    }
    width_ = w; height_ = h;
}
void ParadeEffect::clear() {
    frame_.overlay.clear();
    guests_.clear(); rockets_.clear(); confetti_.clear();
    time_ = 0;
    wait_ = range(1, 2.5F);
    salute_ = range(1, 3);
    next_ = std::uniform_int_distribution<int>(0, 4)(rng_);
}
Vec2 ParadeEffect::guestPoint(std::size_t i) const {
    const auto& g = guests_[i];
    return {g.x, height_ - (g.kind == Kind::Eagle ? 30 : 13) - g.lift};
}
void ParadeEffect::invite(int count) {
    // A little column walks in from one side; the wind tells which.
    const float direction = range(0, 1) < .5F + .5F * config_.wind ? 1.F : -1.F;
    constexpr std::array paces{74.F, 58.F, 66.F, 34.F, 40.F};
    for (int i = 0; i < count; ++i) {
        const int which = (next_ + i) % 5;
        const float start = direction > 0 ? -34 - float(i) * 76 : width_ + 34 + float(i) * 76;
        guests_.push_back({Kind(which), start, direction, paces[std::size_t(which)], range(0, 2), 0, 0, 0, 0});
    }
    next_ = (next_ + count) % 5;
}
void ParadeEffect::shower(Vec2 point, int count) {
    for (int i = 0; i < count; ++i) {
        if (confetti_.size() >= 48) return;
        confetti_.push_back({point, {range(-64, 64), range(-70, -24)}, 0, std::uniform_int_distribution<int>(0, 3)(rng_)});
    }
}
void ParadeEffect::launch(float x) {
    if (rockets_.size() < 10) rockets_.push_back({{std::clamp(x, 14.F, std::max(14.F, width_ - 14)), height_ - 3}, height_ - range(25, edgeStrip - burstReach - 3), 0, std::uniform_int_distribution<int>(0, 2)(rng_)});
}
void ParadeEffect::greet(Guest& guest) {
    if (guest.cheer > 0) return;
    guest.cheer = 1.5F;
    guest.climb = guest.kind == Kind::Astronaut ? 46.F : guest.kind == Kind::Truck ? 34.F : 52.F;
    const float low = guest.kind == Kind::Eagle ? 26.F : 24.F;
    shower({guest.x, height_ - low}, 5);
}
bool ParadeEffect::pressed(Vec2 position) {
    if (!config_.interaction || !needsAnimation()) return false;
    for (std::size_t i = 0; i < guests_.size(); ++i) {
        const auto point = guestPoint(i);
        if (std::abs(position.x - point.x) > 26 || std::abs(position.y - point.y) > 18) continue;
        auto& g = guests_[i];
        if (g.boost > 0) return false;
        g.boost = 2;
        launch(g.x - 26); launch(g.x); launch(g.x + 26);
        shower(point, 9);
        return true;
    }
    return false;
}
void ParadeEffect::advance(float seconds, CursorSample cursor) {
    if (!needsAnimation()) { clear(); return; }
    const float dt = std::clamp(std::isfinite(seconds) ? seconds : 0.F, 0.F, .1F);
    if (!config_.interaction || !std::isfinite(cursor.position.x) || !std::isfinite(cursor.position.y)) cursor.inside = false;
    time_ += dt;
    if (cursor.inside) {
        for (std::size_t i = 0; i < guests_.size(); ++i) {
            const auto point = guestPoint(i);
            if (std::abs(cursor.position.x - point.x) < 24 && std::abs(cursor.position.y - point.y) < 18) greet(guests_[i]);
        }
    }
    parade(dt);
    // The salute along the edge, more often when there are guests.
    salute_ -= dt * (guests_.empty() ? .45F : 1.F) * (.5F + .6F * float(config_.ambientIntensity));
    if (salute_ <= 0) {
        launch(range(60, std::max(61.F, width_ - 70)));
        salute_ = range(2.2F, 5);
    }
    for (auto& r : rockets_) {
        if (r.point.y > r.top) r.point.y = std::max(r.top, r.point.y - 62 * dt);
        else r.age += dt;
    }
    std::erase_if(rockets_, [&](const Rocket& r) { return r.age > .75F; });
    for (auto& c : confetti_) {
        c.age += dt;
        c.velocity.y += 150 * dt;
        c.point.x += (c.velocity.x + config_.wind * 12) * dt;
        c.point.y = std::max(height_ - edgeStrip + 8, c.point.y + c.velocity.y * dt);
    }
    std::erase_if(confetti_, [&](const Confetto& c) { return c.age > 1.2F || c.point.y > height_ - 2 || c.point.x < -20 || c.point.x > width_ + 20; });
    draw();
}
void ParadeEffect::parade(float dt) {
    for (auto& g : guests_) {
        g.cheer = std::max(0.F, g.cheer - dt);
        g.boost = std::max(0.F, g.boost - dt);
        const float pace = g.speed * (g.boost > 0 ? 2.2F : g.cheer > .8F && g.kind != Kind::Eagle ? .55F : 1.F);
        g.x += g.direction * pace * dt;
        g.stride += dt * (g.kind == Kind::Eagle ? 6 + (g.cheer > 0 ? 5 : 0) : 4 + pace * .07F);
        // Walkers hop now and then; the astronaut floats between steps.
        const float gravity = g.kind == Kind::Astronaut ? 88.F : 250.F;
        if (g.kind == Kind::Astronaut && g.lift <= 0 && g.climb <= 0) g.climb = 26;
        g.climb -= gravity * dt;
        g.lift = std::clamp(g.lift + g.climb * dt, 0.F, 11.F);
        if (g.lift <= 0) g.climb = 0;
    }
    std::erase_if(guests_, [&](const Guest& g) { return g.direction > 0 ? g.x > width_ + 40 : g.x < -40; });
    if (guests_.empty()) {
        wait_ -= dt;
        if (wait_ <= 0) {
            invite(std::uniform_int_distribution<int>(1, config_.flightLimit)(rng_));
            wait_ = float(config_.flightPause);
        }
    }
}
void ParadeEffect::draw() {
    frame_.overlay.clear();
    PixelCanvas canvas(frame_.overlay);
    // Left corner: the flag on its pole.
    canvas.rect(8, height_ - 44, pixel, 43, 0xc9d3df); canvas.rect(10, height_ - 44, pixel, 43, 0x7e8a99);
    canvas.rect(8, height_ - 46, 4, pixel, 0xffd23f);
    const float breeze = 4.5F + 3 * std::abs(config_.wind);
    for (int c = 0; c < 13; ++c) {
        const float sag = std::round(std::sin(float(c) * .62F - time_ * breeze) * std::min(1.F, float(c) / 5)) * pixel;
        for (int r = 0; r < 8; ++r) {
            const bool canton = c < 5 && r < 4;
            canvas.dot(12 + float(c) * pixel, height_ - 41 + float(r) * pixel + sag, canton ? ((c + r) % 2 ? 0xffffff : 0x3a5fb8) : r % 2 ? 0xfff4ec : 0xe0474f);
        }
    }
    constexpr std::array<Tint, 4> bunting{{{'r', 0xe0474f}, {'w', 0xfff4ec}, {'n', 0x3a5fb8}, {'.', 0}}};
    // A few hand flags stuck into the edge next to both corners.
    for (int i = 0; i < 6; ++i) {
        const float x = i < 3 ? 46 + float(i) * 14 : width_ - 50 - float(i - 3) * 14;
        const float tall = i % 3 == 1 ? 18.F : 14.F;
        canvas.rect(x, height_ - tall, pixel, tall, 0xc9d3df);
        canvas.sprite(art::tinyFlag, bunting, x + 7, height_ - tall + 8 + (int(time_ * 3 + float(i) * 1.7F) % 3 == 0 ? pixel : 0));
    }
    // Right corner: the statue. Her torch flickers.
    constexpr std::array<Tint, 12> copper{{{'T', 0x3f675b}, {'t', 0x6da78f}, {'J', 0xa9cfac}, {'E', 0x2f5248}, {'g', 0xc5873d}, {'G', 0xefbb57},
        {'S', 0xcfb389}, {'s', 0xac9374}, {'D', 0x6a5c57}, {'y', 0xffe68a}, {'o', 0xff9a4a}, {'.', 0}}};
    canvas.sprite(art::ladyLiberty, copper, width_ - 22, height_ - 1);
    for (int i = 0; i < 4; ++i) {
        const int beat = (int(time_ * 6) + i * 3) % 8;
        if (beat < 3) canvas.dot(width_ - 15 + float(i % 2 ? 4 : -4) * float(1 + beat % 2), height_ - 43 + float(i) * pixel, 0xfff4a8, .8F);
    }
    for (const auto& g : guests_) {
        if (g.x < -40 || g.x > width_ + 40) continue; // Still waiting for its turn.
        const bool flip = g.direction < 0, step = int(g.stride) % 2 != 0;
        const float bottom = height_ - 1 - g.lift;
        if (g.kind == Kind::Eagle) {
            constexpr std::array<Tint, 7> tints{{{'K', 0x2a2238}, {'B', 0x7a5236}, {'b', 0xa5764c}, {'w', 0xfff8ec}, {'E', 0x2a2238}, {'y', 0xffcf3f}, {'.', 0}}};
            const float y = height_ - 17 + std::sin(time_ * 2.1F + g.speed) * 2 - std::min(2.F, g.lift);
            canvas.sprite(step ? art::eagleUp : art::eagleDown, tints, g.x, y, flip);
        } else if (g.kind == Kind::Truck) {
            constexpr std::array<Tint, 10> tints{{{'K', 0x2a2238}, {'r', 0xe0474f}, {'R', 0xa52e36}, {'c', 0x9fdcff}, {'w', 0xfff4ec}, {'k', 0x35304a}, {'s', 0xc9d3df}, {'y', 0xffd23f}, {'n', 0x3a5fb8}, {'p', 0xc9d3df}}};
            canvas.sprite(step ? art::truckB : art::truckA, tints, g.x, bottom - (step ? 0.F : 1.F), flip);
        } else if (g.kind == Kind::Rider) {
            constexpr std::array<Tint, 9> tints{{{'K', 0x2a2238}, {'h', 0x8a5a34}, {'F', 0xf4c9a0}, {'E', 0x2a2238}, {'S', 0xe0474f}, {'j', 0x3a5fb8}, {'B', 0xa5693c}, {'b', 0x6b4022}, {'M', 0x3f2a1c}}};
            canvas.sprite(step ? art::riderB : art::riderA, tints, g.x, bottom - (step ? 2.F : 0.F), flip);
        } else if (g.kind == Kind::Astronaut) {
            constexpr std::array<Tint, 9> tints{{{'K', 0x2a2238}, {'W', 0xe9eef5}, {'s', 0x9fadbf}, {'v', 0x3a5fb8}, {'y', 0xffd23f}, {'r', 0xe0474f}, {'w', 0xfff4ec}, {'n', 0x3a5fb8}, {'p', 0xc9d3df}}};
            canvas.sprite(g.lift > 1 ? art::astroB : art::astroA, tints, g.x, bottom, flip);
        } else {
            constexpr std::array<Tint, 8> tints{{{'K', 0x2a2238}, {'D', 0xb5622f}, {'d', 0x7d3f1c}, {'E', 0x2a2238}, {'N', 0x2a2238}, {'U', 0xf0bf72}, {'u', 0xd1964a}, {'y', 0xffd23f}}};
            canvas.sprite(step ? art::hotDogB : art::hotDogA, tints, g.x, bottom, flip);
        }
        if (g.cheer > 0) {
            // A greeting over the guest: a note from the truck, a heart from everybody else.
            const float rise = std::round((1.5F - g.cheer) * 5);
            const float y = std::max(height_ - edgeStrip + 12, height_ - (g.kind == Kind::Eagle ? 34.F : 26.F) - g.lift - rise);
            constexpr std::array<Tint, 1> note{{{'y', 0xffd23f}}};
            constexpr std::array<Tint, 3> hearts{{{'K', 0x5b1830}, {'h', 0xf4476f}, {'H', 0xffb3c4}}};
            if (g.kind == Kind::Truck) canvas.sprite(art::musicNote, note, g.x + g.direction * 8, y, false, pixelOpacity(g.cheer));
            else canvas.sprite(art::heart, hearts, g.x + g.direction * 6, y, false, pixelOpacity(g.cheer));
        }
    }
    for (const auto& r : rockets_) {
        const auto& tint = sparks[std::size_t(r.tint)];
        if (r.age <= 0) {
            canvas.dot(r.point.x, r.point.y, tint[0]); canvas.dot(r.point.x, r.point.y + pixel, 0xffd23f, .8F);
            canvas.dot(r.point.x, r.point.y + 2 * pixel, 0xffd23f, .4F);
            continue;
        }
        const float open = std::min(1.F, r.age / .28F), fade = pixelOpacity(std::min(1.F, (.75F - r.age) / .4F), 4);
        for (int i = 0; i < 8; ++i) {
            const float angle = float(i) * .7854F;
            const float far = burstReach * open * (i % 2 ? .78F : 1.F);
            canvas.dot(r.point.x + std::round(std::cos(angle) * far / pixel) * pixel, r.point.y + std::round(std::sin(angle) * far / pixel) * pixel, tint[0], fade);
            if (r.age < .4F) canvas.dot(r.point.x + std::round(std::cos(angle) * far * .5F / pixel) * pixel, r.point.y + std::round(std::sin(angle) * far * .5F / pixel) * pixel, tint[1], fade);
        }
        if (r.age < .2F) canvas.dot(r.point.x, r.point.y, 0xffffff);
    }
    for (const auto& c : confetti_) {
        const float fade = pixelOpacity(std::min(1.F, (1.2F - c.age) / .4F), 4);
        if (c.tint == 0) {
            canvas.sprite(art::tinyFlag, bunting, c.point.x, c.point.y, int(c.age * 8) % 2 != 0, fade);
        } else {
            canvas.dot(c.point.x, c.point.y, c.tint == 1 ? 0xe0474f : c.tint == 2 ? 0xfff4ec : 0x4f8dff, fade);
        }
    }
}
}
