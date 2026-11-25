#pragma once

#include "effects/IEffect.h"
#include <array>

namespace te::foliagePixels {
struct Sprite { PixelArt art; std::span<const Tint> palette; };

// Shared bark colours tie the autumn bough to the two corner branches.
inline constexpr std::array bark{
    Tint{'K', 0x39283a}, Tint{'b', 0x604139}, Tint{'m', 0x85564b}, Tint{'h', 0xb68060}
};
inline constexpr std::array mapleColors{
    Tint{'d', 0x843337}, Tint{'v', 0xa94038}, Tint{'b', 0xdc5543}, Tint{'l', 0xf46e51}, Tint{'h', 0xff9d65}
};
inline constexpr std::array planeColors{
    Tint{'d', 0x903d2d}, Tint{'v', 0xbd502a}, Tint{'b', 0xef762b}, Tint{'l', 0xff9638}, Tint{'h', 0xffc061}
};
inline constexpr std::array birchColors{
    Tint{'d', 0x9b5d2e}, Tint{'v', 0xc48530}, Tint{'b', 0xe9ac39}, Tint{'l', 0xffc64f}, Tint{'h', 0xffe489}
};
inline constexpr std::array beechColors{
    Tint{'d', 0x94472d}, Tint{'v', 0xb76332}, Tint{'b', 0xe8883e}, Tint{'l', 0xf9ac58}, Tint{'h', 0xffcc7f}
};
inline constexpr std::array blossomColors{
    Tint{'d', 0x9f4568}, Tint{'m', 0xcf6891}, Tint{'l', 0xf08aac}, Tint{'p', 0xffc2d5}, Tint{'g', 0xffe396}
};
inline constexpr std::array roseColors{
    Tint{'d', 0x923a64}, Tint{'m', 0xc05383}, Tint{'l', 0xe56c9b}, Tint{'p', 0xfda6c6}, Tint{'g', 0xffd492}
};

// The bark matrices retain the original profiles and twig endpoints in local artwork space.

inline constexpr PixelArt bough{28, 65,
    "..........................Kb"
    "..........................Kb"
    ".........................KbK"
    ".........................Kb."
    ".........................bK."
    "........................KhK."
    "........................KhK."
    "........................bh.."
    "........................hh.."
    "........................hh.."
    "........................hh.."
    ".......................Khm.."
    "...............b.......bhm.."
    "................b.....bbhm.."
    "...........bbbbbb...bb.Khm.."
    ".........bbbb.bbbbbbb..Kh..."
    "........b..b...........Kh..."
    "...........b..........KKm..."
    "..........b...........Kb...."
    "......................Km...."
    ".....................Kbb...."
    "....................KKK....."
    "....................KK......"
    "...................KKK......"
    "...................Kb......."
    "..................KKKb......"
    "..................KKKbb....."
    ".........b........KK..b..b.."
    "......b..b........KK..bbbb.."
    "......bbbb........KK...b...."
    ".........bb.......KK...b...."
    "...........b......KK...b...."
    "............b....bbb...bb..."
    ".............bbbbbKb....b..."
    "............b.....Kb....bb.."
    "...........b.....KbK.....bb."
    "..........b......Kb.......b."
    ".................Kh........."
    "................Kbb........."
    "................Kb.........."
    "...bbbb........KKb.........."
    ".......bb......KKK.........."
    ".........b.....KK..........."
    "........bbbbbbbKK..........."
    ".......bb.....KK............"
    "..............KK............"
    ".............KK............."
    ".............KK............."
    ".............KK............."
    ".............KK.b..........."
    ".............Kbb............"
    "............KKbb............"
    "............KK..b..........."
    ".......bbbbbK...bb.........."
    ".....bb....KK...bb.........."
    "...........K.....b.........."
    "..........KK.....b.........."
    "..........K......b.........."
    "..........b......bb........."
    ".........Kb................."
    "..........b................."
    "..........b................."
    "............................"
    "............................"
    "............................"};

inline constexpr PixelArt leftBranch{60, 43,
    "KK.........................................................."
    "KKK........................................................."
    "..KbK......................................................."
    "...Kbb......................bb.............................."
    ".....bhb..................bb.bb............................."
    "......Khhmb.............bbbmbb.............................."
    "........Kmhbbbbbbbbbbbbbbbbb................................"
    "...........Kbhm....bb......................................."
    ".............Kbh.....bb....................................."
    "...............Kbm....bb...................................."
    "................KKb.....bbb............bb..................."
    "..................KK..................bb...................."
    "...................Kb................bb....................."
    "....................bKK..............b......................"
    "....................bKKK.............b............bb........"
    "....................bb.KKK..........bb......bbbbbbb........."
    "....................bb..KKKbbbbbbbbbbbbbbbbbbbbbbb.........."
    "....................bb....KKKKK...bbbbbbbbb.....bb.........."
    "....................b.......KKbbb.........bb......bbbb......"
    "....................b..........Kbb.........bb..............."
    "....................b............Kbb........b..............."
    "...................bb.............bmb......................."
    "...................bb.............bbKKK..............bbbbb.."
    "...................b...............b.KKK...........bbb......"
    "..................bb...............b...KKK......bbbb.b......"
    "..................b................b....KKK.bbbbb....b......"
    ".................bbbb..............b......KKb.........b....."
    "..............bbbbb.bb.............b.......KKK.............."
    "..............b.bb....b......bbbbbbb.........KKK............"
    "................bb.....b.........bb...........bKK..........."
    "................bb.....b.........bb...........b..KKbbbb....."
    "................bb...............b............b...bK........"
    ".................b..............bbbb..........b.....K......."
    ".................b..............bb..bb........b......K......"
    "................................b.....b......b.............."
    "................................b......b...................."
    "...............................bb..........................."
    "............................................................"
    "............................................................"
    "............................................................"
    "............................................................"
    "............................................................"
    "............................................................"};

inline constexpr PixelArt rightBranch{60, 43,
    "KK.........................................................."
    ".KKK........................................................"
    "..KbbK...............bbbbbbbbb.............................."
    "....KhhK..........bbbbbbbbbbb..............................."
    "......bhhm.......bb.bb......................................"
    ".........mhmb...bb....bbbbb................................."
    "...........Kbhmbb..........................................."
    ".............bbbb.......................b..................."
    "..............bbbb......................b.........b........."
    ".................KK.....................b.......bbb........."
    "..................KK....................bbbbbbbbb..........."
    "...................KKK.................bbbbbbbb............."
    ".....................KKK..............bb.....b.............."
    ".......................KKbK..........bb.......bbb..........."
    ".........................bKKbb.......b......................"
    ".........................b..Kbbb.....b......................"
    ".........................b....Kbb....b......................"
    ".........................bb.....KKb.bb......................"
    ".........................bb.......KKb......................."
    "..........................bb.......KKK......................"
    ".........................bbbb........KKK...................."
    ".......................bb..bbb.........KKK............b....."
    "....................bbb.....bb...........KKK.........b......"
    ".............................bb............bKK......b......."
    ".............................bb.............bbK....b........"
    ".............................bb..............bbK...b........"
    ".............................bb................bbKb........."
    ".............................bb..................bb........."
    ".............................bb..................bbbbbbbb..."
    ".............................bb..bbb............b..b........"
    ".............................bbbbbbbbb.........b...bb......."
    ".............................bb................b....bbb....."
    "..............................b............................."
    "..............................bb............................"
    "...............................bb..........................."
    "...............................bbb.........................."
    "..............................b..bb........................."
    ".............................bb...bb........................"
    "............................bb.............................."
    "............................................................"
    "............................................................"
    "............................................................"
    "............................................................"};

inline constexpr PixelArt maple{11, 14,
    "..........."
    ".....d....."
    ".....dddd.."
    "...d.dbd..."
    "...ddvbd..."
    "...dlvbvd.."
    ".dddhvvbd.."
    ".dhhvdbbbd."
    "..dlhvbvbd."
    ".dvvvdvbd.."
    "..dddvdd..."
    ".....d....."
    ".....d....."
    "..........."};

inline constexpr PixelArt plane{11, 14,
    "..........."
    ".....d....."
    "....dvd...."
    "...ddvbd..."
    "..dhvvbbd.."
    "..dvldbvd.."
    ".dlhvdvbbdd"
    ".dvvvdbbvd."
    "dhlvvdvvd.."
    "..hhhdbd..."
    "..dddvd...."
    ".....d....."
    ".....d....."
    "..........."};

inline constexpr PixelArt birch{11, 14,
    "..........."
    ".....d....."
    "....dvd...."
    "...dhvbd..."
    "..dhhdbd..."
    "..dlvvvbd.."
    "..dvhdbbd.."
    ".dhvvvvvbd."
    ".dlvvdbvbd."
    "..dhvvvbd.."
    "..ddbdbd..."
    "....ddd...."
    ".....d....."
    "..........."};

inline constexpr PixelArt beech{11, 14,
    "..........."
    "......d...."
    ".....dvd..."
    "...ddbvd..."
    "..dhhvvd..."
    "..dlvdvbd.."
    ".dlhhvbvd.."
    "..dvvdvbd.."
    "..dlhvbv..."
    "...vvdvd..."
    "...dddd...."
    ".....d....."
    ".....d....."
    "..........."};

inline constexpr PixelArt bloom{11, 11,
    ".....mm...."
    "....mplm..."
    "..mmplllm.."
    ".mplmlml.m."
    ".mllmdgmlpm"
    "..mmdgdmllm"
    "...mgdmllm."
    "..mplmmpm.."
    "..mllmllm.."
    "...mmmlm..."
    "......m...."};

inline constexpr PixelArt rose{11, 11,
    ".....mm...."
    "....mlpm..."
    "..mmlll...."
    ".mlpmm..mm."
    ".mllmdgmlpm"
    "..mmdgdmllm"
    "...mgdmllm."
    "..mlmmlmm.."
    ".mllmllpm.."
    "..mm.mlm..."
    "......m...."};

inline constexpr PixelArt bud{7, 9,
    "....m.."
    "...mld."
    ".dmlld."
    ".dmlmd."
    ".dmlmd."
    ".ddmdd."
    "..ddd.."
    "...d..."
    "......."};

inline const Sprite* spriteFor(Artwork art) {
    static constexpr Sprite sprites[] = {
        {bough, bark}, {maple, mapleColors}, {plane, planeColors}, {birch, birchColors}, {beech, beechColors},
        {leftBranch, bark}, {rightBranch, bark}, {bloom, blossomColors}, {rose, roseColors}, {bud, blossomColors}
    };
    switch (art) {
    case Artwork::Bough: return &sprites[0];
    case Artwork::Maple: return &sprites[1];
    case Artwork::Plane: return &sprites[2];
    case Artwork::Birch: return &sprites[3];
    case Artwork::Beech: return &sprites[4];
    case Artwork::SakuraLeft: return &sprites[5];
    case Artwork::SakuraRight: return &sprites[6];
    case Artwork::CherryBloom: return &sprites[7];
    case Artwork::CherryRose: return &sprites[8];
    case Artwork::CherryBud: return &sprites[9];
    default: return nullptr;
    }
}
} // namespace te::foliagePixels

namespace te {
inline void drawSakuraPetal(PixelCanvas& canvas, const PetalVisual& petal) {
    constexpr std::array<Vec2, 14> outline{{
        {0,-.69F},{.22F,-1},{.48F,-.86F},{.62F,-.54F},{.62F,-.15F},
        {.46F,.36F},{.22F,.77F},{0,1},{-.22F,.77F},{-.46F,.36F},
        {-.62F,-.15F},{-.62F,-.54F},{-.48F,-.86F},{-.22F,-1}
    }};
    constexpr std::array<Vec2, 5> glint{{
        {-.49F,-.51F},{-.40F,-.71F},{-.22F,-.83F},{-.20F,-.39F},{-.36F,.19F}
    }};
    constexpr std::array<std::uint32_t, 4> pink{0xd76695, 0xe978a5, 0xf58ab1, 0xf99cbf};
    const float alpha = pixelOpacity(petal.opacity);
    if (alpha <= 0 || petal.radius <= 0) return;
    const float angleCos = std::cos(petal.angle), angleSin = std::sin(petal.angle);
    const float width = petal.radius * petal.flutter;
    const auto place = [&](Vec2 local) {
        const auto x = local.x * width, y = local.y * petal.radius;
        return Vec2{petal.position.x + x*angleCos - y*angleSin,
                    petal.position.y + x*angleSin + y*angleCos};
    };
    std::array<Vec2, outline.size()> border{};
    std::transform(outline.begin(), outline.end(), border.begin(), place);
    const auto shade = std::clamp(int(petal.shade * float(pink.size())), 0, int(pink.size()) - 1);
    constexpr float cell = 1.5F;
    canvas.pixelPolygon(border, cell, pink[std::size_t(shade)], alpha);
    std::array<Vec2, glint.size()> light{};
    std::transform(glint.begin(), glint.end(), light.begin(), place);
    canvas.pixelPolygon(light, cell, 0xffc1d5, alpha);
}
} // namespace te
