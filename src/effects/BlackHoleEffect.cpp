#include "effects/BlackHoleEffect.h"
#include "effects/SceneSprites.h"
#include <cmath>

namespace te {
namespace {
constexpr float pixel = PixelCanvas::pixel;
constexpr std::array tints{0xffffffU, 0xffd9a0U, 0xff9ac0U, 0xb9a5ffU, 0x8fd8ffU};
constexpr std::array disk{0xfff1c9U, 0xffc56bU, 0xff7a6bU, 0xd14fa0U, 0x7a4fe0U};
constexpr float travelSpeed = 92;
}
BlackHoleEffect::BlackHoleEffect(unsigned seed) : rng_(seed) { clear(); }
float BlackHoleEffect::range(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng_); }
void BlackHoleEffect::configure(EffectConfig config) {
    config.normalize();
    if (config.flightPause != config_.flightPause) wait_ = std::min(wait_, float(config.flightPause));
    config_ = config;
    if (!config_.enabled) clear();
}
void BlackHoleEffect::resize(float w, float h) {
    if (w == width_ && h == height_) return;
    if (width_ > 0 && w > 0) for (auto& s : stars_) { s.point.x *= w / width_; s.point.y += h - height_; }
    width_ = w; height_ = h;
}
void BlackHoleEffect::clear() {
    frame_.overlay.clear();
    stars_.clear(); sparks_.clear();
    charge_ = full_ = burst_ = time_ = spin_ = 0;
    travel_ = -1;
    wait_ = range(1.5F, 3);
}
void BlackHoleEffect::spawn(Star& s, bool anywhere) {
    s.point = {anywhere ? range(0, std::max(1.F, width_ - 70)) : range(-width_ * .25F, 0), height_ - range(5, edgeStrip - 5)};
    s.speed = range(9, 24);
    s.alpha = range(.4F, .9F);
    s.tint = std::uniform_int_distribution<int>(0, int(tints.size()) - 1)(rng_);
}
void BlackHoleEffect::erupt(float power) {
    const auto c = center();
    burst_ = 1;
    const int count = int(14 + 26 * power);
    for (int i = 0; i < count; ++i) {
        const float a = 6.2832F * (float(i) + range(-.4F, .4F)) / float(count), v = range(60, 240) * (.45F + .55F * power);
        sparks_.push_back({c, {v * std::cos(a), v * std::sin(a) * .34F}, 0, range(.5F, 1.1F), std::uniform_int_distribution<int>(0, int(tints.size()) - 1)(rng_)});
    }
    for (auto& s : stars_) if (c.x - s.point.x < 260) s.point.x -= 120 * power * (1 - (c.x - s.point.x) / 260);
}
void BlackHoleEffect::advance(float seconds, CursorSample cursor) {
    if (!needsAnimation()) { clear(); return; }
    const float dt = std::clamp(std::isfinite(seconds) ? seconds : 0.F, 0.F, .1F);
    time_ += dt;
    spin_ += dt * (1 + 2.4F * charge_) * (config_.wind < -.02F ? -1.F : 1.F) * (.8F + std::abs(config_.wind));
    burst_ = std::max(0.F, burst_ - dt * 1.4F);
    const auto c = center();
    const bool fed = config_.interaction && cursor.inside && std::isfinite(cursor.position.x) && std::isfinite(cursor.position.y)
        && std::abs(cursor.position.x - c.x) < 30 && std::abs(cursor.position.y - c.y) < 22;
    if (fed && burst_ <= 0) {
        charge_ = std::min(1.F, charge_ + dt / 1.6F);
        full_ = charge_ >= 1 ? full_ + dt : 0;
    }
    if ((!fed && charge_ > .3F) || full_ > 1.4F) { erupt(charge_); charge_ = full_ = 0; }
    else if (!fed) charge_ = std::max(0.F, charge_ - dt * 2);
    const auto target = std::size_t(std::clamp(width_ / 110, 3.F, 16.F) * (.6F + .5F * float(config_.ambientIntensity)) * load_);
    while (stars_.size() < target) { stars_.emplace_back(); spawn(stars_.back(), true); }
    stars_.resize(target);
    for (auto& s : stars_) {
        const float gap = c.x - s.point.x, pull = 1 + 5 * std::max(0.F, 1 - std::abs(gap) / 190) * (1 + 1.5F * charge_);
        s.point.x += s.speed * pull * dt;
        s.point.y += (c.y - s.point.y) * std::min(1.F, dt * 2.2F * std::max(0.F, 1 - std::abs(gap) / 130));
        if (gap < 8) spawn(s, false);
    }
    if (travel_ < 0) {
        wait_ -= dt;
        if (wait_ <= 0 && width_ > 160) {
            travel_ = 0;
            traveller_ = std::uniform_int_distribution<int>(0, 3)(rng_);
            lane_ = range(15, 30);
        }
    } else {
        const float gap = c.x - (travel_ - 14);
        travel_ += travelSpeed * (1 + 1.6F * std::max(0.F, 1 - gap / 150)) * dt;
        if (travel_ - 14 > c.x - 4) { travel_ = -1; wait_ = float(config_.flightPause) * range(.85F, 1.15F); erupt(.12F); }
    }
    for (auto& s : sparks_) {
        s.age += dt;
        s.point.x += s.velocity.x * dt; s.point.y += s.velocity.y * dt;
        s.velocity.x *= std::exp(-dt * 1.7F); s.velocity.y *= std::exp(-dt * 1.7F);
        s.point.y = std::clamp(s.point.y, height_ - edgeStrip + 3, height_ - 2);
        s.point.x = std::clamp(s.point.x, 2.F, std::max(2.F, width_ - 4));
    }
    std::erase_if(sparks_, [](const Spark& s) { return s.age >= s.life; });
    draw();
}
void BlackHoleEffect::drawHole(PixelCanvas& canvas, bool front) const {
    const auto c = center();
    const float R = 4.2F + 1.8F * charge_ + (charge_ > .5F && int(time_ * 24) % 2 ? .4F : 0.F);
    for (int y = -9; y <= 9; ++y) for (int x = -14; x <= 14; ++x) {
        const float px = float(x), py = float(y), d = std::sqrt(px * px + py * py);
        // The disk is tilted, so its ellipse leans to the upper right.
        const float tilted = py + px * .26F, e = std::sqrt(px * px * .92F + tilted * tilted * 9);
        const bool ring = e > R + 1.4F && e < R + 8.4F;
        const bool near = tilted >= 0;
        const float sx = c.x + px * pixel - pixel / 2, sy = c.y + py * pixel - pixel / 2;
        if (!front) {
            if (d <= R) { canvas.dot(sx, sy, 0x05030c); continue; }
            if (d <= R + 1.1F) { canvas.dot(sx, sy, burst_ > .3F ? 0xffffffU : 0xffe2aeU); continue; }
            if (d <= R + 2) { canvas.dot(sx, sy, 0xff9e6b, .55F); continue; }
        }
        if (!ring || near != front) continue;
        const float u = (e - R - 1.4F) / 7, angle = std::atan2(tilted * 3, px);
        const float streak = std::sin(angle * 3 - spin_ * 4 + u * 8);
        if (u > .45F && streak < -.2F + u * .5F) continue; // Gaps in the outer disk.
        const auto index = std::size_t(std::clamp(u * 4.6F + (streak < 0 ? .5F : 0.F), 0.F, 4.F));
        canvas.dot(sx, sy, disk[index], 1 - .35F * u);
    }
}
void BlackHoleEffect::draw() {
    frame_.overlay.clear();
    PixelCanvas canvas(frame_.overlay);
    const auto c = center();
    for (const auto& s : stars_) {
        if (s.point.x < 0 || s.point.y < height_ - edgeStrip) continue;
        const auto tint = tints[std::size_t(s.tint)];
        const float fade = std::min(1.F, s.point.x / 30);
        const int tail = std::clamp(int(3 - (c.x - s.point.x) / 60), 0, 3);
        canvas.dot(s.point.x, s.point.y, tint, s.alpha * fade);
        for (int i = 1; i <= tail; ++i) canvas.dot(s.point.x - float(i) * pixel, s.point.y, tint, s.alpha * .5F / float(i));
    }
    drawHole(canvas, false);
    if (travel_ >= 0) {
        const float x = travel_ - 14, gap = c.x - x;
        const PixelArt& sprite = traveller_ == 0 ? art::planet : traveller_ == 1 ? art::ringed : traveller_ == 2 ? art::rocket : art::saucer;
        constexpr std::array<Tint, 14> palette{{{'p', 0x5bc2b0}, {'P', 0xa8f0dc}, {'q', 0x2f8f93}, {'b', 0x46a89c}, {'r', 0xffcf8a}, {'w', 0xf3f6ff}, {'f', 0xffb03b},
                                                {'c', 0x8fe9ff}, {'C', 0xffffff}, {'m', 0x9fadbf}, {'M', 0xff5a5f}, {'y', 0xffd23f}, {'.', 0}, {'.', 0}}};
        const float pull = std::clamp(1 - gap / 110, 0.F, 1.F);
        const float middle = lane_ + (height_ - c.y - lane_) * pull + (int(time_ * 3) % 2 ? 1.F : -1.F);
        const float bottom = height_ - middle + float(sprite.height);
        const float alpha = std::clamp((gap - 6) / 16, 0.F, 1.F) * std::clamp(travel_ / 26, 0.F, 1.F);
        canvas.sprite(sprite, palette, x, bottom, false, alpha);
        if (traveller_ == 2 && int(time_ * 14) % 2) canvas.rect(x - float(sprite.width) - 4, bottom - 6, 4, 2, 0xff8a3c, alpha);
    }
    drawHole(canvas, true);
    for (const auto& s : sparks_) canvas.dot(s.point.x, s.point.y, tints[std::size_t(s.tint)], 1 - s.age / s.life);
}
}
