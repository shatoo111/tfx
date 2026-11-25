#pragma once
#include "effects/Illustration.h"
#include <QSGNode>
#include <array>
#include <memory>
#include <span>

class QQuickWindow;
class QImage;
namespace te {
const QImage& rasterArt(Artwork art);
class IllustrationNode final : public QSGNode {
public:
    IllustrationNode();
    ~IllustrationNode() override;
    void sync(QQuickWindow& window, std::span<const Illustration> parts);
private:
    struct Batch;
    struct Layer;
    std::array<std::unique_ptr<Layer>, size_t(Artwork::Count)> layers_;
};
}
