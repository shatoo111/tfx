#pragma once
#include "effects/PixelCanvas.h"
#include <array>

// Original pixel art for the small scenes on the bottom edge. Creatures face right.
namespace te::art {
// ------------------------------------------------------------------- Cat
inline constexpr PixelArt catSit{16, 15,
    "...K.....K......"
    "..KbK...KbK....."
    "..KbbKKKbbK....."
    ".KbsbsbsbsbK...."
    ".KbbbbbbbbbK...."
    ".KbWEbbbWEbK...."
    ".KbEEbbbEEbK...."
    ".KbbblnlbbbK...."
    ".KsbblllbbsK...."
    "..KKbbbbbKK....."
    "K..KblllbK......"
    "bKKbblllbbK....."
    "KbKbblllbbK....."
    ".KbbbbbbbbK....."
    "..KllKKllK......"};
inline constexpr PixelArt catSitWag{16, 15,
    "...K.....K......"
    "..KbK...KbK....."
    "..KbbKKKbbK....."
    ".KbsbsbsbsbK...."
    ".KbbbbbbbbbK...."
    ".KbWEbbbWEbK...."
    ".KbEEbbbEEbK...."
    "KKbbblnlbbbK...."
    "bKsbblllbbsK...."
    "KbKKbbbbbKK....."
    "KbKKblllbK......"
    ".KbbblllbbK....."
    "..KbblllbbK....."
    "..KbbbbbbbK....."
    "..KllKKllK......"};
inline constexpr PixelArt catBlink{16, 15,
    "...K.....K......"
    "..KbK...KbK....."
    "..KbbKKKbbK....."
    ".KbsbsbsbsbK...."
    ".KbbbbbbbbbK...."
    ".KbbbbbbbbbK...."
    ".KbKKbbbKKbK...."
    ".KbbblnlbbbK...."
    ".KsbblllbbsK...."
    "..KKbbbbbKK....."
    "K..KblllbK......"
    "bKKbblllbbK....."
    "KbKbblllbbK....."
    ".KbbbbbbbbK....."
    "..KllKKllK......"};
inline constexpr PixelArt catHappy{16, 15,
    "...K.....K......"
    "..KbK...KbK....."
    "..KbbKKKbbK....."
    ".KbsbsbsbsbK...."
    ".KbbbbbbbbbK...."
    ".KbbKbbbKbbK...."
    ".KbKbKbKbKbK...."
    ".KpbblnlbbpK...."
    ".KsbblllbbsK...."
    "K.KKbbbbbKK....."
    "bK.KblllbK......"
    "KbKbblllbbK....."
    ".KbbblllbbK....."
    "..KbbbbbbbK....."
    "..KllKKllK......"};
inline constexpr PixelArt catWalkA{20, 14,
    "...........K...K...."
    "..........KbK.KbK..."
    "K.........KbbKbbK..."
    "bK........KbsbsbbK.."
    "KbK.......KbbbbbbK.."
    ".KbKKKKKKKKbWEbWEK.."
    "..KbsbsbsbbbEEbEEK.."
    "..KbsbsbsbbbblnlbK.."
    "..KbbbbbbbbbbblllK.."
    "..KbbbbbbbbbbKKKK..."
    "..KbllllllllbK......"
    "..KbbKK..KKbbK......"
    ".KbbK......KbbK....."
    ".KKK........KKK....."};
inline constexpr PixelArt catWalkB{20, 14,
    "...........K...K...."
    "..........KbK.KbK..."
    ".K........KbbKbbK..."
    "KbK.......KbsbsbbK.."
    "KbK.......KbbbbbbK.."
    ".KbKKKKKKKKbWEbWEK.."
    "..KbsbsbsbbbEEbEEK.."
    "..KbsbsbsbbbblnlbK.."
    "..KbbbbbbbbbbblllK.."
    "..KbbbbbbbbbbKKKK..."
    "..KbllllllllbK......"
    "...KbbKKKKbbK......."
    "....KbbKKbbK........"
    "....KKK..KKK........"};
inline constexpr PixelArt catSleepA{20, 10,
    "....K...K..........."
    "...KbK.KbK.KKKKK...."
    "...KbbKbbKKbbsbbKK.."
    "..KbsbsbsbKbsbbsbbK."
    "..KbbbbbbbbKbsbbsbbK"
    "..KbKKbbKKbKbbbbbbbK"
    "..KbbblnlbbKbbbbbbbK"
    "..KsbbllbbsKbbbbbbbK"
    "...KKbbbbKKbbllllbbK"
    "....KKKKKKKKKKKKKKK."};
inline constexpr PixelArt catSleepB{20, 10,
    "...........KKKKK...."
    "....K...K.KbbsbbKK.."
    "...KbK.KbKKbsbbsbbK."
    "...KbbKbbKKbsbbsbbK."
    "..KbsbsbsbbKbbbbbbbK"
    "..KbKKbbKKbKbbbbbbbK"
    "..KbbblnlbbKbbbbbbbK"
    "..KsbbllbbsKbbbbbbbK"
    "...KKbbbbKKbbllllbbK"
    "....KKKKKKKKKKKKKKK."};
inline constexpr PixelArt catPaw{6, 4,
    "..KKK."
    ".KblpK"
    "KbbKK."
    "KbK..."};
inline constexpr PixelArt yarnBall{8, 8,
    "..KKKK.."
    ".KyYyyK."
    "KyYyyYyK"
    "KYyyYyyK"
    "KyyYyyYK"
    "KyYyyYyK"
    ".KyyYyK."
    "..KKKK.."};
inline constexpr PixelArt heart{7, 6,
    ".KK.KK."
    "KhhKhhK"
    "KhHhhhK"
    ".KhhhK."
    "..KhK.."
    "...K..."};
inline constexpr PixelArt letterZ{4, 4,
    "zzzz"
    "..z."
    ".z.."
    "zzzz"};

// ---------------------------------------------------------------- New Year
inline constexpr PixelArt tree{12, 16,
    ".....yy....."
    "....yYYy...."
    ".....gg....."
    "....wggg...."
    "...wwgggg..."
    "....gggd...."
    "...wg1ggd..."
    "..wwggggdd.."
    "...ggg2gd..."
    "..wggggggd.."
    ".ww3gggg1gd."
    "..gggggggd.."
    ".wgg2ggg3gd."
    "wwgggg1gggdd"
    ".....tt....."
    ".....tt....."};
inline constexpr PixelArt smallTree{8, 11,
    "...yy..."
    "...gg..."
    "..wggd.."
    "...gg..."
    "..w1gd.."
    ".wwgggd."
    "..gg2d.."
    ".w3gggd."
    "wwggg1dd"
    "...tt..."
    "...tt..."};
inline constexpr PixelArt snowman{10, 14,
    "...KKKK..."
    "...KKKK..."
    "...rrrr..."
    "..KKKKKK.."
    "..wwwwww.."
    "..wKwwKw.."
    "..wwwoooo."
    "..wwwwwv.."
    "t.rrrrrr.t"
    ".twwwrwvt."
    ".wwKwrwwv."
    "wwwwwwwwvv"
    "wwwKwwwwvv"
    ".wwwwwwvv."};
inline constexpr PixelArt giftBox{8, 8,
    ".yy..yy."
    "..yyyy.."
    "RRRyyRRR"
    "rrryyrrr"
    "rrryyrrr"
    "rrryyrrr"
    "rrryyrrr"
    "DDDyyDDD"};

// -------------------------------------------------------------- Black hole
inline constexpr PixelArt planet{7, 6,
    "..ppp.."
    ".pPPpq."
    "pPpppqq"
    "pbbbbqq"
    ".ppqqq."
    "..qqq.."};
inline constexpr PixelArt ringed{11, 7,
    "....ppp...."
    "...pPppq..."
    "...ppppqrr."
    ".rrrrrrrrr."
    "rrpppqqq..."
    "...ppqqq..."
    "....qqq...."};
inline constexpr PixelArt rocket{10, 5,
    "..rww....."
    "frwwwwwr.."
    "ffwwbbwwr."
    "frwwwwwr.."
    "..rww....."};
inline constexpr PixelArt saucer{10, 5,
    "...cccc..."
    "..cCcccc.."
    "mmmmmmmmmm"
    ".myMmyMmy."
    "...mmmm..."};

// ---------------------------------------------------------------- Aquarium
inline constexpr PixelArt clown{10, 6,
    "....OOO..."
    "O..OWOOWO."
    "OOOOWOOWEO"
    "OOOOWOOWOO"
    "o..oWooWo."
    "....ooo..."};
inline constexpr PixelArt tang{10, 6,
    "....BBBB.."
    "Y..BBBBBB."
    "YYBBKKBBEB"
    "YYBBBBKBBB"
    "Y..nnnnnn."
    "....nnn..."};
inline constexpr PixelArt minnow{6, 3,
    "c.ccc."
    "ccCCEc"
    "c.ccc."};
inline constexpr PixelArt puffer{9, 6,
    "....ssss."
    "y..yyyyyy"
    "yyyysyyEy"
    "yyyyyyyyy"
    "y..wwwww."
    "....www.."};
inline constexpr PixelArt pufferBig{13, 11,
    "....s...s...."
    "..s..yyy..s.."
    "....yyyyy...."
    ".s.yysysyy.s."
    "...yyyyyyEy.."
    "yyyyyyyyyyyys"
    "yyyyyyyyyyyy."
    ".s.wwwwwww.s."
    "....wwwww...."
    "..s..www..s.."
    "....s...s...."};
inline constexpr PixelArt crabA{12, 6,
    "rr........rr"
    "r...K..K...r"
    ".rr.r..r.rr."
    "...rrrrrr..."
    "..rrRRRRrr.."
    "..r.r..r.r.."};
inline constexpr PixelArt crabB{12, 6,
    ".rr......rr."
    ".r..K..K..r."
    "..rrr..rrr.."
    "...rrrrrr..."
    "..rrRRRRrr.."
    ".r..r..r..r."};
inline constexpr PixelArt chestClosed{12, 8,
    ".nnnnnnnnnn."
    "nNNNNNNNNNNn"
    "nNNNNNNNNNNn"
    "gggggggggggg"
    "nNNNNGGNNNNn"
    "nNNNNGGNNNNn"
    "nNNNNNNNNNNn"
    "nnnnnnnnnnnn"};
inline constexpr PixelArt chestOpen{12, 11,
    ".nnnnnnnnnn."
    "nNNNNNNNNNNn"
    "nnnnnnnnnnnn"
    "............"
    ".gGgYgGgYgG."
    "gYgGgYgGgYgg"
    "gggggggggggg"
    "nNNNNGGNNNNn"
    "nNNNNGGNNNNn"
    "nNNNNNNNNNNn"
    "nnnnnnnnnnnn"};
inline constexpr PixelArt coral{9, 8,
    "p...p...p"
    "pp.ppp.pp"
    ".p.p.p.p."
    ".ppp.ppp."
    "...p.p..."
    "p..ppp..p"
    "ppppPpppp"
    "...PPP..."};
// ---------------------------------------------------------------- Parade
inline constexpr PixelArt eagleUp{18, 13,
    "......KK..KK......"
    ".....KBBKKBbK....."
    "....KBBbKBbbK....."
    "....KBBbKBbK......"
    "K....KBbBbbK.KKK.."
    "KwK...KBBbbKKwwwK."
    "KwwKKKKBBBbbwwwEyy"
    ".KwwBBBBBBBBwwwwyK"
    "..KKBBBBBBBBKwwK.."
    "....KKKBBBKKKKK..."
    "......KyKyK......."
    ".................."
    ".................."};
inline constexpr PixelArt eagleDown{18, 13,
    ".................."
    ".................."
    ".................."
    ".................."
    "K............KKK.."
    "KwK...KKKKKKKwwwK."
    "KwwKKKBBBBBBwwwEyy"
    ".KwwBBBBBbBBwwwwyK"
    "..KKBBbBbbBBKwwK.."
    "....KBbbBbbKKKK..."
    "....KBbbKBbK......"
    ".....KBbKBbK......"
    "......KK.KK......."};
inline constexpr PixelArt truckA{26, 14,
    ".p........................"
    ".pnnrrrrr................."
    ".pnnwwwwww................"
    ".pnnrrrrr..KKKKKKK........"
    ".pwwwwwwwwKccKccccK......."
    ".prrrrrr..KccKcccccK......"
    ".p........KccKcccccK......"
    ".pKKKKKKKKKrrKrrrrrKKKKK.."
    "KrrrrrrrrrrrrKrrrrrrrrrrK."
    "KrwwwwwwwwwwwwwwwwwwwwyyK."
    "KRRKKKKRRRRRRRRRKKKKRRRyKK"
    "KRKkkkkKRRRRRRRKkkkkKRRRKK"
    ".KKkskkKKKKKKKKKkskkKKKK.."
    "...kkkk.........kkkk......"};
inline constexpr PixelArt truckB{26, 14,
    ".p........................"
    ".pnnrrrrrr................"
    ".pnnwwwww................."
    ".pnnrrrr...KKKKKKK........"
    ".pwwwwwwwwKccKccccK......."
    ".prrrrrr..KccKcccccK......"
    ".p........KccKcccccK......"
    ".pKKKKKKKKKrrKrrrrrKKKKK.."
    "KrrrrrrrrrrrrKrrrrrrrrrrK."
    "KrwwwwwwwwwwwwwwwwwwwwyyK."
    "KRRKKKKRRRRRRRRRKKKKRRRyKK"
    "KRKkkkkKRRRRRRRKkkkkKRRRKK"
    ".KKkkskKKKKKKKKKkkskKKKK.."
    "...kkkk.........kkkk......"};
inline constexpr PixelArt riderA{20, 16,
    ".......KKK.........."
    "......KhhhK........."
    "....KKKhhhKKK......."
    ".....KFFFEK........."
    "......KFFFK...KK...."
    ".....KSSSSK..KMBK..."
    "....KSSSSSSKKMBBBK.."
    "....KSSjSSFFKMBBEBK."
    ".M..KjjjjKKKMBBBBBK."
    "MMKKBBjjBBBBBBBKKK.."
    ".MKBBBjjBBBBBBK....."
    "..KBBBBBBBBBBBK....."
    "..KBbKKKKKKBBbK....."
    ".KBbK......KBbK....."
    "KbbK........KbbK...."
    "KKK..........KKK...."};
inline constexpr PixelArt riderB{20, 16,
    ".......KKK.........."
    "......KhhhK........."
    "....KKKhhhKKK......."
    ".....KFFFEK........."
    "......KFFFK...KK...."
    ".....KSSSSK..KMBK..."
    "....KSSSSSSKKMBBBK.."
    "....KSSjSSFFKMBBEBK."
    ".M..KjjjjKKKMBBBBBK."
    "MMKKBBjjBBBBBBBKKK.."
    ".MKBBBjjBBBBBBK....."
    "..KBBBBBBBBBBBK....."
    "...KBbKKKKBbK......."
    "....KbbKKbbK........"
    ".....KbK.KbK........"
    ".....KKK.KKK........"};
inline constexpr PixelArt astroA{13, 16,
    ".........p..."
    "..KKKKK..pnr."
    ".KWWWWWK.pnrr"
    "KWKKKKKWKpwww"
    "KWKvvyvKKprrr"
    "KWKvvvvKKp..."
    "KWWKKKKWWKp.."
    ".KWWWWWKKWK.."
    "..KWrWWWWK..."
    ".KWWWWsWK...."
    "KWWWWWWWK...."
    "KWKWWWWKK...."
    ".KKWWKWWK...."
    "..KWWKWWK...."
    "..KssKssK...."
    "..KKKKKKK...."};
inline constexpr PixelArt astroB{13, 16,
    ".........p..."
    "..KKKKK..prn."
    ".KWWWWWK.prrr"
    "KWKKKKKWKpwww"
    "KWKvvyvKKpnrr"
    "KWKvvvvKKp..."
    "KWWKKKKWWKp.."
    ".KWWWWWKKWK.."
    "..KWrWWWWK..."
    ".KWWWWsWK...."
    "KWWWWWWWK...."
    "KWKWWWWKK...."
    ".KKWWKWWKK..."
    ".KWWK.KWWK..."
    "KssK...KssK.."
    "KKKK...KKKK.."};
inline constexpr PixelArt hotDogA{21, 9,
    "................KKK.."
    "..KKKKKKKKKKKKKKDDDK."
    ".KUUUUUUUUUUUUKDDDEDK"
    "KDyDDyDDyDDyDDDDdDDDN"
    "DKDyDDyDDyDDyDDDdDKK."
    "K.KuuuuuuuuuuuKKdK..."
    "..KuuuuuuuuuuuK.K...."
    "..KDKKDK..KDKKDK....."
    "..KK.KK...KK.KK......"};
inline constexpr PixelArt hotDogB{21, 9,
    ".................KKK."
    "K.KKKKKKKKKKKKKKKDDDK"
    "DKUUUUUUUUUUUUKDDDDED"
    "KDyDDyDDyDDyDDDDdDDDN"
    ".KDyDDyDDyDDyDDDdDKK."
    "..KuuuuuuuuuuuKKdK..."
    "..KuuuuuuuuuuuK.K...."
    "...KDKDK..KDKDK......"
    "...KKKK...KKKK......."};
inline constexpr PixelArt ladyLiberty{15, 22,
    "............o.."
    "...........oyo."
    "............y.."
    "...........gGg."
    ".J..J..J....t.."
    "..J.J.J....tJ.."
    "..tJtJt....tt.."
    "..TtttT...tJ..."
    "...tJt....tt..."
    "...tEt...tJ...."
    "..Jttt..ttt...."
    ".JttttttJt....."
    ".tJTtttttT....."
    "gGtTtJtJtT....."
    "gGtTtJtJtT....."
    "gg.TtJtJtT....."
    "...TtJtJtT....."
    "..TtJtJtJtT...."
    "..TtJtJtJtT...."
    ".SSSSSSSSSSS..."
    "..sSSDSDSSs...."
    ".SSSSSSSSSSS..."};
inline constexpr PixelArt tinyFlag{5, 4,
    "nnrrr"
    "nnwww"
    "wwwww"
    "rrrrr"};
inline constexpr PixelArt musicNote{4, 5,
    ".yyy"
    ".y.y"
    ".y.."
    "yy.."
    "yy.."};
}
