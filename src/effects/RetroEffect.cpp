#include "effects/RetroEffect.h"
#include "effects/PixelSprites.h"
#include <cmath>
#include <numbers>

namespace te {
namespace {
constexpr float tau = 2 * std::numbers::pi_v<float>;
constexpr float gravity = 900;      // pt/s²
constexpr float runSpeed = 96;      // pt/s
constexpr float hidden = -28;       // Lift at which the hero is fully inside its home.
constexpr float heroHalfWidth = 16 * RetroEffect::pixel / 2;
constexpr float heroHeight = 16 * RetroEffect::pixel;
constexpr std::array<std::array<Artwork, 4>, 3> heroes{{
    {Artwork::CatIdle, Artwork::CatRunA, Artwork::CatRunB, Artwork::CatJump},
    {Artwork::KnightIdle, Artwork::KnightRunA, Artwork::KnightRunB, Artwork::KnightJump},
    {Artwork::RobotIdle, Artwork::RobotRunA, Artwork::RobotRunB, Artwork::RobotJump},
}};
constexpr std::array homes{Artwork::PixelCrate, Artwork::PixelWell, Artwork::PixelPad};
constexpr float coinDensity(int intensity) { return .5F + .6F * intensity; }
float snap(float v) { return std::round(v * 2) / 2; }
}

RetroEffect::RetroEffect(unsigned seed) : rng_(seed) {
    coins_.reserve(64);
    frame_.illustrations.reserve(96);
    clear();
}
float RetroEffect::range(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(rng_); }
// The home sits in the lower-right corner; the hero runs the full width to the left wall.
float RetroEffect::homeX() const { return std::max(width_ - 8 - 20 * pixel / 2, 8 + 20 * pixel / 2); }
float RetroEffect::laneEnd() const { return std::min(heroHalfWidth + 6, std::max(0.F, homeX() - 40)); }
float RetroEffect::turnBack() const { return std::max(laneEnd(), homeX() - 58); }
void RetroEffect::configure(EffectConfig config) {
    config.normalize();
    if (config_.flightPause != config.flightPause && stage_ == Stage::Home)
        wait_ = std::min(wait_, float(config.flightPause));
    config_ = config;
    if (!config_.enabled) clear();
}
void RetroEffect::resize(float w, float h) {
    if (w == width_ && h == height_) return;
    if (width_ > 0 && height_ > 0 && w > 0 && h > 0) {
        for (auto& coin : coins_) { coin.point.x *= w / width_; coin.point.y *= h / height_; }
    }
    width_ = w; height_ = h;
    heroX_ = std::clamp(heroX_, laneEnd(), homeX());
}
void RetroEffect::clear() {
    frame_.illustrations.clear();
    coins_.clear();
    bursts_ = {};
    stage_ = Stage::Home;
    heroLift_ = hidden; heroVx_ = heroVy_ = 0;
    airborne_ = false; facingRight_ = false;
    laps_ = 0;
    runClock_ = 0; time_ = 0; hopCooldown_ = 3;
    wait_ = range(.8F, 1.8F);
}
void RetroEffect::spawnCoin(Coin& coin, bool anywhere) {
    coin.point = {range(10, std::max(11.F, width_ - 10)), anywhere ? range(-height_ * .6F, height_) : range(-60, -12)};
    coin.speed = range(42, 82);
    coin.phase = range(0, 4);
    coin.spin = range(4.5F, 7.5F);
    coin.sway = range(0, tau);
}
void RetroEffect::burst(Artwork art, Vec2 point, float direction) {
    auto oldest = std::max_element(bursts_.begin(), bursts_.end(), [](const Burst& a, const Burst& b) {
        const float ageA = a.age < 0 ? 1e9F : a.age, ageB = b.age < 0 ? 1e9F : b.age;
        return ageA < ageB;
    });
    *oldest = {point, 0, art, direction};
}
void RetroEffect::startTrip() {
    stage_ = Stage::LeavingHome;
    heroX_ = homeX();
    heroLift_ = -10;
    heroVy_ = 320;
    heroVx_ = -74;
    airborne_ = true;
    facingRight_ = false;
    laps_ = int(range(2, 4)); // Two or three full runs across the window and back.
}
void RetroEffect::hop(float speed) {
    if (airborne_) return;
    airborne_ = true;
    heroVy_ = speed;
}
void RetroEffect::land() {
    heroLift_ = 0; heroVy_ = 0; airborne_ = false;
    burst(Artwork::PixelDust, {heroX_ - (facingRight_ ? 10.F : -10.F), ground()}, facingRight_ ? -1.F : 1.F);
}
void RetroEffect::moveHero(float dt, CursorSample cursor) {
    if (stage_ == Stage::Home) {
        wait_ -= dt;
        if (wait_ <= 0) startTrip();
        return;
    }
    if (airborne_) {
        heroVy_ -= gravity * dt;
        heroLift_ += heroVy_ * dt;
        heroX_ += heroVx_ * dt;
    }
    switch (stage_) {
    case Stage::LeavingHome:
        if (heroVy_ < 0 && heroLift_ <= 0) { land(); stage_ = Stage::Running; heroVx_ = 0; }
        break;
    case Stage::GoingHome:
        if (heroVy_ < 0 && heroLift_ <= hidden) {
            stage_ = Stage::Home;
            heroLift_ = hidden;
            airborne_ = false;
            wait_ = float(config_.flightPause) + range(0, 2);
        }
        break;
    case Stage::Running: {
        if (airborne_) {
            if (heroVy_ < 0 && heroLift_ <= 0) land();
        } else {
            runClock_ += dt;
            heroX_ += (facingRight_ ? runSpeed : -runSpeed) * dt;
            hopCooldown_ -= dt;
            if (hopCooldown_ <= 0) { hop(240); hopCooldown_ = range(3.5F, 7.5F); }
        }
        if (!facingRight_ && heroX_ <= laneEnd()) {
            heroX_ = laneEnd();
            facingRight_ = true;
            --laps_;
            if (!airborne_) burst(Artwork::PixelDust, {heroX_ - 10, ground()}, -1);
        } else if (facingRight_ && laps_ > 0 && heroX_ >= turnBack()) {
            heroX_ = turnBack();
            facingRight_ = false;
            if (!airborne_) burst(Artwork::PixelDust, {heroX_ + 10, ground()}, 1);
        } else if (facingRight_ && !airborne_ && heroX_ >= turnBack()) {
            // Dive back in: a single arc that ends inside the home.
            stage_ = Stage::GoingHome;
            airborne_ = true;
            heroVy_ = 330;
            const float flight = (heroVy_ + std::sqrt(heroVy_ * heroVy_ - 2 * gravity * hidden)) / gravity;
            heroVx_ = (homeX() - heroX_) / flight;
        }
        const bool hovered = config_.interaction && cursor.inside
            && std::abs(cursor.position.x - heroX_) < heroHalfWidth + 4
            && cursor.position.y > ground() - heroLift_ - heroHeight - 6 && cursor.position.y < ground() - heroLift_ + 4;
        if (hovered && !airborne_) {
            hop(300);
            burst(Artwork::PixelSparkle, {heroX_, ground() - heroHeight - 8});
        }
        break;
    }
    case Stage::Home: break;
    }
}
void RetroEffect::moveCoins(float dt, CursorSample cursor) {
    const std::size_t count = std::size_t(std::clamp(width_ * height_ / 40000.F, 4.F, 28.F)
                                          * coinDensity(config_.ambientIntensity) * load_);
    while (coins_.size() < count) { coins_.emplace_back(); spawnCoin(coins_.back(), true); }
    coins_.resize(count);
    const bool heroOut = stage_ != Stage::Home && heroLift_ > hidden + 8;
    const float heroTop = ground() - heroLift_ - heroHeight, heroBottom = ground() - heroLift_;
    const bool reach = config_.interaction && cursor.inside
        && std::isfinite(cursor.position.x) && std::isfinite(cursor.position.y);
    for (auto& coin : coins_) {
        coin.phase += coin.spin * dt;
        coin.point.y += coin.speed * dt;
        coin.point.x += (40 * config_.wind + 9 * std::sin(time_ * 1.3F + coin.sway)) * dt;
        if (coin.point.x < -10) coin.point.x += width_ + 20;
        if (coin.point.x > width_ + 10) coin.point.x -= width_ + 20;
        bool collected = heroOut && std::abs(coin.point.x - heroX_) < heroHalfWidth
            && coin.point.y > heroTop && coin.point.y < heroBottom;
        if (!collected && reach) {
            const float dx = coin.point.x - cursor.position.x, dy = coin.point.y - cursor.position.y;
            collected = dx * dx + dy * dy < 15 * 15;
        }
        if (collected) {
            burst(Artwork::PixelSparkle, coin.point);
            spawnCoin(coin, false);
        } else if (coin.point.y > height_ + 14) {
            spawnCoin(coin, false);
        }
    }
}
void RetroEffect::advance(float seconds, CursorSample cursor) {
    if (!needsAnimation()) { clear(); return; }
    const float dt = std::clamp(seconds, 0.F, .1F);
    time_ += dt;
    moveHero(dt, cursor);
    moveCoins(dt, cursor);
    for (auto& b : bursts_) if (b.age >= 0) { b.age += dt; if (b.age > .42F) b.age = -1; }
    draw();
}
Artwork RetroEffect::heroFrame() const {
    const auto& set = heroes[std::size_t(config_.retroHero)];
    if (airborne_) return set[3];
    if (stage_ != Stage::Running) return set[0];
    return (int(runClock_ / .11F) % 2) ? set[2] : set[1];
}
void RetroEffect::push(Artwork art, float centerX, float bottomY, float scale, bool mirror) {
    const auto* sprite = pixel::spriteFor(art);
    if (!sprite) return;
    const float w = float(sprite->width + 2 * pixel::padding) * scale;
    const float h = float(sprite->height + 2 * pixel::padding) * scale;
    const float pivotY = float(sprite->height + pixel::padding) / float(sprite->height + 2 * pixel::padding);
    frame_.illustrations.push_back({art, snap(centerX), snap(bottomY), mirror ? -w : w, h, .5F, pivotY});
}
void RetroEffect::draw() {
    frame_.illustrations.clear();
    for (const auto& coin : coins_) {
        static constexpr std::array spin{Artwork::PixelCoinFace, Artwork::PixelCoinTurn, Artwork::PixelCoinEdge, Artwork::PixelCoinTurn};
        const int step = int(std::fmod(coin.phase, 4.F));
        push(spin[std::size_t(std::clamp(step, 0, 3))], coin.point.x, coin.point.y + 5 * coinPixel, coinPixel, step == 3);
    }
    if (stage_ != Stage::Home)
        push(heroFrame(), heroX_, ground() - heroLift_, pixel, !facingRight_);
    push(homes[std::size_t(config_.retroHero)], homeX(), ground(), pixel);
    for (const auto& b : bursts_) {
        if (b.age < 0) continue;
        if (b.art == Artwork::PixelDust)
            push(b.art, b.point.x + b.direction * b.age * 30, b.point.y, pixel * std::max(.35F, 1 - b.age * 1.6F), b.direction < 0);
        else
            push(b.art, b.point.x, b.point.y + 4.5F * coinPixel - b.age * 10, coinPixel * std::max(.4F, 1.25F - b.age * 1.8F));
    }
}
}
