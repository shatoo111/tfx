#include "renderer/IllustrationNode.h"
#include "effects/FlightSprites.h"
#include "effects/PixelSprites.h"
#include "effects/SeasonalPixelArt.h"
#include <QImage>
#include <QPainter>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGTexture>
#include <QSGOpacityNode>
#include <QSGTextureMaterial>
#include <QSvgRenderer>
#include <algorithm>
#include <cmath>
#include <mutex>

namespace te {
namespace {
constexpr int opacitySlots = maxFlights;
constexpr std::array files{"bough","maple","plane","birch","beech","comet","glow","rays",
                           "sakura-left","sakura-right","cherry-bloom","cherry-rose","cherry-bud",
                           // Pixel art is generated from PixelSprites.h; names are for diagnostics only.
                           "coin-face","coin-turn","coin-edge","dust",
                           "cat-idle","cat-run-a","cat-run-b","cat-jump",
                           "knight-idle","knight-run-a","knight-run-b","knight-jump",
                           "robot-idle","robot-run-a","robot-run-b","robot-jump",
                           "crate","well","pad","sparkle"};
static_assert(files.size() == size_t(Artwork::Count));
using Vertex = QSGGeometry::TexturedPoint2D;
void writeSprite(Vertex*& out, const Illustration& p) {
    const float c = std::cos(p.angle), s = std::sin(p.angle);
    const auto point = [&](float u, float v) {
        const float x = (u-p.pivotX)*p.width, y = (v-p.pivotY)*p.height;
        Vertex vert; vert.set(p.x+x*c-y*s,p.y+x*s+y*c,u,v); return vert;
    };
    const Vertex a = point(0,0), b = point(0,1), d = point(1,0), e = point(1,1);
    for (const auto& corner : {a, b, d, d, b, e}) *out++ = corner;
}
template <class Sprite, class ColorFor>
QImage paintPixels(const Sprite& sprite, int padding, ColorFor colorFor) {
    QImage image(sprite.width + 2*padding, sprite.height + 2*padding, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    for (int y = 0; y < sprite.height; ++y)
        for (int x = 0; x < sprite.width; ++x) {
            const char key = sprite.rows[std::size_t(y*sprite.width + x)];
            if (key != '.') image.setPixel(x + padding, y + padding, colorFor(key));
        }
    return image;
}
QImage paintSvg(Artwork art) {
    QSvgRenderer svg(QStringLiteral(":/assets/themes/")+QString::fromLatin1(files[size_t(art)])+QStringLiteral(".svg"));
    QImage image(svg.defaultSize()*3,QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image); svg.render(&painter);
    return image;
}
QImage paintArt(Artwork artwork) {
    if (const auto* sprite = foliagePixels::spriteFor(artwork)) {
        return paintPixels(sprite->art, 0, [palette = sprite->palette](char key) {
            const auto color = std::ranges::find(palette, key, &Tint::key);
            return color == palette.end() ? 0U : 0xff000000U | color->rgb;
        });
    }
    if (const auto* sprite = art::flightSpriteFor(artwork))
        return paintPixels(*sprite, 0, art::flightColorFor);
    if (const auto* sprite = pixel::spriteFor(artwork)) return paintPixels(*sprite, pixel::padding, pixel::colorFor);
    return paintSvg(artwork);
}
}
const QImage& rasterArt(Artwork art) {
    struct Entry {
        std::once_flag ready;
        QImage raster;
    };
    static std::array<Entry, files.size()> cache;
    auto& entry = cache[size_t(art)];
    // Preview and overlay may have separate render threads and GPU textures.
    std::call_once(entry.ready, [&] { entry.raster = paintArt(art); });
    return entry.raster;
}
struct IllustrationNode::Batch {
    // Nodes are owned by the scene graph. Texture lifetime is scoped to this window.
    QSGOpacityNode* opacity;
    QSGGeometryNode* node;
    Batch(QSGTexture* texture, QSGNode& parent, bool crisp) {
        auto* material = new QSGTextureMaterial;
        material->setTexture(texture);
        // Pixel art keeps hard edges; illustrations are smoothed and mipmapped.
        material->setFiltering(crisp ? QSGTexture::Nearest : QSGTexture::Linear);
        material->setMipmapFiltering(crisp ? QSGTexture::None : QSGTexture::Linear);
        opacity = new QSGOpacityNode;
        node = new QSGGeometryNode;
        auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(),0);
        geometry->setDrawingMode(QSGGeometry::DrawTriangles);
        geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
        node->setGeometry(geometry); node->setFlag(QSGNode::OwnsGeometry);
        node->setMaterial(material); node->setFlag(QSGNode::OwnsMaterial);
        parent.appendChildNode(opacity); opacity->appendChildNode(node);
    }
};
struct IllustrationNode::Layer : QSGNode {
    std::unique_ptr<QSGTexture> texture;
    std::array<std::unique_ptr<Batch>,opacitySlots> batches;
    bool crisp;
    Layer(QQuickWindow& window, Artwork artwork)
        : crisp(isPixelArt(artwork) || foliagePixels::spriteFor(artwork) || art::flightSpriteFor(artwork)) {
        texture.reset(window.createTextureFromImage(rasterArt(artwork),
            crisp ? QQuickWindow::CreateTextureOptions{} : QQuickWindow::TextureHasMipmaps));
    }
};
IllustrationNode::IllustrationNode() = default;
IllustrationNode::~IllustrationNode() = default;
void IllustrationNode::sync(QQuickWindow& window, std::span<const Illustration> parts) {
    std::array<std::array<int,opacitySlots>,files.size()> counts{};
    std::array<std::array<Vertex*,opacitySlots>,files.size()> write{};
    for (const auto& p : parts) counts[size_t(p.art)][p.flightSlot] += 6;
    for (size_t i=0; i<files.size(); ++i) {
        auto& layer = layers_[i];
        if (!layer && std::any_of(counts[i].begin(), counts[i].end(), [](int n) { return n > 0; })) {
            layer = std::make_unique<Layer>(window,Artwork(i));
            QSGNode* next = nullptr;
            for (size_t j=i+1;j<files.size();++j) {
                if (layers_[j]) { next = layers_[j].get(); break; }
            }
            if (next) insertChildNodeBefore(layer.get(), next);
            else appendChildNode(layer.get());
        }
        if (!layer) continue;
        for (size_t lane=0;lane<layer->batches.size();++lane) {
            auto& b = layer->batches[lane];
            const int count = counts[i][lane];
            if (!b && count) b = std::make_unique<Batch>(layer->texture.get(),*layer,layer->crisp);
            if (!b) continue;
            if (!count) { b->opacity->setOpacity(0); continue; }
            auto* geo = b->node->geometry();
            if (geo->vertexCount()!=count) geo->allocate(count);
            write[i][lane] = geo->vertexDataAsTexturedPoint2D();
            geo->markVertexDataDirty();
            b->node->markDirty(QSGNode::DirtyGeometry);
        }
    }
    for (const auto& p : parts) {
        auto& batch = layers_[size_t(p.art)]->batches[p.flightSlot];
        batch->opacity->setOpacity(isPixelArt(p.art) ? p.opacity : pixelOpacity(p.opacity));
        auto& dest = write[size_t(p.art)][p.flightSlot];
        writeSprite(dest,p);
    }
}
}
