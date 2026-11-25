#pragma once
#include "effects/IEffect.h"
#include <QQuickItem>

namespace te {
class EffectItem final : public QQuickItem {
    Q_OBJECT
public:
    explicit EffectItem(IEffect& effect, QQuickItem* parent = nullptr);
    void advance(float dt, CursorSample cursor);
    bool pressable() const { return effect_->pressable(); }
    bool pressAt(QPointF point);
    void setEffect(IEffect& effect);
protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
private:
    IEffect* effect_;
    std::vector<ShapeVertex> pixelVertices_;
    std::uint32_t textureVersion_ = 0;
    bool textureStale_ = true;
};
}
