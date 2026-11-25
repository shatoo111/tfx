#pragma once
#include "effects/Geometry.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace te {
// Small pixel scenes live in this strip above the bottom edge, so they never cover
// the chat list, the header or the messages.
inline constexpr float edgeStrip = 46;

// One corner of a flat triangle. Colours are premultiplied, as the scene graph blends them.
struct ShapeVertex { float x, y; std::uint8_t r, g, b, a; };
struct Tint { char key; std::uint32_t rgb; };
struct PixelArt { int width, height; std::string_view rows; }; // Keys row by row; '.' is transparent.

inline float pixelOpacity(float alpha, float levels = 16.F) {
    return std::round(std::clamp(alpha, 0.F, 1.F) * levels) / levels;
}

// Draws crisp art pixels as plain quads. Positions snap to whole points.
class PixelCanvas {
public:
    static constexpr float pixel = 2; // Logical points per art pixel, as in the 8-bit theme.
    explicit PixelCanvas(std::vector<ShapeVertex>& out) : out_(out) {}
    PixelCanvas(std::vector<ShapeVertex>& out, float deviceScale, Vec2 sceneOrigin = {})
        : out_(out), deviceScale_(deviceScale), sceneOrigin_(sceneOrigin) {}
    void rect(float x, float y, float width, float height, std::uint32_t rgb, float alpha = 1);
    void dot(float x, float y, std::uint32_t rgb, float alpha = 1) { rect(x, y, pixel, pixel, rgb, alpha); }
    // Anchored by the bottom centre. Keys missing from the palette stay transparent.
    void sprite(const PixelArt& art, std::span<const Tint> palette, float centerX, float bottom, bool flip = false, float alpha = 1);
    // Sample the continuous outline on a screen-aligned grid and batch adjacent cells by row.
    void pixelPolygon(std::span<const Vec2> outline, float logicalCell, std::uint32_t rgb, float alpha = 1);
private:
    void quad(float left, float top, float right, float bottom, std::uint32_t rgb, float alpha);
    std::vector<ShapeVertex>& out_;
    float deviceScale_ = 0;
    Vec2 sceneOrigin_{};
    std::vector<float> crossings_;
};

// Stable pseudo-random value in [0, 1) for decorations that must not move between frames.
inline float hash01(int a, int b = 0, int c = 0) {
    std::uint32_t h = std::uint32_t(a) * 374761393U + std::uint32_t(b) * 668265263U + std::uint32_t(c) * 2147483647U;
    h = (h ^ (h >> 13)) * 1274126177U;
    h ^= h >> 16;
    return float(h & 0xffffffU) / 16777216.F;
}
}
