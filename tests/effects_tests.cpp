#include "effects/SnowEffect.h"
#include "effects/SnowField.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {
int checks = 0;
void check(bool condition, std::string_view message) {
    ++checks;
    if (!condition) throw std::runtime_error(std::string(message));
}
void run(te::SnowEffect& snow, int frames, float dt = 1.F / 60.F) {
    for (int i = 0; i < frames; ++i) snow.advance(dt, {});
}
float snowTotal(const te::EffectFrame& frame) {
    float result = 0;
    for (const auto& p : frame.snow) result += p.height;
    return result;
}
void testField() {
    te::SnowField field;
    field.resize(800); field.setMaximum(50);
    for (int i = 0; i < 10000; ++i) field.deposit(float(i % 800), 0.4F);
    check(field.volume() > 0, "deposition forms a drift");
    check(field.heightAt(-500) >= 0 && field.heightAt(10000) <= 50, "edge samples are clamped");
    for (float h : field.heights()) check(h <= 50 && h >= 0, "maximum depth is respected");
    const auto [lowest, highest] = std::minmax_element(field.heights().begin(), field.heights().end());
    check(*highest - *lowest > 5, "mature drifts retain an uneven silhouette at their height limit");
    const float before = field.volume();
    check(field.sweep(400, 80, 15) > 0, "cursor removes snow");
    check(field.volume() < before, "sweep reduces volume");
    const auto center = field.heightAt(400);
    field.resize(1600);
    check(std::abs(field.heightAt(800) - center) < 0.01F, "resizing preserves relative shape");
    field.setMaximum(8);
    for (float h : field.heights()) check(h <= 8, "lowering depth immediately clamps the surface");
    for (int i = 0; i < 2400; ++i) field.advance(1.F / 60.F, true);
    check(field.empty(), "melting reaches exactly zero, allowing animation to stop");
    field.deposit(800, 1.F);
    check(!field.empty() && field.sweep(800, 100, 2.F) > 0, "deposition revives a melted field");
    check(field.empty(), "complete sweep clears the field immediately");
    field.deposit(800, 1.F);
    field.setMaximum(0.F);
    check(field.empty() && field.heightAt(800) == 0.F, "zero maximum clears a previously occupied field");
    field.resize(0); field.deposit(0, 1);
    check(field.heightAt(0) == 0, "zero size is safe");
}
void testSimulation() {
    te::SnowEffect snow(42);
    snow.resize(1000, 700);
    te::EffectConfig config;
    config.intensity = 2; config.maxDepth = 40;
    snow.configure(config);
    run(snow, 60 * 90);
    check(!snow.frame().particles.empty(), "snowfall is alive");
    check(snowTotal(snow.frame()) > 10, "falling particles accumulate at the bottom");
    for (const auto& p : snow.frame().particles) {
        check(std::isfinite(p.position.x) && std::isfinite(p.position.y), "particles remain finite");
        check(p.opacity >= 0 && p.opacity <= 1, "opacity remains normalized");
    }
    for (auto p : snow.frame().snow) check(p.height >= 0 && p.height <= 40, "drifts obey configured maximum");
    snow.clear();
    check(snow.frame().snow.empty(), "clear removes drift immediately");
    run(snow, 60 * 10);
    config.enabled = false;
    snow.configure(config);
    run(snow, 60 * 90);
    check(snow.frame().particles.empty() && snow.frame().snow.empty(), "disabled effect fully fades and melts");
    check(!snow.needsAnimation(), "disabled and empty effect stops requesting frames");
    config.enabled = true; config.accumulation = false;
    snow.configure(config); run(snow, 600);
    check(!snow.frame().particles.empty(), "effect resumes after disabling");
    check(snow.frame().snow.empty(), "disabled accumulation stays empty");
}
void testBoundsAndModes() {
    std::size_t counts[3]{};
    for (int mode = 0; mode < 3; ++mode) {
        te::SnowEffect snow(7); te::EffectConfig config;
        config.intensity = mode; snow.configure(config); snow.resize(1000, 720); run(snow, 30);
        counts[mode] = snow.frame().particles.size();
    }
    check(counts[0] < counts[1] && counts[1] < counts[2], "presets have distinct densities");
    te::SnowEffect snow(7); snow.resize(16000, 16000); run(snow, 300);
    check(snow.frame().particles.size() <= te::SnowEffect::particleLimit, "particle budget is bounded on large displays");
    check(snow.frame().snow.size() <= 1025, "surface budget is bounded");
    const auto count = snow.frame().particles.size();
    snow.setLoadScale(0.3F); snow.resize(1000, 720); run(snow, 180);
    check(snow.frame().particles.size() < count, "dynamic load reduction retires excess particles");
    snow.resize(0, 0); snow.advance(0, {}); snow.advance(-1, {});
    snow.resize(std::numeric_limits<float>::quiet_NaN(), 100);
    snow.advance(std::numeric_limits<float>::infinity(), {});
    te::EffectConfig bad; bad.intensity = 500; bad.wind = std::numeric_limits<float>::quiet_NaN(); bad.maxDepth = -5;
    bad.normalize();
    check(bad.intensity == 2 && bad.wind == 0 && bad.maxDepth == 0, "invalid settings are normalized");
}
void testCursorAndTiming() {
    te::SnowEffect passive(99), active(99);
    passive.resize(800, 600); active.resize(800, 600);
    run(passive, 60); run(active, 60);
    auto particle = active.frame().particles.front().position;
    active.advance(1.F / 60.F, {{particle.x - 30, particle.y}, true});
    passive.advance(1.F / 60.F, {});
    active.advance(1.F / 60.F, {{particle.x - 4, particle.y}, true});
    passive.advance(1.F / 60.F, {});
    check(std::abs(active.frame().particles.front().position.x - passive.frame().particles.front().position.x) > 0.001F,
          "moving cursor deflects nearby particles");
    te::SnowEffect thirty(123), sixty(123);
    thirty.resize(800, 600); sixty.resize(800, 600);
    run(thirty, 300, 1.F / 30.F); run(sixty, 600);
    check(thirty.frame().particles.size() == sixty.frame().particles.size(), "30 and 60 FPS use the same simulation time");
    check(std::abs(snowTotal(thirty.frame()) - snowTotal(sixty.frame())) < 0.1F, "accumulation is frame-rate independent");
    auto before = sixty.frame().particles.front().position;
    sixty.advance(60, {});
    auto after = sixty.frame().particles.front().position;
    check(std::abs(after.y - before.y) < 20, "long pause does not cause catch-up jump");
}
}
int main() {
    try {
        testField(); testSimulation(); testBoundsAndModes(); testCursorAndTiming();
        std::cout << "PASS: " << checks << " assertions (field, snowfall, lifecycle, budgets, cursor, timing)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
