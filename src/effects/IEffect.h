#pragma once

#include "effects/Geometry.h"
#include "effects/Illustration.h"
#include "effects/PixelCanvas.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

namespace te {
enum class EffectKind { Snow, Rain, Sakura, Leaves, UnitedStates, Stardust, Retro, BlackHole, Cat, Aquarium };
struct EffectConfig {
    EffectKind kind = EffectKind::Snow;
    bool enabled = true;
    int intensity = 1; // Light, Normal, Blizzard
    float wind = 0.15F;
    bool accumulation = true;
    float maxDepth = 48;
    bool interaction = true;
    int fps = 60;
    int rainIntensity = 1;
    bool grass = true;
    float grassHeight = 48;
    bool lightning = false;
    float flashInterval = 15;
    int sakuraIntensity = 1;
    float sakuraAngle = 15; // Degrees from vertical; positive means right.
    bool sakuraBranches = true;
    int ambientIntensity = 1;
    int flightPause = 17; // Seconds from the end of a group to the next departure.
    int flightLimit = 2;
    int branchSide = 2; // Left, right, both.
    int retroHero = 0; // Cat, knight, robot.
    int catCoat = 0; // Ginger, black, grey, white.
    bool festive = true; // New Year decorations in the snowfall.
    static constexpr float minBranchOpacity = 0.25F;
    float branchWidth = 78, branchOpacity = 1;
    bool economy = false, reducedMotion = false;
    void normalize() {
        if (kind < EffectKind::Snow || kind > EffectKind::Aquarium) kind = EffectKind::Snow;
        retroHero = std::clamp(retroHero, 0, 2);
        catCoat = std::clamp(catCoat, 0, 3);
        flightPause = std::clamp(flightPause, 2, 60);
        flightLimit = std::clamp(flightLimit, 1, maxFlights);
        ambientIntensity = std::clamp(ambientIntensity, 0, 2);
        branchSide = std::clamp(branchSide, 0, 2);
        branchWidth = std::isfinite(branchWidth) ? std::clamp(branchWidth, 24.F, 160.F) : 78.F;
        branchOpacity = std::isfinite(branchOpacity) ? std::clamp(branchOpacity, minBranchOpacity, 1.F) : 1.F;
        sakuraIntensity = std::clamp(sakuraIntensity, 0, 2);
        sakuraAngle = std::isfinite(sakuraAngle) ? std::clamp(sakuraAngle, -65.F, 65.F) : 15.F;
        intensity = std::clamp(intensity, 0, 2);
        rainIntensity = std::clamp(rainIntensity, 0, 2);
        wind = std::isfinite(wind) ? std::clamp(wind, -1.F, 1.F) : 0.F;
        maxDepth = std::isfinite(maxDepth) ? std::clamp(maxDepth, 0.F, 160.F) : 48.F;
        fps = fps == 30 ? 30 : 60;
        grassHeight = std::isfinite(grassHeight) ? std::clamp(grassHeight, 0.F, 160.F) : 48.F;
        flashInterval = std::isfinite(flashInterval) ? std::clamp(flashInterval, 5.F, 60.F) : 15.F;
    }
};
struct CursorSample { Vec2 position; bool inside = false; };
struct ParticleVisual { Vec2 position; float radius; float opacity; };
struct SurfaceSample { float x; float height; };
struct RainVisual { Vec2 position, direction; float size, opacity; bool highlight; };
struct SplashVisual { Vec2 position; float opacity; };
struct GrassVisual { Vec2 base, tip; float opacity; std::size_t shade; };
struct FlashVisual { Vec2 center; float radius, opacity; };
struct PetalVisual { Vec2 position; float radius, angle, flutter, shade, opacity; };
struct AmbientVisual { Vec2 center; float radius, phase, alpha; };
struct EffectFrame {
    std::vector<AmbientVisual> ambient;
    std::vector<Illustration> illustrations;
    std::vector<ParticleVisual> particles;
    std::vector<SurfaceSample> snow;
    std::vector<RainVisual> rain;
    std::vector<SplashVisual> splashes;
    std::vector<GrassVisual> grass;
    std::vector<FlashVisual> flashes;
    std::vector<PetalVisual> petals;
    // Pixel scenes: behind the snowdrift, on the snowdrift, and above everything else.
    std::vector<ShapeVertex> underlay, detail, overlay;
    std::uint32_t textureVersion = 0; // Grows whenever underlay or detail change; the overlay is always fresh.
};

// Future effects own their simulation; renderer receives immutable geometry data.
// Future UI masks can implement this without coupling the engine to Telegram.
class ICollisionSurface {
public:
    virtual ~ICollisionSurface() = default;
    virtual float heightAt(float x) const = 0;
    virtual void deposit(float x, float amount) = 0;
};
class IEffect {
public:
    virtual ~IEffect() = default;
    virtual void configure(EffectConfig config) = 0;
    virtual void resize(float width, float height) = 0;
    virtual void advance(float seconds, CursorSample cursor) = 0;
    virtual void setLoadScale(float scale) = 0;
    // Things that answer a click: the overlay listens for presses only while there are any.
    virtual bool pressable() const { return false; }
    virtual bool pressed(Vec2) { return false; }
    virtual void resetCursor() = 0;
    virtual void clear() = 0;
    virtual bool needsAnimation() const = 0;
    virtual const EffectFrame& frame() const = 0;
};
} // namespace te
