#include "effects/AquariumEffect.h"
#include "effects/BlackHoleEffect.h"
#include "effects/CatEffect.h"
#include "effects/EffectFactory.h"
#include "effects/ParadeEffect.h"
#include "effects/SceneSprites.h"
#include "effects/SnowEffect.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
int checks = 0;
void check(bool condition, std::string_view message) {
    ++checks;
    if (!condition) throw std::runtime_error(std::string(message));
}
te::EffectConfig config(te::EffectKind kind, int intensity = 1) {
    te::EffectConfig c;
    c.kind = kind;
    c.ambientIntensity = c.intensity = intensity;
    c.flightPause = 2;
    return c;
}
void sane(const std::vector<te::ShapeVertex>& shapes, std::string_view message) {
    check(shapes.size() % 3 == 0, "shapes are whole triangles");
    check(std::all_of(shapes.begin(), shapes.end(), [](const te::ShapeVertex& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && v.r <= v.a && v.g <= v.a && v.b <= v.a;
    }), message);
}
// Nothing may leave the strip above the bottom edge.
void confined(const std::vector<te::ShapeVertex>& shapes, float width, float height, float strip, std::string_view message) {
    check(std::all_of(shapes.begin(), shapes.end(), [&](const te::ShapeVertex& v) {
        return v.y >= height - strip - 1 && v.y <= height + 40 && v.x >= -80 && v.x <= width + 80;
    }), message);
}

void testSprites() {
    for (const auto* art : {&te::art::catSit, &te::art::catSitWag, &te::art::catBlink, &te::art::catHappy, &te::art::catWalkA, &te::art::catWalkB,
            &te::art::catSleepA, &te::art::catSleepB, &te::art::catPaw, &te::art::yarnBall, &te::art::heart, &te::art::letterZ, &te::art::tree, &te::art::smallTree,
            &te::art::snowman, &te::art::giftBox, &te::art::planet, &te::art::ringed, &te::art::rocket, &te::art::saucer, &te::art::clown, &te::art::tang,
            &te::art::minnow, &te::art::puffer, &te::art::pufferBig, &te::art::crabA, &te::art::crabB, &te::art::chestClosed, &te::art::chestOpen, &te::art::coral,
            &te::art::eagleUp, &te::art::eagleDown, &te::art::truckA, &te::art::truckB, &te::art::riderA, &te::art::riderB, &te::art::astroA, &te::art::astroB,
            &te::art::hotDogA, &te::art::hotDogB, &te::art::tinyFlag, &te::art::musicNote}) {
        check(int(art->rows.size()) == art->width * art->height, "sprite grids are rectangular");
        check(float(art->height) * te::PixelCanvas::pixel <= 32, "sprites keep the size of the 8-bit heroes");
    }
    std::vector<te::ShapeVertex> out;
    te::PixelCanvas canvas(out);
    constexpr te::PixelArt probe{3, 2, "aa.b.."};
    constexpr std::array<te::Tint, 2> tints{{{'a', 0xff0000}, {'b', 0x00ff00}}};
    canvas.sprite(probe, tints, 10.4F, 20.3F);
    check(out.size() == 12, "equal neighbours share one quad");
    check(std::all_of(out.begin(), out.end(), [](const te::ShapeVertex& v) { return v.x == std::round(v.x) && v.y == std::round(v.y); }), "pixels snap to whole points");
    const float left = out.front().x;
    out.clear();
    canvas.sprite(probe, tints, 10.4F, 20.3F, true);
    check(out.front().x == left + 2, "sprites mirror in place");
    sane(out, "sprite geometry");
}

void testDevicePixelGrid() {
    constexpr te::Vec2 origin{.3F, 9.4F};
    constexpr std::array<te::Vec2, 4> diamond{{{8, 1}, {14, 9}, {8, 17}, {2, 9}}};
    for (const float scale : {1.F, 1.25F, 1.5F}) {
        std::vector<te::ShapeVertex> out;
        te::PixelCanvas canvas(out, scale, origin);
        canvas.pixelPolygon(diamond, 1.5F, 0xf4c5ef, te::pixelOpacity(.73F));
        canvas.rect(4.1F, 7.3F, 2.F, 2.F, 0xffffff);
        check(out.size() > 12 && out.size() % 6 == 0, "sloped edges rasterize to rows of pixel quads");
        for (const auto& corner : out) {
            const float x = (corner.x + origin.x) * scale, y = (corner.y + origin.y) * scale;
            check(std::abs(x - std::round(x)) < .0001F && std::abs(y - std::round(y)) < .0001F,
                  "every corner lands at a physical pixel edge at fractional scale");
        }
        check(out.front().a == 191, "fade uses a single quantized alpha across the outline");
        sane(out, "pixel colors use premultiplied alpha");
    }
}

void testParade() {
    check(te::art::ladyLiberty.width * te::art::ladyLiberty.height == int(te::art::ladyLiberty.rows.size())
          && float(te::art::ladyLiberty.height) * te::PixelCanvas::pixel <= te::edgeStrip, "the statue fits into the strip");
    te::ParadeEffect parade(11);
    auto c = config(te::EffectKind::UnitedStates, 2);
    c.flightLimit = 3;
    c.wind = 1;
    parade.configure(c);
    parade.resize(1000, 640);
    std::size_t most = 0, rockets = 0;
    bool seen = false;
    for (int i = 0; i < 4000; ++i) {
        te::CursorSample cursor;
        if (parade.guestCount() > 0 && i % 7 == 0) cursor = {parade.guestPoint(0), true};
        parade.advance(.05F, cursor);
        if (i % 90 == 0 && parade.guestCount() > 0) seen = parade.pressed(parade.guestPoint(0)) || seen;
        most = std::max(most, parade.guestCount());
        rockets = std::max(rockets, parade.rocketCount());
        check(parade.guestCount() <= 3 && parade.rocketCount() <= 10 && parade.confettiCount() <= 48, "the parade stays small");
        confined(parade.frame().overlay, 1000, 640, te::edgeStrip, "the parade stays in the bottom strip");
        sane(parade.frame().overlay, "parade geometry");
    }
    check(most == 3 && rockets >= 2 && seen, "guests come together and answer with fireworks");
    // A greeting needs the cursor and the interaction setting.
    te::ParadeEffect shy(5);
    shy.configure(c); shy.resize(1000, 640);
    for (int i = 0; i < 400 && shy.guestCount() == 0; ++i) shy.advance(.05F, {});
    check(shy.guestCount() > 0 && shy.pressable(), "the first guests arrive soon");
    check(!shy.pressed({500, 20}), "empty places do not answer");
    auto off = c; off.interaction = false; shy.configure(off);
    for (int i = 0; i < 40; ++i) shy.advance(.05F, {shy.guestPoint(0), true});
    check(shy.guestCheer(0) == 0 && !shy.pressable() && !shy.pressed(shy.guestPoint(0)), "interaction can be switched off");
    shy.configure(c);
    shy.advance(.05F, {shy.guestPoint(0), true});
    check(shy.guestCheer(0) > 0 && shy.confettiCount() > 0, "a hovered guest greets with confetti");
    // Wind decides the side the column comes from.
    for (float wind : {-1.F, 1.F}) {
        te::ParadeEffect column(3);
        auto windy = c; windy.wind = wind; column.configure(windy); column.resize(900, 500);
        for (int i = 0; i < 400 && column.guestCount() == 0; ++i) column.advance(.02F, {});
        check((column.guestPoint(0).x < 0) == (wind > 0), "the wind chooses the direction");
    }
    parade.resize(300, 200);
    parade.advance(.05F, {});
    confined(parade.frame().overlay, 300, 200, te::edgeStrip, "a resize keeps the strip");
    c.enabled = false; parade.configure(c);
    check(parade.frame().overlay.empty() && parade.guestCount() == 0 && !parade.needsAnimation(), "the parade can be switched off");
}
void testFestiveSnow() {
    te::SnowEffect snow(4);
    auto c = config(te::EffectKind::Snow);
    c.maxDepth = 24;
    snow.configure(c);
    snow.resize(1000, 640);
    snow.advance(.05F, {});
    check(snow.frame().overlay.empty() && snow.frame().underlay.empty(), "nothing hangs over the chat");
    for (int i = 0; i < 1500; ++i) {
        snow.advance(.1F, {});
        check(snow.frame().overlay.empty(), "the snowfall keeps its plain flakes");
        confined(snow.frame().underlay, 1000, 640, te::edgeStrip, "figures stay in the bottom strip");
    }
    check(!snow.frame().underlay.empty() && !snow.frame().detail.empty(), "figures and lights appear on textured snow");
    float highest = 0;
    for (const auto& s : snow.frame().snow) highest = std::max(highest, s.height);
    confined(snow.frame().detail, 1000, 640, highest + 8, "the texture stays on the drift");
    sane(snow.frame().underlay, "figure geometry"); sane(snow.frame().detail, "snow texture");
    const auto decorated = snow.frame().detail.size();
    c.festive = false; snow.configure(c); snow.advance(.05F, {});
    check(snow.frame().underlay.empty() && !snow.frame().detail.empty() && snow.frame().detail.size() < decorated, "decorations can be switched off, the texture stays");
    te::SnowEffect deep(4);
    c.festive = true; c.maxDepth = 120; deep.configure(c); deep.resize(1000, 640); deep.settle(90); deep.advance(.016F, {});
    float crest = 0;
    for (const auto& s : deep.frame().snow) crest = std::max(crest, s.height);
    confined(deep.frame().underlay, 1000, 640, std::max(te::edgeStrip, crest + 12), "deep snow buries the figures");
}

void testBlackHole() {
    te::BlackHoleEffect hole(3), few(3), many(3);
    hole.configure(config(te::EffectKind::BlackHole)); hole.resize(1000, 640);
    few.configure(config(te::EffectKind::BlackHole, 0)); few.resize(1000, 640); few.advance(.02F, {});
    many.configure(config(te::EffectKind::BlackHole, 2)); many.resize(1000, 640); many.advance(.02F, {});
    check(few.starCount() < many.starCount(), "intensity controls the stars");
    const auto run = [&](int frames, bool fed) {
        for (int i = 0; i < frames; ++i) {
            hole.advance(1.F / 60, fed ? te::CursorSample{hole.center(), true} : te::CursorSample{});
            confined(hole.frame().overlay, 1000, 640, te::edgeStrip, "the hole, stars and sparks stay in the strip");
        }
    };
    run(60, false);
    check(hole.center().x > 900 && hole.center().y > 600, "the hole sits in the corner");
    run(80, true);
    check(hole.charge() > .6F, "a resting cursor feeds the hole");
    run(1, false);
    check(hole.erupting() && hole.charge() == 0, "letting go releases sparks");
    run(90, false);
    check(!hole.erupting(), "the burst ends");
    bool burst = false;
    for (int i = 0; i < 300; ++i) { run(1, true); burst |= hole.erupting(); }
    check(burst, "an overfed hole bursts by itself");
    bool travel = false;
    for (int i = 0; i < 60 * 30; ++i) { run(1, false); travel |= hole.travelling(); }
    check(travel, "travellers fly in");
    auto c = config(te::EffectKind::BlackHole); c.interaction = false;
    te::BlackHoleEffect passive(3); passive.configure(c); passive.resize(1000, 640);
    for (int i = 0; i < 90; ++i) passive.advance(1.F / 60, {passive.center(), true});
    check(passive.charge() == 0, "interaction can be switched off");
}

void testCat() {
    te::CatEffect cat(8);
    cat.configure(config(te::EffectKind::Cat, 2)); cat.resize(1000, 640);
    check(cat.ballCount() == 3, "intensity controls the yarn");
    for (int i = 0; i < 30; ++i) cat.advance(1.F / 60, {});
    const auto ball = cat.ball(0);
    for (int i = 0; i < 8; ++i) cat.advance(1.F / 60, {{ball.x - 30 + 8 * float(i), ball.y}, true});
    cat.advance(1.F / 60, {});
    check(cat.ball(0).x > ball.x && cat.mode() == te::CatEffect::Mode::Chase, "pushed yarn rolls and the cat gives chase");
    const float before = cat.x();
    bool slept = false, walked = false;
    for (int i = 0; i < 60 * 120; ++i) {
        cat.advance(1.F / 60, {});
        if (i == 60) check(std::abs(cat.x() - before) > 20, "the cat runs");
        check(cat.x() >= 0 && cat.x() <= 1000, "the cat stays inside the window");
        confined(cat.frame().overlay, 1000, 640, te::edgeStrip, "the cat and yarn stay in the strip");
        slept |= cat.mode() == te::CatEffect::Mode::Sleep; walked |= cat.mode() == te::CatEffect::Mode::Walk;
    }
    check(walked, "the cat strolls on its own");
    check(slept, "without a moving cursor the cat falls asleep");
    te::CatEffect pet(5);
    pet.configure(config(te::EffectKind::Cat)); pet.resize(1000, 640);
    for (int i = 0; i < 150; ++i) {
        pet.advance(1.F / 60, {{pet.x() + 8 * std::sin(float(i) * .4F), pet.ground() - 16}, true});
        confined(pet.frame().overlay, 1000, 640, te::edgeStrip, "hearts stay in the strip");
    }
    check(pet.purr() > .5F && pet.heartCount() > 0, "stroking makes it purr");
    te::CatEffect playful(5);
    playful.configure(config(te::EffectKind::Cat)); playful.resize(1000, 640);
    bool swat = false;
    for (int i = 0; i < 90; ++i) {
        playful.advance(1.F / 60, {{playful.x() - 28 + float(i % 4) * 2, playful.ground() - 24}, true}); swat |= playful.swatting();
        confined(playful.frame().overlay, 1000, 640, te::edgeStrip, "the paw stays in the strip");
    }
    check(swat, "a fidgeting cursor nearby gets swatted");
}

void testAquarium() {
    te::AquariumEffect tank(6);
    tank.configure(config(te::EffectKind::Aquarium)); tank.resize(1000, 640);
    bool bubbles = false;
    for (int i = 0; i < 60 * 60; ++i) {
        tank.advance(1.F / 60, {});
        bubbles |= tank.bubbleCount() > 0;
        confined(tank.frame().overlay, 1000, 640, te::edgeStrip, "sea life stays in the strip");
    }
    check(bubbles, "bubbles rise");
    const auto shy = tank.fishPoint(0);
    const auto inside = std::clamp(shy.x, 200.F, 800.F);
    if (inside == shy.x) {
        for (int i = 0; i < 40; ++i) tank.advance(1.F / 60, {shy, true});
        check(std::abs(tank.fishPoint(0).x - shy.x) > 25, "fish flee from the cursor");
    }
    for (int i = 0; i < 30; ++i) { tank.advance(1.F / 60, {tank.pufferPoint(), true}); confined(tank.frame().overlay, 1000, 640, te::edgeStrip, "a puffed fish stays in the strip"); }
    check(tank.puffed() > .5F, "the puffer inflates");
    const auto chest = tank.chestPoint();
    for (int i = 0; i < 60; ++i) tank.advance(1.F / 60, {{chest.x, chest.y - 4}, true});
    check(tank.chestOpen() > .9F, "the chest opens under the cursor");
    for (int i = 0; i < 240; ++i) tank.advance(1.F / 60, {});
    check(tank.chestOpen() < .1F && tank.puffed() < .3F, "everything calms down");
    te::AquariumEffect few(2), many(2);
    few.configure(config(te::EffectKind::Aquarium, 0)); few.resize(1000, 640); few.advance(.02F, {});
    many.configure(config(te::EffectKind::Aquarium, 2)); many.resize(1000, 640); many.advance(.02F, {});
    check(few.fishCount() < many.fishCount() && many.fishCount() < 12, "intensity controls the fish");
}

void testLifecycle() {
    constexpr std::array kinds{te::EffectKind::Snow, te::EffectKind::BlackHole, te::EffectKind::Cat, te::EffectKind::Aquarium};
    for (auto kind : kinds) {
        auto effect = te::createEffect(kind);
        auto c = config(kind, 2);
        effect->configure(c);
        effect->advance(.016F, {});
        check(kind == te::EffectKind::Snow || !effect->needsAnimation(), "a scene without a size does not animate");
        for (auto size : {te::Vec2{900, 600}, te::Vec2{240, 180}, te::Vec2{3000, 1800}, te::Vec2{40, 30}, te::Vec2{0, 0}, te::Vec2{700, 500}}) {
            effect->resize(size.x, size.y);
            effect->setLoadScale(.3F);
            for (int i = 0; i < 30; ++i) effect->advance(i == 7 ? 5.F : .05F, {{size.x * .5F + float(i), size.y * .5F}, i % 3 != 0});
            effect->advance(.016F, {{std::numeric_limits<float>::quiet_NaN(), 0}, true});
            sane(effect->frame().overlay, "overlay geometry survives any size");
            sane(effect->frame().underlay, "underlay geometry survives any size");
            sane(effect->frame().detail, "detail geometry survives any size");
            check(effect->frame().overlay.size() + effect->frame().detail.size() + effect->frame().underlay.size() < 60000, "geometry stays within budget");
        }
        check(effect->needsAnimation() && (kind == te::EffectKind::Snow || !effect->frame().overlay.empty()), "an enabled scene draws");
        c.enabled = false;
        effect->configure(c);
        effect->advance(.016F, {});
        check(effect->frame().overlay.empty(), "a disabled scene draws nothing");
    }
    te::EffectConfig c;
    c.catCoat = 99; c.normalize();
    check(c.catCoat == 3, "coat index is clamped");
}
}

int main() {
    try {
        testSprites(); testDevicePixelGrid(); testParade(); testFestiveSnow(); testBlackHole(); testCat(); testAquarium(); testLifecycle();
        std::cout << "PASS: " << checks << " assertions (sprites, New Year, black hole, cat, aquarium, lifecycle)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
