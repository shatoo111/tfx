#pragma once

#include "effects/IEffect.h"
#include <array>
#include <limits>

namespace te {
namespace rainPixels {
struct Paint {
    std::uint32_t rgb;
    float opacity;
    bool operator==(const Paint&) const = default;
};

inline constexpr int dropSegments = 6;
inline constexpr int bladeSegments = 4;
inline constexpr auto dropOutline = [] {
    std::array<Vec2, dropSegments * 2> outline{};
    for (int i = 0; i <= dropSegments; ++i) {
        const float t = float(i) / dropSegments;
        const float u = 1.F - t;
        const Vec2 p{-(3.F * u * u * t * 0.35F + 3.F * u * t * t),
                     -3.5F * u * u * u - 6.F * u * u * t + 0.65F * t * t * t};
        outline[i] = p;
        if (i > 0 && i < dropSegments) outline[outline.size() - i] = {-p.x, p.y};
    }
    return outline;
}();

inline constexpr Paint greens[3][3] = {
    {{0x2F5638, .23F}, {0x4C7E4C, .45F}, {0x64985C, .65F}},
    {{0x487547, .37F}, {0x6B924F, .52F}, {0x84A964, .67F}},
    {{0x314E3A, .24F}, {0x497B4C, .46F}, {0x5B915B, .65F}}
};

// Rain has narrow, connected silhouettes. Keep their tapered tips visible as a
// single pixel, and combine adjacent rows of a single colour into one rectangle.
template<std::size_t N, typename RowPaint>
void drawRows(PixelCanvas& canvas, const std::array<Vec2, N>& outline, RowPaint paintAt) {
    constexpr float cell = PixelCanvas::pixel;
    const auto [top, bottom] = std::minmax_element(outline.begin(), outline.end(),
        [](Vec2 a, Vec2 b) { return a.y < b.y; });
    const float height = bottom->y - top->y;
    if (height <= 0.F) return;

    float runX = 0.F, runY = 0.F, runWidth = 0.F, runBottom = 0.F;
    Paint runPaint{};
    const auto flush = [&] {
        if (runWidth > 0.F) canvas.rect(runX, runY, runWidth, runBottom - runY, runPaint.rgb, runPaint.opacity);
    };
    const float first = std::floor(top->y / cell) * cell;
    const float end = std::ceil(bottom->y / cell) * cell;
    for (float y = first; y < end; y += cell) {
        const float low = std::max(y, top->y), high = std::min(y + cell, bottom->y);
        const float sampleY = (low + high) * .5F;
        float left = std::numeric_limits<float>::infinity(), right = -left;
        for (std::size_t i = 0; i < outline.size(); ++i) {
            const auto a = outline[i], b = outline[(i + 1) % outline.size()];
            if ((a.y > sampleY) == (b.y > sampleY)) continue;
            const float x = std::lerp(a.x, b.x, (sampleY - a.y) / (b.y - a.y));
            left = std::min(left, x);
            right = std::max(right, x);
        }
        if (left >= right) continue;
        float x = std::ceil((left - cell * .5F) / cell) * cell;
        const float edge = std::ceil((right - cell * .5F) / cell) * cell;
        const float width = std::max(cell, edge - x);
        if (edge <= x) x = std::floor((left + right) * .5F / cell) * cell;
        const Paint paint = paintAt((sampleY - top->y) / height);
        if (runX != x || runWidth != width || runPaint != paint || runBottom != y) {
            flush();
            runX = x;
            runY = y;
            runWidth = width;
            runPaint = paint;
        }
        runBottom = y + cell;
    }
    flush();
}
}

inline void drawRainDrop(PixelCanvas& canvas, const RainVisual& drop) {
    auto outline = rainPixels::dropOutline;
    for (auto& p : outline) {
        const float x = p.x * drop.size, y = p.y * drop.size;
        p = {drop.position.x + x * drop.direction.y + y * drop.direction.x,
             drop.position.y - x * drop.direction.x + y * drop.direction.y};
    }
    const float opacity = pixelOpacity(drop.opacity);
    rainPixels::drawRows(canvas, outline, [&](float progress) -> rainPixels::Paint {
        if (progress < .38F) return {drop.highlight ? 0xA3CCDEU : 0x76B2CEU, opacity};
        return {drop.highlight ? 0xD1E9EFU : 0xB0D8E8U, opacity};
    });
}

inline void drawRainSplash(PixelCanvas& canvas, const SplashVisual& splash) {
    static constexpr PixelArt fleck{2, 2, ".b" "ba"};
    static constexpr Tint palette[]{{'b', 0x84CBE0}, {'a', 0xD5F1ED}};
    canvas.sprite(fleck, palette, splash.position.x, splash.position.y + PixelCanvas::pixel,
                  false, pixelOpacity(splash.opacity));
}

inline void drawGrassBlade(PixelCanvas& canvas, const GrassVisual& blade) {
    constexpr int segments = rainPixels::bladeSegments;
    const float lean = blade.tip.x - blade.base.x, height = blade.base.y - blade.tip.y;
    std::array<Vec2, segments * 2 + 1> outline{};
    const auto point = [&](float t, bool right) {
        const float u = 1.F - t;
        const float start = right ? 2.7F : -2.7F;
        const float control = right ? lean * .32F + 2.F : lean * .2F - 2.F;
        const float rise = 2.F * u * t * (right ? .45F : .65F) + t * t;
        return Vec2{blade.base.x + u * u * start + 2.F * u * t * control + t * t * lean,
                    blade.base.y - rise * height};
    };
    for (int i = 0; i < segments; ++i) {
        const float t = float(i) / segments;
        outline[i] = point(t, false);
        outline[outline.size() - 1 - i] = point(t, true);
    }
    outline[segments] = blade.tip;
    rainPixels::drawRows(canvas, outline, [&](float progress) {
        auto paint = rainPixels::greens[blade.shade][progress < .35F ? 2 : progress < .69F ? 1 : 0];
        paint.opacity = pixelOpacity(paint.opacity * blade.opacity * std::min(1.F, height / PixelCanvas::pixel));
        return paint;
    });
}
}
