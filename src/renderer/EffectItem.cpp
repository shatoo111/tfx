#include "renderer/EffectItem.h"
#include "effects/RainPixels.h"
#include "effects/SeasonalPixelArt.h"
#include "renderer/FlightPixels.h"
#include "renderer/IllustrationNode.h"
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGMaterialShader>
#include <QSGVertexColorMaterial>
#include <cstring>
#include <span>

namespace te {
namespace {
using Vertex = QSGGeometry::ColoredPoint2D;
QSGGeometryNode* makeNode() {
    auto* node = new QSGGeometryNode;
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
    geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
    node->setGeometry(geometry); node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(new QSGVertexColorMaterial); node->setFlag(QSGNode::OwnsMaterial);
    return node;
}
void syncShapes(QSGGeometryNode& node, std::span<const ShapeVertex> shapes) {
    static_assert(sizeof(ShapeVertex) == sizeof(Vertex));
    auto& geometry = *node.geometry();
    if (shapes.empty() && geometry.vertexCount() == 0) return;
    if (geometry.vertexCount() != int(shapes.size())) geometry.allocate(int(shapes.size()));
    auto* out = geometry.vertexDataAsColoredPoint2D();
    for (const auto& v : shapes) (out++)->set(v.x, v.y, v.r, v.g, v.b, v.a);
    geometry.markVertexDataDirty();
    node.markDirty(QSGNode::DirtyGeometry);
}
void vertex(Vertex& out, float x, float y, float alpha, float tint = 1.F) {
    const auto a = static_cast<uchar>(std::clamp(alpha * 255.F, 0.F, 255.F));
    // Qt's scene graph blends premultiplied colors.
    out.set(x, y, static_cast<uchar>(float(a) * tint * 0.92F), static_cast<uchar>(float(a) * tint * 0.96F), a, a);
}
void colored(Vertex& out, Vec2 point, float red, float green, float blue, float opacity) {
    const float alpha = std::clamp(opacity, 0.F, 1.F) * 255.F;
    out.set(point.x, point.y, uchar(red * alpha), uchar(green * alpha), uchar(blue * alpha), uchar(alpha));
}
void drawSnowflake(PixelCanvas& canvas, const ParticleVisual& flake) {
    const float alpha = pixelOpacity(flake.opacity);
    if (alpha == 0.F) return;
    const auto [x, y] = flake.position;
    const float reach = flake.radius + .45F;
    if (flake.radius < 1.55F) {
        const float half = reach * .65F;
        canvas.rect(x - half, y - half, half * 2.F, half * 2.F, 0xdcedff, alpha);
        return;
    }
    const float arm = std::max(.5F, flake.radius * .3F);
    const float tipAlpha = pixelOpacity(alpha * .63F);
    canvas.rect(x - arm, y - reach, arm * 2.F, reach - arm, 0xbccfef, tipAlpha);
    canvas.rect(x - arm, y + arm, arm * 2.F, reach - arm, 0xbccfef, tipAlpha);
    canvas.rect(x - reach, y - arm, reach - arm, arm * 2.F, 0xbccfef, tipAlpha);
    canvas.rect(x + arm, y - arm, reach - arm, arm * 2.F, 0xbccfef, tipAlpha);
    canvas.rect(x - arm, y - arm, arm * 2.F, arm * 2.F, 0xf4faff, alpha);
}
class LightningShader final : public QSGMaterialShader {
public:
    LightningShader() {
        setShaderFileName(VertexStage, QStringLiteral(":/tfx/shaders/lightning.vert.qsb"));
        setShaderFileName(FragmentStage, QStringLiteral(":/tfx/shaders/lightning.frag.qsb"));
    }
    bool updateUniformData(RenderState& state, QSGMaterial* material, QSGMaterial*) override;
};

class LightningMaterial final : public QSGMaterial {
public:
    LightningMaterial() { setFlag(Blending); }
    QSGMaterialType* type() const override {
        static QSGMaterialType type;
        return &type;
    }
    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override {
        return new LightningShader;
    }
    int compare(const QSGMaterial* other) const override {
        const auto otherStrength = static_cast<const LightningMaterial*>(other)->strength;
        if (strength != otherStrength) return strength < otherStrength ? -1 : 1;
        const auto otherRadius = static_cast<const LightningMaterial*>(other)->radius;
        if (radius == otherRadius) return 0;
        return radius < otherRadius ? -1 : 1;
    }
    float strength = 0.F;
    float radius = 0.F;
};

bool LightningShader::updateUniformData(RenderState& state, QSGMaterial* material, QSGMaterial*) {
    auto* data = state.uniformData()->data();
    const auto matrix = state.combinedMatrix();
    const auto& flash = *static_cast<const LightningMaterial*>(material);
    const float opacity = state.opacity() * flash.strength;
    constexpr size_t matrixBytes = 16 * sizeof(float);
    std::memcpy(data, matrix.constData(), matrixBytes);
    std::memcpy(data + matrixBytes, &opacity, sizeof(opacity));
    std::memcpy(data + matrixBytes + sizeof(opacity), &flash.radius, sizeof(flash.radius));
    return true;
}

class LightningNode final : public QSGGeometryNode {
public:
    LightningNode() {
        auto* mesh = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0);
        mesh->setDrawingMode(QSGGeometry::DrawTriangleStrip);
        setGeometry(mesh);
        setFlag(OwnsGeometry);
        setMaterial(new LightningMaterial);
        setFlag(OwnsMaterial);
    }
    void sync(std::span<const FlashVisual> flashes) {
        auto* mesh = geometry();
        if (flashes.empty()) {
            if (mesh->vertexCount() != 0) {
                mesh->allocate(0);
                markDirty(DirtyGeometry);
            }
            return;
        }

        // Rain has one active flash. One quad covers its entire radial gradient.
        const auto& flash = flashes.front();
        if (!mesh->vertexCount()) mesh->allocate(4);
        const float radius = flash.radius;
        QSGGeometry::updateTexturedRectGeometry(mesh,
            QRectF(flash.center.x - radius, flash.center.y - radius, 2*radius, 2*radius), QRectF(0, 0, 1, 1));
        auto& light = *static_cast<LightningMaterial*>(material());
        light.strength = flash.opacity;
        light.radius = radius;
        mesh->markVertexDataDirty();
        markDirty(DirtyGeometry | DirtyMaterial);
    }
};
}
EffectItem::EffectItem(IEffect& effect, QQuickItem* parent) : QQuickItem(parent), effect_(&effect) {
    setFlag(ItemHasContents, true); setClip(true);
    setAcceptedMouseButtons(Qt::NoButton); setAcceptHoverEvents(false);
}
void EffectItem::setEffect(IEffect& effect) {
    effect_ = &effect;
    textureStale_ = true;
    effect_->resize(float(width()), float(height()));
    update();
}
bool EffectItem::pressAt(QPointF point) {
    return contains(point) && effect_->pressed({float(point.x()), float(point.y())});
}
void EffectItem::advance(float dt, CursorSample cursor) {
    effect_->advance(dt, cursor);
    update();
}
void EffectItem::geometryChange(const QRectF& geometry, const QRectF& old) {
    QQuickItem::geometryChange(geometry, old);
    effect_->resize(float(geometry.width()), float(geometry.height()));
    update();
}
QSGNode* EffectItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*) {
    // Called during scene graph synchronization while Qt blocks the GUI thread.
    // The simulation and resize mutate the frame exclusively on the GUI thread.
    auto* root = old;
    if (!root) {
        root = new QSGNode;
        root->appendChildNode(new LightningNode);
        for (int i = 0; i < 5; ++i) root->appendChildNode(makeNode()); // underlay, drift, detail, flakes, rain
        root->appendChildNode(new IllustrationNode);
        root->appendChildNode(makeNode()); // falling petals and stardust
        root->appendChildNode(makeNode()); // vector overlay
    }
    auto* flashes = static_cast<LightningNode*>(root->firstChild());
    auto* underlay = static_cast<QSGGeometryNode*>(flashes->nextSibling());
    auto* drift = static_cast<QSGGeometryNode*>(underlay->nextSibling());
    auto* detail = static_cast<QSGGeometryNode*>(drift->nextSibling());
    auto* flakes = static_cast<QSGGeometryNode*>(detail->nextSibling());
    auto* rain = static_cast<QSGGeometryNode*>(flakes->nextSibling());
    auto* illustrated = static_cast<IllustrationNode*>(rain->nextSibling());
    auto* aboveArt = static_cast<QSGGeometryNode*>(illustrated->nextSibling());
    const auto& frame = effect_->frame();
    auto* overlay = static_cast<QSGGeometryNode*>(aboveArt->nextSibling());
    if (!old || textureStale_ || textureVersion_ != frame.textureVersion) {
        textureStale_ = false;
        textureVersion_ = frame.textureVersion;
        syncShapes(*underlay, frame.underlay);
        syncShapes(*detail, frame.detail);
    }
    syncShapes(*overlay, frame.overlay);
    illustrated->sync(*window(),frame.illustrations);
    const auto origin = mapToScene(QPointF{});
    PixelCanvas pixels(pixelVertices_, float(window()->effectiveDevicePixelRatio()), {float(origin.x()), float(origin.y())});
    pixelVertices_.clear();
    for (const auto& petal : frame.petals) drawSakuraPetal(pixels, petal);
    for (const auto& star : frame.ambient) paintStarPixels(pixels, star);
    syncShapes(*aboveArt, pixelVertices_);
    const auto segments = frame.snow.empty() ? 0 : frame.snow.size() - 1;
    drift->geometry()->allocate(int(segments * 12));
    auto* vertices = drift->geometry()->vertexDataAsColoredPoint2D();
    const float bottom = float(height());
    for (std::size_t i = 0; i < segments; ++i) {
        const auto a = frame.snow[i], b = frame.snow[i + 1];
        const float ay = bottom - a.height, by = bottom - b.height;
        const float aa = std::clamp(a.height / 2.F, 0.F, 0.95F);
        const float ba = std::clamp(b.height / 2.F, 0.F, 0.95F);
        // Deep snow turns a cool blue.
        const auto deep = [&](float x, float alpha) { colored(*vertices++, {x, bottom}, 0.74F, 0.85F, 0.97F, alpha); };
        vertex(*vertices++, a.x, ay, aa); deep(a.x, aa); deep(b.x, ba);
        vertex(*vertices++, a.x, ay, aa); deep(b.x, ba); vertex(*vertices++, b.x, by, ba);
        // A 1.5-point feather avoids a jagged snow silhouette without an offscreen texture.
        vertex(*vertices++, a.x, ay - 1.5F, 0); vertex(*vertices++, a.x, ay, aa); vertex(*vertices++, b.x, by, ba);
        vertex(*vertices++, a.x, ay - 1.5F, 0); vertex(*vertices++, b.x, by, ba); vertex(*vertices++, b.x, by - 1.5F, 0);
    }
    drift->geometry()->markVertexDataDirty(); drift->markDirty(QSGNode::DirtyGeometry);
    pixelVertices_.clear();
    for (const auto& flake : frame.particles) drawSnowflake(pixels, flake);
    syncShapes(*flakes, pixelVertices_);
    pixelVertices_.clear();
    for (const auto& drop : frame.rain) drawRainDrop(pixels, drop);
    for (const auto& splash : frame.splashes) drawRainSplash(pixels, splash);
    for (const auto& blade : frame.grass) drawGrassBlade(pixels, blade);
    syncShapes(*rain, pixelVertices_);
    flashes->sync(frame.flashes);
    return root;
}
}
