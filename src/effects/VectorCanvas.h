#pragma once
#include "effects/PixelCanvas.h"
#include <initializer_list>
#include <span>
#include <vector>

namespace te {
struct Paint { float r = 0, g = 0, b = 0, a = 1; }; // Straight alpha.
constexpr Paint rgb(std::uint32_t hex, float alpha = 1) {
    return {float((hex >> 16) & 255) / 255.F, float((hex >> 8) & 255) / 255.F, float(hex & 255) / 255.F, alpha};
}
constexpr Paint faded(Paint p, float k) { return {p.r, p.g, p.b, p.a * k}; }
constexpr Paint mix(Paint a, Paint b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}

// Immediate-mode vector drawing for the snowbank: every call appends antialiased triangles.
class VectorCanvas {
public:
    explicit VectorCanvas(std::vector<ShapeVertex>& out) : out_(out) {}
    void ellipse(Vec2 center, float rx, float ry, Paint paint) { ellipse(center, rx, ry, paint, paint); }
    void ellipse(Vec2 center, float rx, float ry, Paint inner, Paint outer);
    // A tiny soft dot in six triangles, for marks of a couple of points.
    void speck(Vec2 center, float rx, float ry, Paint paint);
    // Simple polygon, convex or not, with a vertical gradient.
    void polygon(std::span<const Vec2> outline, Paint top, Paint bottom);
    void polygon(std::span<const Vec2> outline, Paint paint) { polygon(outline, paint, paint); }
    void polygon(std::initializer_list<Vec2> outline, Paint top, Paint bottom);
    void polygon(std::initializer_list<Vec2> outline, Paint paint) { polygon(outline, paint, paint); }
    void stroke(std::span<const Vec2> points, float startWidth, float endWidth, Paint start, Paint end, bool caps = true);
    void stroke(std::initializer_list<Vec2> points, float startWidth, float endWidth, Paint paint, bool caps = true);
    void quad(Vec2 p0, Paint c0, Vec2 p1, Paint c1, Vec2 p2, Paint c2, Vec2 p3, Paint c3); // No antialiasing.

private:
    void put(Vec2 point, Paint paint);
    void triangle(Vec2 a, Paint ca, Vec2 b, Paint cb, Vec2 c, Paint cc);
    void fill(std::span<const Vec2> screen, Paint top, Paint bottom);
    void ribbon(std::span<const Vec2> screen, float startWidth, float endWidth, Paint start, Paint end, bool caps);
    std::vector<ShapeVertex>& out_;
    std::vector<Vec2> normals_;
    std::vector<int> order_;
};

}
