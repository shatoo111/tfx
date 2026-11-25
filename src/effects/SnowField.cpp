#include "effects/SnowField.h"
#include <numeric>
#include <numbers>

namespace te {
void SnowField::resize(float width) {
    if (!std::isfinite(width)) width = 0;
    width = std::clamp(width, 0.F, 32768.F);
    if (width == width_) return;
    const auto old = heights_;
    width_ = width;
    const auto count = static_cast<std::size_t>(std::clamp(std::ceil(width / 8.F) + 1.F, 2.F, 1025.F));
    heights_.assign(count, 0);
    hasSnow_ = false;
    scratch_.resize(count);
    ceilingFactors_.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        // Cache the silhouette: no trigonometry in the per-frame field update.
        const float phase = 2 * std::numbers::pi_v<float> * float(i) / float(count - 1);
        ceilingFactors_[i] = std::clamp(0.76F + 0.13F * std::sin(phase * 2 + 0.4F)
            + 0.08F * std::sin(phase * 5 + 1.7F) + 0.03F * std::sin(phase * 12), 0.5F, 1.F);
    }
    if (old.size() < 2) return;
    for (std::size_t i = 0; i < count; ++i) {
        const float p = float(i) / float(count - 1) * float(old.size() - 1);
        const auto a = std::min(static_cast<std::size_t>(p), old.size() - 2);
        heights_[i] = std::lerp(old[a], old[a + 1], p - float(a));
        hasSnow_ |= heights_[i] > 0.F;
    }
}
float SnowField::spacing() const {
    return heights_.size() > 1 ? width_ / float(heights_.size() - 1) : 0;
}
void SnowField::setMaximum(float maximum) {
    maximum_ = std::isfinite(maximum) ? std::clamp(maximum, 0.F, 160.F) : 48.F;
    if (!hasSnow_) return;
    hasSnow_ = false;
    for (std::size_t i = 0; i < heights_.size(); ++i) {
        heights_[i] = std::min(heights_[i], localMaximum(i));
        hasSnow_ |= heights_[i] > 0.F;
    }
}
float SnowField::localMaximum(std::size_t index) const {
    // Keep mature drifts uneven instead of saturating into a flat rectangular strip.
    // Frequencies are relative to window width, so resizing preserves the silhouette.
    return maximum_ * ceilingFactors_[index];
}
float SnowField::heightAt(float x) const {
    if (!hasSnow_ || width_ <= 0 || !std::isfinite(x)) return 0;
    const float p = std::clamp(x / width_, 0.F, 1.F) * float(heights_.size() - 1);
    const auto i = std::min(static_cast<std::size_t>(p), heights_.size() - 2);
    return std::lerp(heights_[i], heights_[i + 1], p - float(i));
}
void SnowField::deposit(float x, float amount) {
    if (width_ <= 0 || maximum_ <= 0 || !std::isfinite(x) || !std::isfinite(amount) || amount <= 0) return;
    const float step = spacing();
    const int center = static_cast<int>(std::clamp(x / step, 0.F, float(heights_.size() - 1)));
    for (int j = std::max(0, center - 3); j <= std::min(int(heights_.size()) - 1, center + 3); ++j) {
        const float distance = (float(j) * step - x) / std::max(12.F, step * 1.5F);
        const float weight = std::exp(-0.5F * distance * distance);
        heights_[std::size_t(j)] = std::min(localMaximum(std::size_t(j)), heights_[std::size_t(j)] + amount * weight);
        hasSnow_ |= heights_[std::size_t(j)] > 0.F;
    }
}
float SnowField::sweep(float x, float radius, float strength) {
    if (!hasSnow_ || !std::isfinite(x) || !std::isfinite(radius) || !std::isfinite(strength)
        || radius <= 0 || strength <= 0) return 0;
    float removed = 0;
    const float step = spacing();
    hasSnow_ = false;
    for (std::size_t i = 0; i < heights_.size(); ++i) {
        auto& depth = heights_[i];
        if (depth <= 0.F) continue;
        const float distance = std::abs(float(i) * step - x) / radius;
        if (distance < 1.F) {
            const float amount = std::min(depth, strength * (1.F - distance * distance));
            depth -= amount;
            removed += amount;
        }
        hasSnow_ |= depth > 0.F;
    }
    return removed;
}
void SnowField::advance(float dt, bool melting) {
    if (!hasSnow_ || !std::isfinite(dt) || dt <= 0) return;
    dt = std::min(dt, 0.05F);
    // Explicit diffusion with a bounded coefficient: no unstable peaks or negative depth.
    const float diffusion = std::min(dt * 0.85F, 0.24F);
    hasSnow_ = false;
    for (std::size_t i = 0; i < heights_.size(); ++i) {
        const float left = heights_[i == 0 ? i : i - 1];
        const float right = heights_[std::min(i + 1, heights_.size() - 1)];
        float value = heights_[i] + diffusion * (left + right - 2 * heights_[i]);
        if (melting) value -= dt * (2.5F + value * 0.035F);
        scratch_[i] = std::clamp(value, 0.F, localMaximum(i));
        if (scratch_[i] < 0.005F) scratch_[i] = 0;
        hasSnow_ |= scratch_[i] > 0.F;
    }
    heights_.swap(scratch_);
}
void SnowField::clear() {
    std::fill(heights_.begin(), heights_.end(), 0);
    hasSnow_ = false;
}
bool SnowField::empty() const { return !hasSnow_; }
float SnowField::volume() const { return std::accumulate(heights_.begin(), heights_.end(), 0.F) * spacing(); }
}
