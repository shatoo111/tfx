#pragma once

#include "effects/IEffect.h"
#include <array>

namespace te {
inline void paintStarPixels(PixelCanvas& canvas, const AmbientVisual& star) {
    const float c = std::cos(star.phase), s = std::sin(star.phase);
    const float cell = star.radius < 3.F ? 1.F : 1.5F;
    const auto draw = [&](float extent, std::uint32_t color, float alpha) {
        const float corner = extent * .2F;
        const std::array<Vec2, 8> rays{{{extent, 0}, {corner, corner}, {0, extent}, {-corner, corner},
            {-extent, 0}, {-corner, -corner}, {0, -extent}, {corner, -corner}}};
        std::array<Vec2, 8> points;
        for (std::size_t i = 0; i < rays.size(); ++i) {
            const auto ray = rays[i];
            points[i] = {star.center.x + ray.x*c - ray.y*s, star.center.y + ray.x*s + ray.y*c};
        }
        canvas.pixelPolygon(points, cell, color, pixelOpacity(alpha));
    };
    draw(star.radius + .5F, 0xb0bafa, star.alpha * .38F);
    draw(star.radius * .82F, 0xdce9ff, star.alpha * .67F);

    // Draw the hot centre on the same grid as the rotated rays.
    const float half = std::max(.67F, star.radius * .29F);
    const auto [x, y] = star.center;
    const std::array<Vec2, 4> core{{{x-half, y-half}, {x+half, y-half}, {x+half, y+half}, {x-half, y+half}}};
    canvas.pixelPolygon(core, cell, 0xffe8f7, pixelOpacity(star.alpha));
}
}
