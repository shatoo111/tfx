#pragma once
#include "renderer/EffectItem.h"
#include <memory>

namespace te {
// The menu samples the very same simulation and scene graph as the live overlay.
class IllustratedPreview : public QQuickItem {
    Q_OBJECT
public:
    using QQuickItem::QQuickItem;
    Q_INVOKABLE void configure(int theme, int intensity, double wind, int flightPause, int flightLimit, int hero = 0,
                               int coat = 0, bool festive = true, bool accumulation = true, double depth = 48,
                               bool lightning = false, double flashInterval = 15);
    Q_INVOKABLE void pressAt(QPointF point) { if (item_) item_->pressAt(point); }
    Q_INVOKABLE void advance(double seconds, QPointF cursor);
protected:
    void geometryChange(const QRectF& geometry, const QRectF& old) override;
private:
    int theme_ = -1;
    std::unique_ptr<IEffect> effect_;
    EffectItem* item_ = nullptr;
};
}
