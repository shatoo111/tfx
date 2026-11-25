#include "effects/FestiveDecor.h"
#include "effects/SceneSprites.h"
#include <cmath>

namespace te {
namespace {
constexpr std::array bulbs{0xff5a5fU, 0xffd23fU, 0x4fd08aU, 0x5aa9ffU};
constexpr float pixel = PixelCanvas::pixel;
}
void FestiveDecor::advance(float dt, CursorSample cursor) {
    time_ = std::fmod(time_ + dt, 10000.F);
    cursor_ = cursor;
}
void FestiveDecor::drawFigures(std::vector<ShapeVertex>& out, const Surface& depth) const {
    struct Figure { float fraction; int kind, seed; };
    constexpr std::array<Figure, 7> figures{{{.03F, 0, 0}, {.064F, 1, 1}, {.092F, 3, 0}, {.12F, 2, 0}, {.91F, 3, 1}, {.938F, 1, 2}, {.968F, 0, 3}}};
    PixelCanvas canvas(out);
    float free = -100;
    for (const auto& f : figures) {
        const PixelArt& sprite = f.kind == 0 ? art::tree : f.kind == 1 ? art::smallTree : f.kind == 2 ? art::snowman : art::giftBox;
        const float half = float(sprite.width) * pixel / 2, tall = float(sprite.height) * pixel;
        const float x = std::clamp(f.fraction * width_, half + 2, std::max(half + 2, width_ - half - 2));
        if (x - half < free) continue; // Narrow windows keep fewer figures.
        const float snow = depth(x), shown = std::clamp((snow - 2) / 6, 0.F, 1.F);
        if (shown <= 0) continue;
        free = x + half + 2;
        // Figures rise with the first snow. Deep snow buries them instead of lifting them over the chat.
        const float top = std::min(snow + tall * shown, std::max(edgeStrip, snow + 12));
        const auto light = [&](int i, std::uint32_t color) {
            return std::sin(time_ * 2.6F + float(i) * 2.1F + float(f.seed)) > -.2F ? color : 0x2b5f4cU;
        };
        const std::array<Tint, 20> palette{{{'K', 0x2a2238}, {'g', 0x2f9a5f}, {'d', 0x1c6a48}, {'w', 0xffffff}, {'v', 0xc9dcf0}, {'t', 0x6b4a34},
            {'y', f.kind == 3 && f.seed ? 0xff7a75U : 0xffd23fU}, {'Y', 0xfff4a8}, {'o', 0xff8a3c},
            {'r', f.kind == 3 && f.seed ? 0x3f7dffU : 0xe5484dU}, {'R', f.seed ? 0x6ea2ffU : 0xff7a75U}, {'D', f.seed ? 0x2a55bfU : 0xb02f3aU},
            {'1', light(0, bulbs[0])}, {'2', light(1, bulbs[1])}, {'3', light(2, bulbs[3])}, {'.', 0}, {'.', 0}, {'.', 0}, {'.', 0}, {'.', 0}}};
        canvas.sprite(sprite, palette, x, height_ - top + tall, f.seed == 3);
    }
}
void FestiveDecor::drawLights(std::vector<ShapeVertex>& out, const Surface& depth) const {
    PixelCanvas canvas(out);
    // A string of lights lies along the crest.
    const int columns = int(width_ / pixel);
    for (int c = 0; c < columns; ++c) {
        const float x = float(c) * pixel, snow = depth(x + 1);
        if (snow < 5) continue;
        const float y = height_ - std::ceil(snow / pixel) * pixel;
        const int at = c % 15;
        if (at == 7) {
            const int i = c / 15;
            float glow = std::sin(time_ * 2 + float(i) * 1.7F) > -.15F ? 1.F : .35F;
            if (cursor_.inside && std::hypot(cursor_.position.x - x, cursor_.position.y - y) < 70) glow = 1;
            const auto color = bulbs[std::size_t(i) % bulbs.size()];
            if (glow > .9F) canvas.rect(x - pixel * 1.5F, y - pixel * 2, pixel * 4, pixel * 4, color, .28F);
            canvas.rect(x - pixel / 2, y - pixel, pixel * 2, pixel * 2, glow > .9F ? color : 0x6f7f9aU);
            canvas.dot(x - pixel / 2, y - pixel, 0xffffff, .55F * glow);
            canvas.dot(x, y + pixel, 0x2c6b55);
        } else if ((at + 1) % 3) {
            canvas.dot(x, y + (at > 3 && at < 11 ? 0 : pixel), 0x2c6b55, .8F);
        }
    }
}
}
