#pragma once

#include "effects/SakuraEffect.h"
#include <QQuickItem>

namespace te {
class SakuraPreview : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(bool software READ software WRITE setSoftware NOTIFY softwareChanged)
public:
    explicit SakuraPreview(QQuickItem* parent = nullptr);
    ~SakuraPreview() override;
    bool software() const { return software_; }
    void setSoftware(bool software);
    Q_INVOKABLE void configure(int intensity, double angle, bool branches, int side,
                               double size, double opacity, bool interaction);
    Q_INVOKABLE void advance(double seconds, QPointF cursor);
signals:
    void softwareChanged();
protected:
    void geometryChange(const QRectF& geometry, const QRectF& old) override;
private:
    void selectRenderer();
    SakuraEffect effect_{42};
    QQuickItem* renderer_ = nullptr;
    bool software_ = false;
};
}
