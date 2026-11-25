#include "effects/VectorCanvas.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace te {
namespace {
constexpr float pi = std::numbers::pi_v<float>;
constexpr float feather = .7F; // Logical points.
float cross(Vec2 o, Vec2 a, Vec2 b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); }
Vec2 unit(Vec2 v) {
    const float length = std::hypot(v.x, v.y);
    return length > 1e-5F ? Vec2{v.x / length, v.y / length} : Vec2{};
}
int segmentsFor(float radius) { return std::clamp(int(4 + 6.2F * std::sqrt(std::max(0.F, radius))), 8, 160); }
}

void VectorCanvas::put(Vec2 point, Paint paint) {
    const float alpha = std::clamp(paint.a, 0.F, 1.F) * 255.F;
    const auto channel = [&](float v) { return std::uint8_t(std::clamp(v, 0.F, 1.F) * alpha + .5F); };
    out_.push_back({point.x, point.y, channel(paint.r), channel(paint.g), channel(paint.b), std::uint8_t(alpha + .5F)});
}
void VectorCanvas::triangle(Vec2 a, Paint ca, Vec2 b, Paint cb, Vec2 c, Paint cc) {
    put(a, ca); put(b, cb); put(c, cc);
}
void VectorCanvas::quad(Vec2 p0, Paint c0, Vec2 p1, Paint c1, Vec2 p2, Paint c2, Vec2 p3, Paint c3) {
    triangle(p0, c0, p1, c1, p2, c2);
    triangle(p0, c0, p2, c2, p3, c3);
}
void VectorCanvas::ellipse(Vec2 center, float rx, float ry, Paint inner, Paint outer) {
    if (rx <= 0 || ry <= 0) return;
    const int count = segmentsFor(std::max(rx, ry));
    const Paint clear = faded(outer, 0);
    Vec2 previous{}, previousOut{};
    for (int i = 0; i <= count; ++i) {
        const float t = 2 * pi * float(i) / float(count);
        const Vec2 p{center.x + rx * std::cos(t), center.y + ry * std::sin(t)};
        const Vec2 n = unit({std::cos(t) / rx, std::sin(t) / ry}); // Constant edge width.
        const Vec2 q{p.x + n.x * feather, p.y + n.y * feather};
        if (i > 0) {
            triangle(center, inner, previous, outer, p, outer);
            triangle(previous, outer, previousOut, clear, q, clear);
            triangle(previous, outer, q, clear, p, outer);
        }
        previous = p; previousOut = q;
    }
}
void VectorCanvas::speck(Vec2 center, float rx, float ry, Paint paint) {
    constexpr std::array<Vec2, 6> corners{{{1, 0}, {.5F, .866F}, {-.5F, .866F}, {-1, 0}, {-.5F, -.866F}, {.5F, -.866F}}};
    constexpr float grow = .55F;
    const auto at = [&](std::size_t i) {
        const Vec2 d = corners[i % corners.size()];
        return Vec2{center.x + d.x * (rx * 1.15F + grow), center.y + d.y * (ry * 1.15F + grow)};
    };
    for (std::size_t i = 0; i < corners.size(); ++i) triangle(center, paint, at(i), faded(paint, 0), at(i + 1), faded(paint, 0));
}
void VectorCanvas::fill(std::span<const Vec2> p, Paint top, Paint bottom) {
    const int n = int(p.size());
    if (n < 3) return;
    float area = 0, low = p[0].y, high = p[0].y;
    for (int i = 0; i < n; ++i) {
        const Vec2 a = p[std::size_t(i)], b = p[std::size_t((i + 1) % n)];
        area += a.x * b.y - b.x * a.y;
        low = std::min(low, a.y); high = std::max(high, a.y);
    }
    if (std::abs(area) < 1e-3F) return;
    const float sign = area > 0 ? 1.F : -1.F;
    const auto paint = [&](Vec2 v) { return mix(top, bottom, high > low ? (v.y - low) / (high - low) : 0.F); };
    // Ear clipping: outlines here have a few dozen points.
    order_.resize(std::size_t(n));
    for (int i = 0; i < n; ++i) order_[std::size_t(i)] = i;
    const auto at = [&](std::size_t i) { return p[std::size_t(order_[i % order_.size()])]; };
    std::size_t cursor = 0, misses = 0;
    while (order_.size() > 3 && misses <= order_.size()) {
        const std::size_t size = order_.size();
        const std::size_t prev = (cursor + size - 1) % size, here = cursor % size, next = (cursor + 1) % size;
        const Vec2 a = at(prev), b = at(here), c = at(next);
        bool ear = cross(a, b, c) * sign > 0;
        for (std::size_t k = 0; ear && k < size; ++k) {
            if (k == prev || k == here || k == next) continue;
            const Vec2 q = at(k);
            ear = !(cross(a, b, q) * sign >= 0 && cross(b, c, q) * sign >= 0 && cross(c, a, q) * sign >= 0);
        }
        if (ear) {
            triangle(a, paint(a), b, paint(b), c, paint(c));
            order_.erase(order_.begin() + std::ptrdiff_t(here));
            cursor = here;
            misses = 0;
        } else {
            ++cursor; ++misses;
        }
        if (!order_.empty()) cursor %= order_.size();
    }
    if (order_.size() == 3) triangle(at(0), paint(at(0)), at(1), paint(at(1)), at(2), paint(at(2)));
    // Soft outer edge.
    normals_.resize(std::size_t(n));
    for (int i = 0; i < n; ++i) {
        const Vec2 a = p[std::size_t((i + n - 1) % n)], b = p[std::size_t(i)], c = p[std::size_t((i + 1) % n)];
        const Vec2 e0 = unit({b.x - a.x, b.y - a.y}), e1 = unit({c.x - b.x, c.y - b.y});
        const Vec2 sum = unit({(e0.y + e1.y) * sign, -(e0.x + e1.x) * sign});
        normals_[std::size_t(i)] = sum;
    }
    for (int i = 0; i < n; ++i) {
        const auto j = std::size_t((i + 1) % n), k = std::size_t(i);
        const Vec2 a = p[k], b = p[j];
        const Vec2 ao{a.x + normals_[k].x * feather, a.y + normals_[k].y * feather};
        const Vec2 bo{b.x + normals_[j].x * feather, b.y + normals_[j].y * feather};
        const Paint ca = paint(a), cb = paint(b);
        triangle(a, ca, ao, faded(ca, 0), bo, faded(cb, 0));
        triangle(a, ca, bo, faded(cb, 0), b, cb);
    }
}
void VectorCanvas::polygon(std::span<const Vec2> outline, Paint top, Paint bottom) {
    fill(outline, top, bottom);
}
void VectorCanvas::polygon(std::initializer_list<Vec2> outline, Paint top, Paint bottom) {
    polygon(std::span<const Vec2>(outline.begin(), outline.size()), top, bottom);
}
void VectorCanvas::ribbon(std::span<const Vec2> p, float startWidth, float endWidth, Paint start, Paint end, bool caps) {
    const std::size_t n = p.size();
    if (n < 2) return;
    const auto normal = [&](std::size_t i) {
        const Vec2 a = p[i > 0 ? i - 1 : 0], b = p[std::min(i + 1, n - 1)];
        const Vec2 d = unit({b.x - a.x, b.y - a.y});
        return Vec2{-d.y, d.x};
    };
    Vec2 last{}, lastNormal = normal(0);
    float lastHalf = 0;
    Paint lastPaint = start;
    for (std::size_t i = 0; i < n; ++i) {
        const float t = float(i) / float(n - 1);
        const float half = .5F * (startWidth + (endWidth - startWidth) * t);
        const Paint paint = mix(start, end, t);
        const Vec2 nrm = normal(i);
        if (i > 0) {
            const auto side = [](Vec2 c, Vec2 d, float k) { return Vec2{c.x + d.x * k, c.y + d.y * k}; };
            const Vec2 l0 = side(last, lastNormal, lastHalf), r0 = side(last, lastNormal, -lastHalf);
            const Vec2 l1 = side(p[i], nrm, half), r1 = side(p[i], nrm, -half);
            const Vec2 lf0 = side(last, lastNormal, lastHalf + feather), rf0 = side(last, lastNormal, -lastHalf - feather);
            const Vec2 lf1 = side(p[i], nrm, half + feather), rf1 = side(p[i], nrm, -half - feather);
            const Paint c0 = faded(lastPaint, 0), c1 = faded(paint, 0);
            triangle(l0, lastPaint, r0, lastPaint, r1, paint); triangle(l0, lastPaint, r1, paint, l1, paint);
            triangle(lf0, c0, l0, lastPaint, l1, paint); triangle(lf0, c0, l1, paint, lf1, c1);
            triangle(r0, lastPaint, rf0, c0, rf1, c1); triangle(r0, lastPaint, rf1, c1, r1, paint);
        }
        last = p[i]; lastNormal = nrm; lastHalf = half; lastPaint = paint;
    }
    if (!caps) return;
    const auto cap = [&](Vec2 c, Vec2 nrm, float half, Paint paint, float direction) {
        if (half < .35F) return;
        const Vec2 forward{nrm.y * direction, -nrm.x * direction};
        const int count = std::clamp(int(3 + half), 4, 14);
        Vec2 previous{}, previousOut{};
        for (int i = 0; i <= count; ++i) {
            const float t = pi * float(i) / float(count);
            const Vec2 d{nrm.x * std::cos(t) + forward.x * std::sin(t), nrm.y * std::cos(t) + forward.y * std::sin(t)};
            const Vec2 q{c.x + d.x * half, c.y + d.y * half}, o{c.x + d.x * (half + feather), c.y + d.y * (half + feather)};
            if (i > 0) {
                triangle(c, paint, previous, paint, q, paint);
                triangle(previous, paint, previousOut, faded(paint, 0), o, faded(paint, 0));
                triangle(previous, paint, o, faded(paint, 0), q, paint);
            }
            previous = q; previousOut = o;
        }
    };
    cap(p[0], normal(0), startWidth * .5F, start, -1);
    cap(p[n - 1], normal(n - 1), endWidth * .5F, end, 1);
}
void VectorCanvas::stroke(std::span<const Vec2> points, float startWidth, float endWidth, Paint start, Paint end, bool caps) {
    ribbon(points, startWidth, endWidth, start, end, caps);
}
void VectorCanvas::stroke(std::initializer_list<Vec2> points, float startWidth, float endWidth, Paint paint, bool caps) {
    stroke(std::span<const Vec2>(points.begin(), points.size()), startWidth, endWidth, paint, paint, caps);
}
}
