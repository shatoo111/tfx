#pragma once
#include "effects/IEffect.h"
#include <functional>

namespace te {
// Small pixel New Year figures and lights on the snowdrift.
class FestiveDecor {
public:
    using Surface = std::function<float(float)>; // Snow depth at x.
    void resize(float width, float height) { width_ = width; height_ = height; }
    void advance(float dt, CursorSample cursor);
    void drawFigures(std::vector<ShapeVertex>& out, const Surface& depth) const; // Behind the snow.
    void drawLights(std::vector<ShapeVertex>& out, const Surface& depth) const;  // On the snow.
private:
    CursorSample cursor_;
    float width_ = 0, height_ = 0, time_ = 0;
};
}
