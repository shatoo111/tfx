#include "effects/AquariumEffect.h"
#include "effects/SceneSprites.h"
#include <cmath>

namespace te {
namespace {
constexpr float pixel = PixelCanvas::pixel;
constexpr float ceiling = edgeStrip - 9, bed = 11; // Swimming band, measured from the bottom edge.
}
AquariumEffect::AquariumEffect(unsigned seed) : rng_(seed) { clear(); }
float AquariumEffect::range(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng_); }
void AquariumEffect::configure(EffectConfig config) {
    config.normalize();
    const bool changed = config.ambientIntensity != config_.ambientIntensity;
    config_ = config;
    if (!config_.enabled) clear();
    else if (changed) fish_.clear();
}
void AquariumEffect::resize(float w, float h) {
    if (w == width_ && h == height_) return;
    if (width_ > 0 && w > 0) {
        for (auto& f : fish_) { f.point.x *= w / width_; f.point.y += h - height_; }
        for (auto& b : bubbles_) { b.point.x *= w / width_; b.point.y += h - height_; }
        crab_ *= w / width_;
    } else {
        crab_ = w * .3F;
    }
    width_ = w; height_ = h;
}
void AquariumEffect::clear() {
    frame_.overlay.clear();
    fish_.clear(); bubbles_.clear();
    time_ = chest_ = chestBubble_ = stream_ = 0;
    tracked_ = false;
}
float AquariumEffect::puffed() const {
    float most = 0;
    for (const auto& f : fish_) most = std::max(most, f.puff);
    return most;
}
Vec2 AquariumEffect::pufferPoint() const {
    for (const auto& f : fish_) if (f.kind == Kind::Puffer) return f.point;
    return {};
}
void AquariumEffect::populate() {
    if (!fish_.empty()) return;
    const auto add = [&](Kind kind, float x, float lane, float direction, float speed, int tint) {
        fish_.push_back({kind, {x, height_ - lane}, speed, 0, direction, lane, range(0, 6.28F), 0, tint});
    };
    const auto side = [&] { return range(0, 1) < .5F ? -1.F : 1.F; };
    constexpr std::array order{Kind::Clown, Kind::Puffer, Kind::Tang, Kind::Clown, Kind::Tang};
    const int big = 2 + config_.ambientIntensity + (config_.ambientIntensity == 2 ? 1 : 0);
    for (int i = 0; i < big; ++i)
        add(order[std::size_t(i)], width_ * (float(i) + range(.1F, .9F)) / float(big), range(bed + 5, ceiling - 4), side(), order[std::size_t(i)] == Kind::Puffer ? 11.F : range(16, 25), i);
    const float direction = side(), x = range(.2F, .8F) * width_;
    const int school = 2 + config_.ambientIntensity;
    for (int i = 0; i < school; ++i) add(Kind::Minnow, x - float(i) * 13 * direction, 18 + float((i * 7) % 15), direction, 29, 0);
}
void AquariumEffect::bubble(Vec2 point) {
    if (bubbles_.size() < 36) bubbles_.push_back({point, range(9, 16), range(0, 6.28F), 0, range(0, 1) < .35F});
}
void AquariumEffect::advance(float seconds, CursorSample cursor) {
    if (!needsAnimation()) { clear(); return; }
    const float dt = std::clamp(std::isfinite(seconds) ? seconds : 0.F, 0.F, .1F);
    if (!config_.interaction || !std::isfinite(cursor.position.x) || !std::isfinite(cursor.position.y)) cursor.inside = false;
    time_ += dt;
    populate();
    const float speed = cursor.inside && tracked_ && dt > 0
        ? std::hypot(cursor.position.x - previous_.x, cursor.position.y - previous_.y) / dt : 0.F;
    previous_ = cursor.position;
    tracked_ = cursor.inside;
    for (auto& f : fish_) {
        const float dx = f.point.x - cursor.position.x, dy = f.point.y - cursor.position.y;
        const float d = cursor.inside ? std::hypot(dx, dy) : 1e6F;
        const bool startled = d < 30 || (d < 52 && speed > 120);
        if (f.kind == Kind::Puffer) {
            f.puff += ((startled ? 1.F : 0.F) - f.puff) * (1 - std::exp(-dt * (startled ? 12.F : 1.8F)));
        } else if (startled) {
            f.boost = 1;
            f.direction = dx >= 0 ? 1.F : -1.F;
        }
        f.boost = std::max(0.F, f.boost - dt * .9F);
        f.phase += dt * (5 + 9 * f.boost);
        f.point.x += (f.direction * f.speed * (1 + 7 * f.boost) * (1 - .8F * f.puff) + config_.wind * 7) * dt;
        f.point.y = height_ - std::clamp(f.lane + 3 * std::sin(time_ * .8F + f.lane), bed, ceiling);
        if (f.point.x > width_ + 26 || f.point.x < -26) {
            // Beyond the window it turns round and comes back in another lane.
            f.point.x = std::clamp(f.point.x, -26.F, width_ + 26);
            f.direction = f.point.x < 0 ? 1.F : -1.F;
            f.boost = 0;
            if (f.kind != Kind::Minnow) f.lane = range(bed + 5, ceiling - 4);
        }
        if (f.kind != Kind::Minnow && range(0, 1) < dt * .1F) bubble({f.point.x + f.direction * 10, f.point.y});
    }
    // The crab patrols the bed and pauses now and then.
    crabRest_ -= dt;
    if (crabRest_ <= 0) {
        const bool scared = cursor.inside && std::abs(cursor.position.x - crab_) < 34 && cursor.position.y > height_ - 34;
        if (scared) crabDirection_ = cursor.position.x < crab_ ? 1.F : -1.F;
        const float pace = scared ? 62.F : 13.F;
        crab_ += crabDirection_ * pace * dt;
        crabStep_ += dt * pace * .32F;
        if (crab_ < 60) { crab_ = 60; crabDirection_ = 1; }
        if (crab_ > width_ - 74) { crab_ = std::max(60.F, width_ - 74); crabDirection_ = -1; }
        if (!scared && range(0, 1) < dt * .22F) { crabRest_ = range(1, 3); if (range(0, 1) < .4F) crabDirection_ = -crabDirection_; }
    }
    const auto chest = chestPoint();
    const bool opened = cursor.inside && std::abs(cursor.position.x - chest.x) < 18 && cursor.position.y > height_ - 28;
    chest_ += ((opened ? 1.F : 0.F) - chest_) * (1 - std::exp(-dt * (opened ? 10.F : 3.F)));
    chestBubble_ += dt * (chest_ > .4F ? 9.F : .25F);
    while (chestBubble_ >= 1) { chestBubble_ -= 1; bubble({chest.x + range(-8, 8), height_ - 20}); }
    stream_ -= dt;
    if (stream_ <= 0) { bubble({20 + range(-4, 4), height_ - 8}); stream_ = range(1.2F, 2.6F); }
    for (auto& b : bubbles_) { b.age += dt; b.point.y -= b.speed * dt; b.point.x += (config_.wind * 5 + 3 * std::sin(b.age * 3 + b.phase)) * dt; }
    std::erase_if(bubbles_, [&](const Bubble& b) { return b.point.y < height_ - edgeStrip + 4; });
    draw();
}
void AquariumEffect::draw() {
    frame_.overlay.clear();
    PixelCanvas canvas(frame_.overlay);
    // Plants only in the two corners.
    const auto weed = [&](float x, int rows, int seed) {
        for (int r = 0; r < rows; ++r) {
            const float sway = std::round(std::sin(time_ * 1.6F + float(r) * .55F + float(seed)) * float(r) / float(rows) * 1.5F) * pixel;
            const float y = height_ - 2 - float(r + 1) * pixel;
            canvas.dot(x + sway, y, r > rows - 3 ? 0x7be0a0 : 0x2f9a6c);
            if (r < rows - 2) canvas.dot(x + sway + pixel, y, 0x1f7a58);
            if (r % 4 == 2 && r < rows - 1) canvas.dot(x + sway + (r % 8 == 2 ? -pixel : 2 * pixel), y, 0x53c98c);
        }
    };
    weed(8, 12, 1); weed(19, 8, 2); weed(width_ - 12, 13, 4); weed(width_ - 68, 8, 6);
    constexpr std::array<Tint, 2> corals{{{'p', 0xff8fb0}, {'P', 0xd95c88}}};
    canvas.sprite(art::coral, corals, 38, height_ - 2);
    const auto sand = [&](float from, float to) {
        for (int c = 0; float(c) * pixel < to - from; ++c) {
            const float x = from + float(c) * pixel, edge = std::min(x - from, to - x) / 12;
            if (hash01(c, 3) > edge + .25F) continue;
            canvas.rect(x, height_ - 4, pixel, 4, hash01(c, 4) > .75F ? 0xd9b277 : 0xf2d9a2);
            if (hash01(c, 5) > .62F && edge > .6F) canvas.dot(x, height_ - 6, 0xf2d9a2);
        }
    };
    sand(0, 58); sand(width_ - 80, width_);
    const bool open = chest_ > .4F;
    constexpr std::array<Tint, 7> chestTints{{{'n', 0x6b4022}, {'N', 0xa5683c}, {'g', 0xe0a030}, {'G', 0xffd65c}, {'Y', 0xfff4a8}, {'.', 0}, {'.', 0}}};
    const auto chest = chestPoint();
    canvas.sprite(open ? art::chestOpen : art::chestClosed, chestTints, chest.x, height_ - 3);
    if (open) for (int i = 0; i < 3; ++i)
        if (int(time_ * 5 + float(i) * 1.7F) % 3 == 0) canvas.dot(chest.x - 8 + float(i) * 8, height_ - 28 - float(i % 2) * 4, 0xfff4a8);
    constexpr std::array<Tint, 3> crabTints{{{'r', 0xe5533d}, {'R', 0xff8a6b}, {'K', 0x2a2238}}};
    canvas.sprite(int(crabStep_) % 2 ? art::crabB : art::crabA, crabTints, crab_, height_ - 1);
    constexpr std::array<std::array<std::uint32_t, 2>, 2> clowns{{{0xff8a3d, 0xd9642a}, {0xff5d73, 0xcf3f5c}}};
    for (const auto& f : fish_) {
        const bool flip = f.direction < 0;
        const float wiggle = std::sin(f.phase) > 0 ? 0.F : 1.F;
        if (f.kind == Kind::Clown) {
            const auto& c = clowns[std::size_t(f.tint) % clowns.size()];
            const std::array<Tint, 4> tints{{{'O', c[0]}, {'o', c[1]}, {'W', 0xffffff}, {'E', 0x1b2238}}};
            canvas.sprite(art::clown, tints, f.point.x, f.point.y + 6 + wiggle, flip);
        } else if (f.kind == Kind::Tang) {
            constexpr std::array<Tint, 5> tints{{{'B', 0x3f7dff}, {'n', 0x2a55bf}, {'Y', 0xffd34d}, {'K', 0x1b2a5c}, {'E', 0xffffff}}};
            canvas.sprite(art::tang, tints, f.point.x, f.point.y + 6 + wiggle, flip);
        } else if (f.kind == Kind::Puffer) {
            constexpr std::array<Tint, 4> tints{{{'y', 0xffd257}, {'s', 0xc98a2a}, {'w', 0xfff6de}, {'E', 0x1b2238}}};
            if (f.puff > .45F) canvas.sprite(art::pufferBig, tints, f.point.x, std::clamp(f.point.y + 11, height_ - edgeStrip + 24, height_ - 1), flip);
            else canvas.sprite(art::puffer, tints, f.point.x, f.point.y + 6 + wiggle, flip);
        } else {
            constexpr std::array<Tint, 3> tints{{{'c', 0x7fd8f5}, {'C', 0xffffff}, {'E', 0x1b2238}}};
            canvas.sprite(art::minnow, tints, f.point.x, f.point.y + 3 + wiggle, flip);
        }
    }
    for (const auto& b : bubbles_) {
        if (b.big) {
            canvas.rect(b.point.x, b.point.y - pixel, pixel, pixel, 0xd8f4ff, .8F); canvas.rect(b.point.x - pixel, b.point.y, pixel, pixel, 0xd8f4ff, .8F);
            canvas.rect(b.point.x + pixel, b.point.y, pixel, pixel, 0xa9dcf5, .7F); canvas.rect(b.point.x, b.point.y + pixel, pixel, pixel, 0xa9dcf5, .7F);
        } else {
            canvas.dot(b.point.x, b.point.y, 0xd8f4ff, .75F);
        }
    }
}
}
