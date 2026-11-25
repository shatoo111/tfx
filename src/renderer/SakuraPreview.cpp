#include "renderer/SakuraPreview.h"
#include "effects/SeasonalPixelArt.h"
#include "renderer/EffectItem.h"
#include "renderer/IllustrationNode.h"
#include <QPainter>
#include <QQuickPaintedItem>
#include <numbers>

namespace te {
namespace {
constexpr double toDegrees = 180.0 / std::numbers::pi;

class PaintedSakura final : public QQuickPaintedItem {
public:
    PaintedSakura(const SakuraEffect& effect, QQuickItem* parent)
        : QQuickPaintedItem(parent), effect_(effect) { setSmooth(false); }
    void paint(QPainter* painter) override;
private:
    const SakuraEffect& effect_;
    std::vector<ShapeVertex> pixelVertices_;
};
}

SakuraPreview::SakuraPreview(QQuickItem* parent) : QQuickItem(parent) {
    selectRenderer();
}

SakuraPreview::~SakuraPreview() { delete renderer_; }

void SakuraPreview::setSoftware(bool software) {
    if (software_ == software) return;
    software_ = software;
    selectRenderer();
    emit softwareChanged();
}

void SakuraPreview::selectRenderer() {
    delete renderer_;
    if (software_) renderer_ = new PaintedSakura(effect_, this);
    else renderer_ = new EffectItem(effect_, this);
    renderer_->setSize(size());
}

void SakuraPreview::configure(int intensity, double angle, bool branches, int side,
                              double size, double opacity, bool interaction) {
    EffectConfig config;
    config.kind = EffectKind::Sakura;
    config.sakuraIntensity = intensity;
    config.sakuraAngle = float(angle);
    config.sakuraBranches = branches;
    config.branchSide = side;
    config.branchWidth = float(size);
    config.branchOpacity = float(opacity);
    config.interaction = interaction;
    effect_.configure(config);
    if (effect_.frame().petals.empty()) effect_.advance(1.F / 60.F, {});
    renderer_->update();
}

void SakuraPreview::geometryChange(const QRectF& geometry, const QRectF& old) {
    QQuickItem::geometryChange(geometry, old);
    effect_.resize(float(geometry.width()), float(geometry.height()));
    renderer_->setSize(geometry.size());
    renderer_->update();
}

void SakuraPreview::advance(double seconds, QPointF cursor) {
    effect_.advance(float(seconds), {{float(cursor.x()), float(cursor.y())}, contains(cursor)});
    renderer_->update();
}

void PaintedSakura::paint(QPainter* painter) {
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (const auto& sprite : effect_.frame().illustrations) {
        painter->save();
        painter->setOpacity(pixelOpacity(sprite.opacity));
        painter->translate(sprite.x, sprite.y);
        painter->rotate(sprite.angle * toDegrees);
        const float w = std::abs(sprite.width);
        if (sprite.width < 0) painter->scale(-1, 1);
        painter->drawImage(QRectF(-w * sprite.pivotX, -sprite.height * sprite.pivotY, w, sprite.height),
                           rasterArt(sprite.art));
        painter->restore();
    }

    pixelVertices_.clear();
    PixelCanvas pixels(pixelVertices_, float(painter->device()->devicePixelRatioF()));
    for (const auto& petal : effect_.frame().petals) drawSakuraPetal(pixels, petal);
    for (std::size_t i = 0; i < pixelVertices_.size(); i += 6) {
        const auto& top = pixelVertices_[i];
        const auto& bottom = pixelVertices_[i + 2];
        painter->fillRect(QRectF(QPointF(top.x, top.y), QPointF(bottom.x, bottom.y)),
                         QColor::fromRgba(qUnpremultiply(qRgba(top.r, top.g, top.b, top.a))));
    }
}
}
