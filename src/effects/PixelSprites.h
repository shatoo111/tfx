#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include "effects/Illustration.h"

// Original 8-bit artwork for the "8 бит" theme, painted pixel by pixel.
// Each sprite is a grid of palette keys; '.' is transparent. Heroes face right.
namespace te::pixel {
struct Swatch { char key; std::uint32_t argb; };
struct Sprite {
    int width, height;
    std::string_view rows;     // width*height keys, row-major.
};

inline constexpr std::array<Swatch, 30> swatches{{
    {'K', 0xff2a2238}, // outline
    {'W', 0xfffff8ec}, // white
    {'E', 0xff2a2238}, // eye
    // Cat
    {'O', 0xfff2a54a}, {'o', 0xffc9742c}, {'P', 0xffff8fa8}, {'B', 0xff2f7de1}, {'b', 0xff74b6ff},
    // Knight
    {'S', 0xffd3dbe6}, {'s', 0xff8c99ad}, {'F', 0xfff4c9a0}, {'R', 0xff3a6fd8}, {'r', 0xff244a9a},
    {'N', 0xff8a5a34}, {'G', 0xffffcf3f},
    // Robot
    {'M', 0xffe3eaf2}, {'m', 0xff9fadbf}, {'C', 0xff4fe3ff}, {'A', 0xffffd23f}, {'T', 0xff2aa3e8},
    // Spawners
    {'D', 0xffd59a5a}, {'d', 0xffa8713c}, {'Q', 0xff9aa3ad}, {'q', 0xff6c7480}, {'V', 0xff3c4450},
    // Coin, sparkle, dust
    {'Y', 0xffffd23f}, {'y', 0xffe09a00}, {'L', 0xfffff4a8}, {'Z', 0xff8a5a00}, {'U', 0xffe9eef5},
}};

// ---------------------------------------------------------------- Cat in a cap
inline constexpr Sprite catIdle{16, 16,
    "................"
    "...K.....K......"
    "..KOK...KOK....."
    "..KBBBBBBBBK...."
    ".KBbbBBBBBBBKKK."
    ".KBBBBBBBBBBBBK."
    ".KOOOOOOOOOKKK.."
    ".KOOWKOOOWKOK..."
    ".KOOKKOOOKKOK..."
    ".KoOOOPPOOOoK..."
    "..KoOOOOOOoK...."
    "K..KKKOOOKK....."
    "OKKOOWWWWWOK...."
    "KOOOOWWWWWOK...."
    ".KKOOOOOOOOK...."
    "...KKK..KKK....."};
inline constexpr Sprite catRunA{16, 16,
    "................"
    "...K.....K......"
    "..KOK...KOK....."
    "..KBBBBBBBBK...."
    ".KBbbBBBBBBBKKK."
    ".KBBBBBBBBBBBBK."
    ".KOOOOOOOOOKKK.."
    ".KOOWKOOOWKOK..."
    ".KOOKKOOOKKOK..."
    ".KoOOOPPOOOoK..."
    "..KoOOOOOOoK...."
    "K..KKKOOOKK....."
    "OKKOOWWWWWOK...."
    "KOOOOWWWWWOOK..."
    ".KKOOOKKKOOOOK.."
    "..KKK.....KKK..."};
inline constexpr Sprite catRunB{16, 16,
    "................"
    "................"
    "...K.....K......"
    "..KOK...KOK....."
    "..KBBBBBBBBK...."
    ".KBbbBBBBBBBKKK."
    ".KBBBBBBBBBBBBK."
    ".KOOOOOOOOOKKK.."
    ".KOOWKOOOWKOK..."
    ".KOOKKOOOKKOK..."
    ".KoOOOPPOOOoK..."
    "K.KKKKOOOKKK...."
    "OKKOOWWWWWOK...."
    "KOOOOWWWWWOK...."
    ".KKKOOOOOOKK...."
    "....KKKKKK......"};
inline constexpr Sprite catJump{16, 16,
    "...K.....K......"
    "..KOK...KOK....."
    "..KBBBBBBBBK...."
    ".KBbbBBBBBBBKKK."
    ".KBBBBBBBBBBBBK."
    ".KOOOOOOOOOKKK.."
    ".KOOWKOOOWKOK..."
    ".KOOKKOOOKKOK..."
    ".KoOOOPPOOOoK..."
    "..KoOOOOOOoKKK.."
    "...KKOOOOOKKOK.."
    "..KOOWWWWWOOKK.."
    ".KOOOWWWWWOK...."
    "KOKOOOOOOOOK...."
    "KK.KKOOOOKK....."
    "......KKKK......"};

// ----------------------------------------------------------- Little knight
inline constexpr Sprite knightIdle{16, 16,
    "......GG........"
    ".....GRRG......."
    "....KKKKKK......"
    "...KSSSSSSK....."
    "..KSSSSSSSsK...."
    "..KSKKKKKKsK...."
    "..KSFFEFFEsK...."
    "..KSFFFFFFsK...."
    "...KKKKKKKK....."
    "..KRRRRRRRRK.KS."
    ".KSKRRGRRRKSKS.."
    ".KSKRRRRRRKKSK.."
    "..KKRRRRRRK.K..."
    "...KrrKKrrK....."
    "...KNNK.KNNK...."
    "...KKKK.KKKK...."};
inline constexpr Sprite knightRunA{16, 16,
    "......GG........"
    ".....GRRG......."
    "....KKKKKK......"
    "...KSSSSSSK....."
    "..KSSSSSSSsK...."
    "..KSKKKKKKsK...."
    "..KSFFEFFEsK...."
    "..KSFFFFFFsK...."
    "...KKKKKKKK..KS."
    "..KRRRRRRRRKKS.."
    ".KSKRRGRRRKSK..."
    ".KSKRRRRRRKK...."
    "..KKRRRRRRK....."
    "..KrrK..KrrK...."
    ".KNNK....KNNK..."
    ".KKKK.....KKKK.."};
inline constexpr Sprite knightRunB{16, 16,
    "................"
    "......GG........"
    ".....GRRG......."
    "....KKKKKK......"
    "...KSSSSSSK....."
    "..KSSSSSSSsK...."
    "..KSKKKKKKsK...."
    "..KSFFEFFEsK...."
    "..KSFFFFFFsK.KS."
    "...KKKKKKKKKKS.."
    "..KRRRRRRRRKK..."
    ".KSKRRGRRRKK...."
    ".KSKRRRRRRK....."
    "..KKrrKKrrK....."
    "....KNNNNK......"
    "....KKKKKK......"};
inline constexpr Sprite knightJump{16, 16,
    "......GG....KS.."
    ".....GRRG..KS..."
    "....KKKKKKKS...."
    "...KSSSSSSK....."
    "..KSSSSSSSsK...."
    "..KSKKKKKKsK...."
    "..KSFFEFFEsK...."
    "..KSFFFFFFsK...."
    "...KKKKKKKK....."
    "..KRRRRRRRRK...."
    ".KSKRRGRRRKK...."
    ".KSKRRRRRRK....."
    "..KKRRRRRRK....."
    "..KrrKKKrrKK...."
    ".KNNK...KNNK...."
    ".KKK.....KKK...."};

// ------------------------------------------------------------- Courier robot
inline constexpr Sprite robotIdle{16, 16,
    ".......A........"
    ".......K........"
    "....KKKKKKK....."
    "...KMMMMMMMK...."
    "...KMVVVVVMK...."
    "...KMVCVCVMK...."
    "...KMVVVVVMK...."
    "...KmMMMMMmK...."
    "....KKKKKKK....."
    "..KKMMMMMMMKK..."
    ".KMKMTTTTTMKMK.."
    ".KMKMTWWTTMKMK.."
    ".KKKMMMMMMMKKK.."
    "...KmmKKKmmK...."
    "..KMMMK.KMMMK..."
    "..KKKKK.KKKKK..."};
inline constexpr Sprite robotRunA{16, 16,
    ".......A........"
    ".......K........"
    "....KKKKKKK....."
    "...KMMMMMMMK...."
    "...KMVVVVVMK...."
    "...KMVVCVCMK...."
    "...KMVVVVVMK...."
    "...KmMMMMMmK...."
    "....KKKKKKK....."
    "...KMMMMMMMKKK.."
    "..KMMTTTTTMKMK.."
    ".KMKMTWWTTMKKK.."
    ".KKKMMMMMMMK...."
    "...KmmK.KmmK...."
    "..KMMMK..KMMMK.."
    "..KKKKK..KKKKK.."};
inline constexpr Sprite robotRunB{16, 16,
    "................"
    ".......A........"
    ".......K........"
    "....KKKKKKK....."
    "...KMMMMMMMK...."
    "...KMVVVVVMK...."
    "...KMVVCVCMK...."
    "...KMVVVVVMK...."
    "...KmMMMMMmK...."
    "..KKKKKKKKKKK..."
    ".KMKMTTTTTMKMK.."
    ".KMKMTWWTTMKMK.."
    ".KKKMMMMMMMKKK.."
    "....KmmmmmK....."
    "....KMMMMMK....."
    "....KKKKKKK....."};
inline constexpr Sprite robotJump{16, 16,
    ".......A........"
    ".......K........"
    "....KKKKKKK....."
    "...KMMMMMMMK...."
    "...KMVVVVVMK...."
    "...KMVVCVCMK...."
    "...KMVVVVVMK...."
    ".KKKmMMMMMmKKK.."
    ".KMKKKKKKKKKMK.."
    ".KMKMMMMMMMKMK.."
    "..KKMTTTTTMKK..."
    "....MTWWTTM....."
    "...KMMMMMMMK...."
    "..KmmK...KmmK..."
    "..KAAK...KAAK..."
    "...KK.....KK...."};

// ---------------------------------------------------------------- Spawners
inline constexpr Sprite crate{20, 14,
    "..KK............KK.."
    ".KDDK..........KDDK."
    "KDDDDK........KDDDDK"
    ".KKKKKKKKKKKKKKKKKK."
    "KDDDDDDDDDDDDDDDDDDK"
    "KDdDDDDDDDDDDDDDDdDK"
    "KDDDDDDDdddDDDDDDDDK"
    "KDDDDDDDDdDDDDDDDDDK"
    "KDDDDDDDDdDDDDDDDDDK"
    "KDDDDDDDDDDDDDDDDDDK"
    "KDdDDDDDDDDDDDDDDdDK"
    "KDDDDDDDDDDDDDDDDDDK"
    "KddddddddddddddddddK"
    "KKKKKKKKKKKKKKKKKKKK"};
inline constexpr Sprite well{20, 14,
    "KKKKKKKKKKKKKKKKKKKK"
    "KQQQQQQQQQQQQQQQQQQK"
    "KqqqqqqqqqqqqqqqqqqK"
    ".KVVVVVVVVVVVVVVVVK."
    "KQQQKQQQQKQQQQKQQQQK"
    "KQQQKQQQQKQQQQKQQQQK"
    "KqqqKqqqqKqqqqKqqqqK"
    "KKKKKKKKKKKKKKKKKKKK"
    "KQQKQQQQKQQQQKQQQQQK"
    "KQQKQQQQKQQQQKQQQQQK"
    "KqqKqqqqKqqqqKqqqqqK"
    "KKKKKKKKKKKKKKKKKKKK"
    "KQQQQKQQQQKQQQQKQQQK"
    "KKKKKKKKKKKKKKKKKKKK"};
inline constexpr Sprite hatch{20, 14,
    "...................."
    "...................."
    "....C..........C...."
    "...C............C..."
    "....C..........C...."
    "...C............C..."
    "....C..........C...."
    "...C............C..."
    "....CCCCCCCCCCCC...."
    "..KKKKKKKKKKKKKKKK.."
    ".KMMMMMMMMMMMMMMMMK."
    "KmMMAMMMMMMMMMMAMMmK"
    "KmmmmmmmmmmmmmmmmmmK"
    "KKKKKKKKKKKKKKKKKKKK"};

// ------------------------------------------------------------ Coins & bits
inline constexpr Sprite coinFace{10, 10,
    "...KKKK..."
    ".KKYYYYKK."
    ".KYLLYYyK."
    "KYLZZZZYyK"
    "KYLYZZYYyK"
    "KYYYZZYYyK"
    "KYYYZZYYyK"
    ".KYYYYYyK."
    ".KKyyyyKK."
    "...KKKK..."};
inline constexpr Sprite coinTurn{10, 10,
    "....KK...."
    "...KYYK..."
    "..KYLYyK.."
    "..KLYZyK.."
    "..KLYZyK.."
    "..KYYZyK.."
    "..KYZZyK.."
    "..KYYYyK.."
    "...KyyK..."
    "....KK...."};
inline constexpr Sprite coinEdge{10, 10,
    "....KK...."
    "....KK...."
    "....KK...."
    "...KLyK..."
    "...KLyK..."
    "...KLyK..."
    "...KLyK..."
    "....KK...."
    "....KK...."
    "....KK...."};
inline constexpr Sprite sparkle{9, 9,
    "....L...."
    "....Y...."
    "....L...."
    "........."
    "LYL...LYL"
    "........."
    "....L...."
    "....Y...."
    "....L...."};
inline constexpr Sprite dust{8, 5,
    "..UU...."
    ".UUUU.U."
    "UUWWUUUU"
    ".UUUUUU."
    "..U..U.."};

// Lookup shared by the simulation (sizes, pivots) and the renderer (textures).
inline constexpr const Sprite* spriteFor(Artwork art) {
    switch (art) {
    case Artwork::PixelCoinFace: return &coinFace;
    case Artwork::PixelCoinTurn: return &coinTurn;
    case Artwork::PixelCoinEdge: return &coinEdge;
    case Artwork::PixelDust: return &dust;
    case Artwork::CatIdle: return &catIdle;
    case Artwork::CatRunA: return &catRunA;
    case Artwork::CatRunB: return &catRunB;
    case Artwork::CatJump: return &catJump;
    case Artwork::KnightIdle: return &knightIdle;
    case Artwork::KnightRunA: return &knightRunA;
    case Artwork::KnightRunB: return &knightRunB;
    case Artwork::KnightJump: return &knightJump;
    case Artwork::RobotIdle: return &robotIdle;
    case Artwork::RobotRunA: return &robotRunA;
    case Artwork::RobotRunB: return &robotRunB;
    case Artwork::RobotJump: return &robotJump;
    case Artwork::PixelCrate: return &crate;
    case Artwork::PixelWell: return &well;
    case Artwork::PixelPad: return &hatch;
    case Artwork::PixelSparkle: return &sparkle;
    default: return nullptr;
    }
}
inline constexpr std::uint32_t colorFor(char key) {
    for (const auto& swatch : swatches) if (swatch.key == key) return swatch.argb;
    return 0;
}
// Textures carry one transparent pixel on every side so nearest sampling never bleeds.
inline constexpr int padding = 1;
}
