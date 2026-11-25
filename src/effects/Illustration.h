#pragma once
#include <cstdint>

namespace te {
// Painter's order, back to front. Each part has its own local UV space.
enum class Artwork : std::uint8_t {
    Bough, Maple, Plane, Birch, Beech, Tail, Halo, Rays,
    SakuraLeft, SakuraRight, CherryBloom, CherryRose, CherryBud,
    // Pixel art for the 8-bit theme: coins and dust behind the hero, spawners in front of it.
    PixelCoinFace, PixelCoinTurn, PixelCoinEdge, PixelDust,
    CatIdle, CatRunA, CatRunB, CatJump,
    KnightIdle, KnightRunA, KnightRunB, KnightJump,
    RobotIdle, RobotRunA, RobotRunB, RobotJump,
    PixelCrate, PixelWell, PixelPad, PixelSparkle, Count
};
inline constexpr bool isPixelArt(Artwork art) { return art >= Artwork::PixelCoinFace && art < Artwork::Count; }
inline constexpr int maxFlights = 3;
struct Illustration {
    Artwork art;
    float x, y;               // Anchor on the overlay in logical points.
    float width, height;      // Signed dimensions allow mirroring around the anchor.
    float pivotX, pivotY;     // Anchor in the illustration, normalized.
    float angle = 0;
    float opacity = 1;
    int flightSlot = 0; // Separate fades for each flight.
};
}
