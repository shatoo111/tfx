#pragma once

#include "effects/Illustration.h"
#include "effects/PixelCanvas.h"

namespace te::art {

// A sparse blue wake widens and brightens toward the head at the right.
inline constexpr PixelArt cometTail{40, 7,
    "........................................"
    "..........................f....ff......."
    "............f....f....ff..fmm.fmmiif...."
    "..f...f..ff..fm.mf.fmmmmmmmmiiiiiiiWWm.."
    "............f....f.....ffmmfffmiiiim...."
    ".........................m...fmff..f...."
    "...............................f........"};
static_assert(cometTail.rows.size() == cometTail.width * cometTail.height);

// A compact corona behind the arrival flash.
inline constexpr PixelArt cometCore{11, 11,
    "..........."
    ".....f....."
    "....fff...."
    "...fmimf..."
    "..fmiWiimf."
    ".fmiWWWimf."
    "..fmiiWim.."
    "...fmimf..."
    "....fff...."
    ".....f....."
    "..........."};
static_assert(cometCore.rows.size() == cometCore.width * cometCore.height);

inline constexpr PixelArt cometFlash{15, 15,
    ".......f......."
    ".......m......."
    ".......m......."
    "...m...i...m..."
    "....m..i..m...."
    ".....miWim....."
    ".....iWWWi....."
    "fmmiiWWWWWiiimf"
    ".....iWWWi....."
    ".....miWim....."
    "....m..i..m...."
    "...m...i...m..."
    ".......m......."
    ".......m......."
    ".......f......."};
static_assert(cometFlash.rows.size() == cometFlash.width * cometFlash.height);

inline constexpr const PixelArt* flightSpriteFor(Artwork art) {
    switch (art) {
    case Artwork::Tail: return &cometTail;
    case Artwork::Halo: return &cometCore;
    case Artwork::Rays: return &cometFlash;
    default: return nullptr;
    }
}

constexpr std::uint32_t flightTone(std::uint32_t rgb, std::uint32_t alpha = 255) {
    const auto channel = [rgb, alpha](int shift) { return ((((rgb >> shift) & 255U) * alpha + 127U) / 255U) << shift; };
    return (alpha << 24) | channel(16) | channel(8) | channel(0);
}

inline constexpr std::uint32_t flightColorFor(char key) {
    switch (key) {
    // Three discrete opacity levels across the tail and corona.
    case 'f': return flightTone(0x8db6f3, 46);
    case 'm': return flightTone(0xa8cefa, 112);
    case 'i': return flightTone(0xcceeff, 208);
    case 'W': return flightTone(0xfff7df);
    default: return 0;
    }
}
}
