#include "effects/EffectFactory.h"
#include "effects/PixelSprites.h"
#include "effects/RetroEffect.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
int checks = 0;
void check(bool condition, std::string_view message) {
    ++checks;
    if (!condition) throw std::runtime_error(std::string(message));
}
using Stage = te::RetroEffect::Stage;
long countArt(const te::EffectFrame& frame, te::Artwork lo, te::Artwork hi) {
    return std::count_if(frame.illustrations.begin(), frame.illustrations.end(),
        [&](const te::Illustration& p) { return p.art >= lo && p.art <= hi; });
}
te::EffectConfig retro(int hero = 0) {
    te::EffectConfig config;
    config.kind = te::EffectKind::Retro;
    config.retroHero = hero;
    config.flightPause = 2;
    return config;
}

void testSprites() {
    for (int i = int(te::Artwork::PixelCoinFace); i < int(te::Artwork::Count); ++i) {
        const auto* sprite = te::pixel::spriteFor(te::Artwork(i));
        check(sprite != nullptr, "every pixel artwork has a sprite");
        check(int(sprite->rows.size()) == sprite->width * sprite->height, "sprite grids are rectangular");
        for (char key : sprite->rows)
            check(key == '.' || te::pixel::colorFor(key) != 0, "every sprite key has a colour");
    }
    check(te::pixel::spriteFor(te::Artwork::Tail) == nullptr, "vector artwork is not pixel art");
    check(te::isPixelArt(te::Artwork::CatIdle) && !te::isPixelArt(te::Artwork::Rays), "pixel art range");
}

void testTrip() {
    for (int hero = 0; hero < 3; ++hero) {
        te::RetroEffect effect(7);
        effect.configure(retro(hero));
        effect.resize(900, 600);
        check(effect.stage() == Stage::Home, "the hero starts at home");
        const te::Artwork home = hero == 0 ? te::Artwork::PixelCrate : hero == 1 ? te::Artwork::PixelWell : te::Artwork::PixelPad;
        effect.advance(.05F, {});
        check(countArt(effect.frame(), home, home) == 1, "the chosen hero's home is drawn");
        bool left = false, ran = false, turned = false, back = false;
        float leftmost = 1e9F;
        int laps = 0, wallTouches = 0;
        bool atWall = false;
        for (int i = 0; i < 60 * 120 && !back; ++i) {
            effect.advance(1.F / 60, {});
            const auto stage = effect.stage();
            if (stage == Stage::LeavingHome && !left) laps = effect.lapsLeft();
            left |= stage == Stage::LeavingHome;
            ran |= stage == Stage::Running;
            if (stage == Stage::Running || stage == Stage::GoingHome) leftmost = std::min(leftmost, effect.heroFeet().x);
            const bool touching = stage == Stage::Running && effect.heroFeet().x <= effect.laneEnd() + .5F;
            if (touching && !atWall) ++wallTouches;
            atWall = touching;
            if (stage == Stage::Running)
                check(effect.heroFeet().y <= effect.ground() + .01F, "the hero never sinks below the window edge while running");
            if (ran && stage == Stage::GoingHome) turned = true;
            if (turned && stage == Stage::Home) back = true;
        }
        check(left && ran && turned && back, "a full trip: out of home, run, return");
        check(laps >= 2 && laps <= 3, "a trip is two or three runs across the window");
        check(wallTouches == laps, "every run reaches the left wall");
        check(leftmost >= effect.laneEnd() - .5F && leftmost <= effect.laneEnd() + .5F, "the hero runs the full width to the left wall");
        check(effect.homeX() > 900 - 40, "home sits in the lower-right corner");
        check(countArt(effect.frame(), te::Artwork::CatIdle, te::Artwork::RobotJump) == 0, "a hero at home is hidden");
    }
}

void testLaneAndResize() {
    te::RetroEffect effect(3);
    effect.configure(retro());
    effect.resize(2400, 900);
    check(effect.laneEnd() < 30 && effect.homeX() > 2400 - 40, "wide windows: home on the right, lane to the left wall");
    effect.resize(260, 300);
    check(effect.laneEnd() < effect.turnBack() && effect.turnBack() < effect.homeX(), "narrow windows keep a usable lane");
}

void testCoinsAndInteraction() {
    te::RetroEffect sparse(5), dense(5);
    auto config = retro();
    config.ambientIntensity = 0; sparse.configure(config);
    config.ambientIntensity = 2; dense.configure(config);
    sparse.resize(1000, 700); dense.resize(1000, 700);
    sparse.advance(.016F, {}); dense.advance(.016F, {});
    const auto coins = [](const te::EffectFrame& f) { return countArt(f, te::Artwork::PixelCoinFace, te::Artwork::PixelCoinEdge); };
    check(coins(dense.frame()) > coins(sparse.frame()) && coins(sparse.frame()) >= 4, "intensity controls coin density");

    te::RetroEffect effect(11);
    effect.configure(retro(1));
    effect.resize(900, 600);
    for (int i = 0; i < 600 && !(effect.stage() == Stage::Running && effect.heroFeet().y >= effect.ground() - .01F); ++i)
        effect.advance(1.F / 60, {});
    check(effect.stage() == Stage::Running, "the hero is out running");
    const auto feet = effect.heroFeet();
    effect.advance(1.F / 60, {{feet.x, feet.y - 12}, true});
    effect.advance(1.F / 60, {});
    check(effect.heroFeet().y < feet.y - 2, "hovering the hero makes it hop");

    config = retro(); config.interaction = false;
    te::RetroEffect calm(11);
    calm.configure(config); calm.resize(900, 600);
    for (int i = 0; i < 600 && calm.stage() != Stage::Running; ++i) calm.advance(1.F / 60, {});
    for (int i = 0; i < 4 && calm.heroFeet().y < calm.ground() - .01F; ++i) calm.advance(1.F / 60, {});
    const auto still = calm.heroFeet();
    calm.advance(1.F / 60, {{still.x, still.y - 12}, true});
    check(std::abs(calm.heroFeet().y - still.y) < .01F || calm.heroFeet().y >= still.y, "interaction can be switched off");
}

void testLifecycle() {
    auto effect = te::createEffect(te::EffectKind::Retro);
    auto config = retro();
    effect->configure(config);
    effect->resize(800, 500);
    check(effect->needsAnimation(), "an enabled theme animates");
    effect->advance(.5F, {});
    check(!effect->frame().illustrations.empty(), "frames contain pixel art");
    config.enabled = false;
    effect->configure(config);
    check(!effect->needsAnimation(), "a disabled theme stops");
    effect->advance(.016F, {});
    check(effect->frame().illustrations.empty(), "a disabled theme draws nothing");
    config.retroHero = 9; config.normalize();
    check(config.retroHero == 2, "hero index is clamped");
}
}

int main() {
    try {
        testSprites(); testTrip(); testLaneAndResize(); testCoinsAndInteraction(); testLifecycle();
        std::cout << "PASS: " << checks << " assertions (sprites, trip, lane, coins, interaction, lifecycle)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
