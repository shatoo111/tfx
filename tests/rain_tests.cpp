#include "effects/EffectFactory.h"
#include "effects/RainEffect.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void check(bool condition, const char* description) {
    if (!condition) { std::cerr << description << '\n'; std::exit(1); }
}
void advance(te::IEffect& effect, int frames) {
    for (int i = 0; i < frames; ++i) effect.advance(1.F / 60.F, {});
}
}
int main() {
    te::EffectConfig config;
    config.kind = te::EffectKind::Rain;
    config.wind = 0;
    std::size_t previousCount = 0;
    for (int intensity = 0; intensity < 3; ++intensity) {
        te::RainEffect rain(43);
        config.rainIntensity = intensity;
        rain.configure(config); rain.resize(1200, 600); advance(rain, 60);
        check(rain.frame().rain.size() > previousCount, "Rain presets must increase density.");
        previousCount = rain.frame().rain.size();
        check(!rain.frame().grass.empty(), "Grass should grow along the lower edge.");
        check(rain.frame().snow.empty() && rain.frame().particles.empty(), "Rain must not produce snow.");
        check(rain.frame().flashes.empty(), "Lightning defaults to off.");
        config.enabled = false;
        rain.configure(config); advance(rain, 180);
        check(!rain.needsAnimation(), "Disabled rain must stop all animation.");
        check(rain.frame().rain.empty() && rain.frame().grass.empty() && rain.frame().splashes.empty(),
              "Disabled rain must fade out.");
        config.enabled = true;
    }
    te::RainEffect reference(23);
    config.enabled = true;
    config.rainIntensity = 2;
    config.wind = 0.28F;
    config.grass = false;
    reference.configure(config);
    reference.resize(626, 174);
    reference.advance(1.F / 60.F, {});
    check(reference.frame().rain.size() == 52, "Downpour density must match the settings preview.");
    const auto firstFrame = reference.frame().rain;
    reference.advance(1.F / 60.F, {});
    for (std::size_t i = 0; i < firstFrame.size(); ++i) {
        const auto& first = firstFrame[i];
        if (first.position.y < 0 || first.position.y > 150 || first.position.x > 646) continue;
        const auto& second = reference.frame().rain[i];
        const float fall = second.position.y - first.position.y;
        check(fall > 3.1F && fall < 5.5F, "Drops must move at the preview's speed at 60 fps.");
        check(std::abs(second.position.x - first.position.x - fall * 0.182F) < 0.001F,
              "Drops must follow the preview's wind angle.");
    }
    config.grass = true;
    reference.configure(config);
    for (int i = 0; i < 120 && reference.frame().splashes.empty(); ++i) advance(reference, 1);
    check(!reference.frame().splashes.empty(), "Drops that hit grass must produce visible splashes.");
    const auto splash = reference.frame().splashes.front();
    check(splash.position.y > 154.F && splash.position.y < 174.F,
          "A splash must start at the grass surface.");
    advance(reference, 1);
    const auto moved = std::ranges::any_of(reference.frame().splashes, [&](const te::SplashVisual& next) {
        return std::abs(next.position.x - splash.position.x + 22.F / 60.F) < 0.001F
            && std::abs(next.position.y - splash.position.y + 28.F / 60.F) < 0.001F
            && next.opacity < splash.opacity;
    });
    check(moved, "A splash must move up and left and fade like the settings preview.");
    config.grass = false;
    reference.configure(config);
    advance(reference, 9);
    check(reference.frame().splashes.empty(), "Splashes must expire within 0.14 seconds without new grass impacts.");
    config.grass = true;
    config.wind = 0;
    te::RainEffect natural(91), touched(91), ignored(91);
    config.rainIntensity = 0; config.grassHeight = 120;
    for (auto* effect : {&natural, &touched, &ignored}) { effect->configure(config); effect->resize(600, 400); }
    config.interaction = false; ignored.configure(config); config.interaction = true;
    for (int i = 0; i < 60; ++i) {
        te::CursorSample cursor{{100.F + float(i) * 6.F, 350}, true};
        natural.advance(1.F / 60.F, {});
        touched.advance(1.F / 60.F, cursor);
        ignored.advance(1.F / 60.F, cursor);
    }
    float movement = 0;
    for (std::size_t i = 0; i < natural.frame().grass.size(); ++i) {
        const auto& a = natural.frame().grass[i];
        const auto& b = touched.frame().grass[i];
        movement += std::abs(a.tip.x - b.tip.x);
        check(std::abs(a.tip.x - ignored.frame().grass[i].tip.x) < 0.001F, "Disabling interaction must ignore the cursor.");
        check(a.tip.y >= 300, "Grass must respect the quarter-window height cap.");
    }
    check(movement > 20, "Moving the cursor through grass must bend the blades.");
    config.grass = false; touched.configure(config); advance(touched, 60);
    check(touched.frame().grass.empty() && !touched.frame().rain.empty(), "Grass toggle must leave rain enabled.");
    config.grass = true; config.grassHeight = 0; touched.configure(config); advance(touched, 60);
    check(touched.frame().grass.empty(), "Zero grass height must hide grass.");

    te::RainEffect drizzle(73), downpour(73);
    config.grassHeight = 48;
    config.rainIntensity = 0; drizzle.configure(config);
    config.rainIntensity = 2; downpour.configure(config);
    for (auto* effect : {&drizzle, &downpour}) {
        effect->resize(800, 600);
        advance(*effect, 120);
    }
    bool deflectedByRain = false;
    for (std::size_t i = 0; i < drizzle.frame().grass.size(); ++i)
        deflectedByRain |= std::abs(drizzle.frame().grass[i].tip.x - downpour.frame().grass[i].tip.x) > 1.F;
    check(deflectedByRain, "Raindrop impacts must bend the grass without wind or cursor input.");

    config.rainIntensity = 0;
    config.lightning = true; config.flashInterval = 5; config.grassHeight = 48;
    te::RainEffect storm(115);
    storm.configure(config); storm.resize(800, 600);
    bool flashing = false;
    int events = 0, lastFrame = 0;
    float peak = 0.F, lastOpacity = 0.F;
    for (int i = 0; i < 3000; ++i) {
        storm.advance(1.F / 60.F, {});
        const auto& flashes = storm.frame().flashes;
        if (!flashes.empty()) {
            const auto& flash = flashes.front();
            check(flash.opacity <= 0.1201F, "Lightning must stay muted.");
            check(std::abs(flash.center.x - 568.F) < 0.001F && flash.center.y == 0.F && flash.radius == 600.F,
                  "Lightning must use the preview's broad radial gradient.");
            check(std::abs(flash.opacity - lastOpacity) < 0.012F, "Lightning must brighten and fade gradually.");
            peak = std::max(peak, flash.opacity);
            lastOpacity = flash.opacity;
            if (!flashing) {
                if (events > 0) {
                    check(i - lastFrame >= 224 && i - lastFrame <= 376, "Lightning interval must stay within its jitter range.");
                }
                ++events; lastFrame = i;
            }
        } else {
            lastOpacity = 0.F;
        }
        flashing = !flashes.empty();
    }
    check(events >= 7, "Lightning must work during light rain.");
    check(peak > 0.119F, "Lightning must reach the preview's brightness smoothly.");
    for (int intensity : {1, 2}) {
        te::RainEffect heavier(67);
        config.rainIntensity = intensity;
        heavier.configure(config); heavier.resize(800, 600);
        bool sawFlash = false;
        for (int i = 0; i < 450; ++i) {
            heavier.advance(1.F / 60.F, {});
            sawFlash = sawFlash || !heavier.frame().flashes.empty();
        }
        check(sawFlash, "Lightning must also work with normal rain and downpour.");
    }
    config.lightning = false; storm.configure(config); advance(storm, 600);
    check(storm.frame().flashes.empty(), "Disabling lightning must remove its flashes.");
    config.grassHeight = std::numeric_limits<float>::quiet_NaN();
    config.flashInterval = std::numeric_limits<float>::infinity();
    config.rainIntensity = 99; config.normalize();
    check(config.grassHeight == 48 && config.flashInterval == 15 && config.rainIntensity == 2, "Rain settings must normalize invalid values.");
    storm.configure(config); storm.resize(32768, 32768); advance(storm, 120);
    check(storm.frame().rain.size() <= te::RainEffect::dropLimit && storm.frame().splashes.size() <= te::RainEffect::splashLimit,
          "Rain must respect its particle limit.");
    check(storm.frame().grass.size() <= te::RainEffect::bladeLimit, "Grass must respect its blade limit.");
    storm.resize(0, 0); storm.advance(std::numeric_limits<float>::quiet_NaN(), {});
    for (auto kind : {te::EffectKind::Snow, te::EffectKind::Rain}) {
        auto effect = te::createEffect(kind);
        effect->configure({}); effect->resize(600, 400); advance(*effect, 60);
        check(effect->needsAnimation(), "Both effect factory variants must animate.");
    }
    std::cout << "Rain, grass, interaction, lightning and lifecycle checks passed.\n";
}
