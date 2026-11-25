#include "effects/CatEffect.h"
#include "effects/SceneSprites.h"
#include <cmath>

namespace te {
namespace {
constexpr float walkSpeed = 34, runSpeed = 118, margin = 22, ballRadius = 8;
// Base, stripe, light, eye, outline for every coat.
constexpr std::array<std::array<std::uint32_t, 5>, 4> coats{{{0xf2a54a, 0xc9742c, 0xfff1dc, 0x2a2238, 0x2a2238},
    {0x4a4e66, 0x3a3e54, 0x6b7190, 0xc8e05a, 0x1d1f2e}, {0x9aa6b8, 0x75829a, 0xe9eef5, 0x2a2238, 0x2c3242}, {0xf6f3ec, 0xd9d4cb, 0xffffff, 0x3f8cd8, 0x5d6478}}};
constexpr std::array<std::array<std::uint32_t, 2>, 3> yarns{{{0xff6f8f, 0xffb3c4}, {0x4fb3ff, 0xb5e0ff}, {0xffc94a, 0xffeaa8}}};
}
CatEffect::CatEffect(unsigned seed) : rng_(seed) { clear(); }
float CatEffect::range(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng_); }
void CatEffect::configure(EffectConfig config) {
    config.normalize();
    config_ = config;
    if (!config_.enabled) { clear(); return; }
    const auto wanted = std::size_t(config_.ambientIntensity + 1);
    while (balls_.size() < wanted) {
        Ball ball;
        ball.color = int(balls_.size());
        ball.x = width_ * (.36F + .17F * float(balls_.size()));
        balls_.push_back(ball);
    }
    if (balls_.size() > wanted) { balls_.resize(wanted); chased_ = -1; }
}
void CatEffect::resize(float w, float h) {
    if (w == width_ && h == height_) return;
    const bool first = width_ <= 0 || height_ <= 0;
    if (!first && w > 0) {
        x_ *= w / width_; target_ *= w / width_;
        for (auto& b : balls_) b.x *= w / width_;
    }
    width_ = w; height_ = h;
    if (first) {
        x_ = target_ = w * .84F;
        for (std::size_t i = 0; i < balls_.size(); ++i) balls_[i].x = w * (.36F + .17F * float(i));
    }
    x_ = std::clamp(x_, std::min(margin, w * .5F), std::max(w - margin, w * .5F));
}
void CatEffect::clear() {
    frame_.overlay.clear();
    hearts_.clear();
    mode_ = Mode::Sit;
    modeAge_ = idle_ = time_ = stride_ = purr_ = heartWait_ = swat_ = swatWait_ = boredom_ = batWait_ = blink_ = 0;
    rest_ = 4; nextBlink_ = 2; bats_ = 0; chased_ = -1;
    tracked_ = false;
    for (auto& b : balls_) b.vx = b.vy = b.lift = 0;
}
void CatEffect::setMode(Mode mode) {
    if (mode == mode_) return;
    mode_ = mode;
    modeAge_ = 0;
    if (mode == Mode::Sit) rest_ = range(5, 10);
    if (mode == Mode::Sleep) rest_ = range(20, 36);
    if (mode == Mode::Walk) {
        const float low = margin, high = std::max(low, width_ - margin);
        for (int i = 0; i < 6; ++i) { target_ = range(low, high); if (std::abs(target_ - x_) > 80) break; }
    }
}
void CatEffect::moveBalls(float dt, CursorSample cursor, Vec2 velocity) {
    for (auto& b : balls_) {
        if (cursor.inside && std::hypot(velocity.x, velocity.y) > 60) {
            const float dx = b.x - cursor.position.x, dy = ground() - b.lift - ballRadius - cursor.position.y;
            if (dx * dx + dy * dy < 20 * 20) {
                b.vx = std::clamp(b.vx + velocity.x * .4F, -420.F, 420.F);
                b.vy = std::max(b.vy, std::min(170.F, 70 + std::abs(velocity.y) * .2F));
                boredom_ = 0;
            }
        }
        b.vy -= 760 * dt;
        b.lift += b.vy * dt;
        if (b.lift <= 0) {
            b.lift = 0;
            b.vy = b.vy < -60 ? -b.vy * .42F : 0;
            b.vx *= std::exp(-dt * 1.3F);
            if (std::abs(b.vx) < 4) b.vx = 0;
        }
        b.lift = std::min(b.lift, edgeStrip - 2 * ballRadius - 4); // Never bounces into the chat.
        b.x += b.vx * dt;
        if (b.x < ballRadius) { b.x = ballRadius; b.vx = std::abs(b.vx) * .7F; }
        if (b.x > width_ - ballRadius) { b.x = std::max(ballRadius, width_ - ballRadius); b.vx = -std::abs(b.vx) * .7F; }
        b.turn += b.vx * dt / ballRadius;
    }
}
void CatEffect::think(float dt, CursorSample cursor, float cursorSpeed) {
    modeAge_ += dt;
    swat_ = std::max(0.F, swat_ - dt / .4F);
    swatWait_ -= dt; batWait_ -= dt; heartWait_ -= dt;
    boredom_ = std::max(0.F, boredom_ - dt);
    idle_ = cursor.inside && cursorSpeed > 12 ? 0 : idle_ + dt;
    nextBlink_ -= dt;
    if (nextBlink_ <= 0) { blink_ = .16F; nextBlink_ = range(2.2F, 5.5F); }
    blink_ = std::max(0.F, blink_ - dt);
    const float dx = cursor.position.x - x_, dy = cursor.position.y - (ground() - 18);
    const float distance = cursor.inside ? std::hypot(dx, dy) : 1e6F;
    const bool over = cursor.inside && std::abs(dx) < 17 && cursor.position.y > ground() - 34;
    if (over && cursorSpeed > 20 && mode_ != Mode::Chase) {
        if (mode_ == Mode::Walk) setMode(Mode::Sit);
        purr_ = std::min(1.F, purr_ + dt * 1.7F);
        rest_ = std::max(rest_, modeAge_ + 3);
    } else {
        purr_ = std::max(0.F, purr_ - dt * .55F);
    }
    if (purr_ > .35F && heartWait_ <= 0 && hearts_.size() < 8) {
        hearts_.push_back({{x_ + range(-14, 14), ground() - range(20, 30)}, 0, range(-8, 8)});
        heartWait_ = .5F;
    }
    if (mode_ == Mode::Sit && cursor.inside && idle_ < 5 && std::abs(dx) > 10) facing_ = dx > 0 ? 1.F : -1.F;
    // Rolling yarn is irresistible until the cat gets bored.
    if (mode_ != Mode::Chase && boredom_ <= 0 && purr_ < .3F) {
        for (std::size_t i = 0; i < balls_.size(); ++i)
            if (std::abs(balls_[i].vx) > 50) { chased_ = int(i); bats_ = 0; setMode(Mode::Chase); break; }
    }
    const float low = margin, high = std::max(low, width_ - margin);
    const auto travel = [&](float goal, float speed) {
        const float left = goal - x_, move = speed * dt;
        if (std::abs(left) > 1) facing_ = left > 0 ? 1.F : -1.F;
        x_ = std::abs(left) <= move ? goal : x_ + (left > 0 ? move : -move);
        stride_ += dt * (speed > 60 ? 11.F : 5.5F);
        x_ = std::clamp(x_, low, high);
    };
    switch (mode_) {
    case Mode::Sit:
        if (config_.interaction && swatWait_ <= 0 && purr_ < .2F && cursorSpeed > 40 && !over && distance > 16 && distance < 42) {
            swat_ = 1; swatSide_ = dx > 0 ? 1.F : -1.F; facing_ = swatSide_; swatWait_ = 1;
            rest_ = std::max(rest_, modeAge_ + 2);
        }
        if (idle_ > 11 && modeAge_ > 5) setMode(Mode::Sleep);
        else if (modeAge_ > rest_) setMode(Mode::Walk);
        break;
    case Mode::Walk:
        travel(target_, walkSpeed);
        if (std::abs(target_ - x_) < 1) setMode(Mode::Sit);
        break;
    case Mode::Sleep:
        if ((distance < 50 && cursorSpeed > 60) || (idle_ < .1F && modeAge_ > 12) || modeAge_ > rest_) {
            idle_ = std::min(idle_, 0.F) - 12; // Rested: time for a stroll even if nobody is around.
            setMode(Mode::Sit);
        }
        break;
    case Mode::Chase: {
        if (chased_ < 0 || std::size_t(chased_) >= balls_.size()) { setMode(Mode::Sit); break; }
        auto& b = balls_[std::size_t(chased_)];
        const float gap = b.x - x_;
        if (batWait_ > 0) break;
        if (std::abs(gap) > 24) {
            travel(b.x - (gap > 0 ? 20.F : -20.F), runSpeed);
        } else {
            float push = gap >= 0 ? 1.F : -1.F;
            if (b.x < 46) push = 1; else if (b.x > width_ - 46) push = -1;
            facing_ = gap >= 0 ? 1.F : -1.F;
            b.vx = push * range(150, 270);
            b.vy = range(40, 130);
            batWait_ = .34F;
            if (++bats_ >= 4) { boredom_ = range(9, 15); chased_ = -1; idle_ = 0; setMode(Mode::Sit); }
        }
        if (modeAge_ > 14) { boredom_ = 8; chased_ = -1; setMode(Mode::Sit); }
        break;
    }
    }
}
void CatEffect::advance(float seconds, CursorSample cursor) {
    if (!needsAnimation()) { clear(); return; }
    const float dt = std::clamp(std::isfinite(seconds) ? seconds : 0.F, 0.F, .1F);
    if (!config_.interaction || !std::isfinite(cursor.position.x) || !std::isfinite(cursor.position.y)) cursor.inside = false;
    time_ += dt;
    Vec2 velocity{};
    if (cursor.inside && tracked_ && dt > 0)
        velocity = {std::clamp((cursor.position.x - previous_.x) / dt, -2200.F, 2200.F), std::clamp((cursor.position.y - previous_.y) / dt, -2200.F, 2200.F)};
    previous_ = cursor.position;
    tracked_ = cursor.inside;
    moveBalls(dt, cursor, velocity);
    think(dt, cursor, std::hypot(velocity.x, velocity.y));
    for (auto& h : hearts_) { h.age += dt; h.point.y = std::max(h.point.y - 13 * dt, ground() - edgeStrip + 14); h.point.x += h.drift * dt; }
    std::erase_if(hearts_, [](const Heart& h) { return h.age > 1.4F; });
    draw();
}
void CatEffect::draw() {
    frame_.overlay.clear();
    PixelCanvas canvas(frame_.overlay);
    for (const auto& b : balls_) {
        const auto& yarn = yarns[std::size_t(b.color) % yarns.size()];
        const std::array<Tint, 3> palette{{{'K', 0x2a2238}, {'y', yarn[0]}, {'Y', yarn[1]}}};
        const float bottom = ground() - b.lift, side = b.vx > 0 ? -1.F : 1.F;
        if (b.lift < 4) for (int i = 0; i < 6; ++i) // Loose thread.
            canvas.dot(b.x + side * (ballRadius + float(i) * 2) - 1, bottom - (i == 3 ? 4.F : 2.F), yarn[0]);
        canvas.sprite(art::yarnBall, palette, b.x, bottom, int(std::floor(b.turn * 1.3F)) % 2 != 0);
    }
    const auto& coat = coats[std::size_t(config_.catCoat)];
    const std::array<Tint, 9> palette{{{'K', coat[4]}, {'b', coat[0]}, {'s', coat[1]}, {'l', coat[2]}, {'W', 0xffffff}, {'E', coat[3]}, {'n', 0xff7f92}, {'p', 0xff9dab}, {'.', 0}}};
    const bool right = facing_ > 0;
    const float floor = ground();
    switch (mode_) {
    case Mode::Sit: {
        const PixelArt& pose = purr_ > .25F ? art::catHappy : blink_ > 0 ? art::catBlink : int(time_ * 1.4F) % 3 == 0 ? art::catSitWag : art::catSit;
        const float hum = purr_ > .25F && int(time_ * 18) % 2 ? 1.F : 0.F;
        canvas.sprite(pose, palette, x_ + hum, floor, !right);
        if (swat_ > 0) {
            const float reach = std::sin(3.1416F * (1 - swat_));
            canvas.sprite(art::catPaw, palette, x_ + swatSide_ * (9 + 4 * reach), floor - 8 - 6 * reach, swatSide_ < 0);
        }
        break;
    }
    case Mode::Walk:
        canvas.sprite(int(stride_) % 2 ? art::catWalkB : art::catWalkA, palette, x_, floor, !right);
        break;
    case Mode::Chase: {
        const bool bat = batWait_ > 0;
        canvas.sprite(bat || int(stride_) % 2 ? art::catWalkB : art::catWalkA, palette, x_, floor - (!bat && int(stride_) % 2 ? 2.F : 0.F), !right);
        if (bat) canvas.sprite(art::catPaw, palette, x_ + facing_ * 16, floor - 6, !right);
        break;
    }
    case Mode::Sleep:
        canvas.sprite(std::sin(time_ * 2) > 0 ? art::catSleepA : art::catSleepB, palette, x_, floor, !right);
        for (int i = 0; i < 2; ++i) {
            const float t = std::fmod(modeAge_ * .4F + float(i) * .5F, 1.F);
            constexpr std::array<Tint, 1> letter{{{'z', 0xdfe9ff}}}, shade{{{'z', 0x31406b}}};
            const float zx = x_ + facing_ * (4 + 10 * t), zy = floor - 20 - 15 * t, alpha = 1 - t;
            canvas.sprite(art::letterZ, shade, zx + 1, zy + 1, false, alpha * .6F);
            canvas.sprite(art::letterZ, letter, zx, zy, false, alpha);
        }
        break;
    }
    constexpr std::array<Tint, 3> hearts{{{'K', 0x5b1830}, {'h', 0xf4476f}, {'H', 0xffb3c4}}};
    for (const auto& h : hearts_) canvas.sprite(art::heart, hearts, h.point.x, h.point.y, false, std::min(1.F, (1.4F - h.age) * 2));
}
}
