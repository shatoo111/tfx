#include "effects/PixelCanvas.h"
#include <algorithm>
#include <cmath>

namespace te {
void PixelCanvas::rect(float x, float y, float width, float height, std::uint32_t rgb, float alpha) {
    if (deviceScale_ > 0) {
        const auto align = [this](float value, float origin) {
            return std::round((value + origin) * deviceScale_) / deviceScale_ - origin;
        };
        quad(align(x, sceneOrigin_.x), align(y, sceneOrigin_.y),
             align(x + width, sceneOrigin_.x), align(y + height, sceneOrigin_.y), rgb, alpha);
    } else {
        const float left = std::round(x), top = std::round(y);
        quad(left, top, left + width, top + height, rgb, alpha);
    }
}
void PixelCanvas::quad(float left, float top, float right, float bottom, std::uint32_t rgb, float alpha) {
    const float a = std::clamp(alpha, 0.F, 1.F);
    if (a <= 0 || right <= left || bottom <= top) return;
    const auto channel = [&](int shift) { return std::uint8_t(float((rgb >> shift) & 255U) * a + .5F); };
    const std::uint8_t r = channel(16), g = channel(8), b = channel(0), o = std::uint8_t(255 * a + .5F);
    out_.push_back({left, top, r, g, b, o}); out_.push_back({right, top, r, g, b, o}); out_.push_back({right, bottom, r, g, b, o});
    out_.push_back({left, top, r, g, b, o}); out_.push_back({right, bottom, r, g, b, o}); out_.push_back({left, bottom, r, g, b, o});
}
void PixelCanvas::pixelPolygon(std::span<const Vec2> outline, float logicalCell, std::uint32_t rgb, float alpha) {
    if (outline.size() < 3 || alpha <= 0) return;
    const float scale = deviceScale_ > 0 ? deviceScale_ : 1.F;
    const float cell = std::max(1.F, std::round(logicalCell * scale));
    const float step = cell / scale;
    const auto [top, bottom] = std::minmax_element(outline.begin(), outline.end(),
        [](Vec2 a, Vec2 b) { return a.y < b.y; });
    const auto cellIndex = [scale, cell](float point, float origin) {
        return int(std::ceil((point + origin) * scale / cell - .5F));
    };
    const int first = cellIndex(top->y, sceneOrigin_.y);
    const int end = cellIndex(bottom->y, sceneOrigin_.y);
    for (int row = first; row < end; ++row) {
        const float y = float(row) * step - sceneOrigin_.y;
        const float sampleY = y + step * .5F;
        crossings_.clear();
        Vec2 previous = outline.back();
        for (const Vec2 point : outline) {
            if ((point.y > sampleY) != (previous.y > sampleY)) {
                const float t = (sampleY - previous.y) / (point.y - previous.y);
                crossings_.push_back(std::lerp(previous.x, point.x, t));
            }
            previous = point;
        }
        std::sort(crossings_.begin(), crossings_.end());
        for (std::size_t i = 1; i < crossings_.size(); i += 2) {
            const float x0 = float(cellIndex(crossings_[i - 1], sceneOrigin_.x)) * step - sceneOrigin_.x;
            const float x1 = float(cellIndex(crossings_[i], sceneOrigin_.x)) * step - sceneOrigin_.x;
            quad(x0, y, x1, y + step, rgb, alpha);
        }
    }
}
void PixelCanvas::sprite(const PixelArt& art, std::span<const Tint> palette, float centerX, float bottom, bool flip, float alpha) {
    const float left = std::round(centerX - float(art.width) * pixel / 2), top = std::round(bottom) - float(art.height) * pixel;
    for (int y = 0; y < art.height; ++y) {
        for (int x = 0; x < art.width;) {
            const char key = art.rows[std::size_t(y * art.width + x)];
            int end = x + 1;
            while (end < art.width && art.rows[std::size_t(y * art.width + end)] == key) ++end;
            if (key != '.') {
                const auto found = std::find_if(palette.begin(), palette.end(), [key](const Tint& t) { return t.key == key; });
                const int column = flip ? art.width - end : x;
                if (found != palette.end())
                    rect(left + float(column) * pixel, top + float(y) * pixel, float(end - x) * pixel, pixel, found->rgb, alpha);
            }
            x = end;
        }
    }
}
}
