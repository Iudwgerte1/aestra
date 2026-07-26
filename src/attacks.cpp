/*
    Aestra is a UCI-compliant chess engine written in C++.
    Copyright (C) 2026  Iudwgerte1 <a09701070@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifdef USE_PEXT
#include <immintrin.h>
#endif

#include "attacks.hpp"
#include "bitboards.hpp"
#include "types.hpp"

alignas(64) Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
alignas(64) Bitboard KnightAttacks[SQUARE_NB];
alignas(64) Bitboard BishopAttacks[0x1480];
alignas(64) Bitboard RookAttacks[0x19000];
alignas(64) Bitboard KingAttacks[SQUARE_NB];

alignas(64) Magic BishopTable[SQUARE_NB];
alignas(64) Magic RookTable[SQUARE_NB];

constexpr int sliderIndex(Bitboard occ, const Magic& table) {
#ifdef USE_PEXT
    return _pext_u64(occ, table.mask);
#else
    return ((occ & table.mask) * table.magic) >> table.shift;
#endif
}

bool validCoord(File f, Rank r) { return f <= FILE_H && r <= RANK_8; }
void setSq(Bitboard& b, File f, Rank r) {
    if (validCoord(f, r)) b |= makeSquare(f, r);
}

Bitboard sliderAttacks(Square s, Bitboard occ, const int delta[4][2]) {
    File f;
    Rank r;
    Bitboard res = 0;

    for (int i = 0; i < 4; i++) {
        int df = delta[i][0];
        int dr = delta[i][1];
        for (f = File(fileOf(s) + df), r = Rank(rankOf(s) + dr); validCoord(f, r); f = File(f + df), r = Rank(r + dr)) {
            res |= makeSquare(f, r);
            if (occ & makeSquare(f, r)) break;
        }
    }

    return res;
}

void initSliderAttacks(Square s, Magic* table, Key magic, const int delta[4][2]) {
    Bitboard edges = ((RANK_1BB | RANK_8BB) & ~RanksBB[rankOf(s)]) | ((FILE_ABB | FILE_HBB) & ~FilesBB[fileOf(s)]);
    Bitboard occ = 0ull;

    table[s].magic = magic;
    table[s].mask = sliderAttacks(s, 0, delta) & ~edges;
    table[s].shift = 64 - popcount(table[s].mask);

    if (s != SQ_H8) table[s + 1].offset = table[s].offset + (1 << popcount(table[s].mask));

    do {
        int idx = sliderIndex(occ, table[s]);
        table[s].offset[idx] = sliderAttacks(s, occ, delta);
        occ = (occ - table[s].mask) & table[s].mask;
    } while (occ);
}
void initAttacks() {
    const int PawnDelta[2][2] = {{-1, 1}, {1, 1}};
    const int KnightDelta[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
    const int KingDelta[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
    const int BishopDelta[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    const int RookDelta[4][2] = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}};

    BishopTable[0].offset = BishopAttacks;
    RookTable[0].offset = RookAttacks;

    for (Square s = SQ_A1; s <= SQ_H8; ++s)
        for (int dir = 0; dir < 2; ++dir) {
            setSq(PawnAttacks[WHITE][s], File(fileOf(s) + PawnDelta[dir][0]), Rank(rankOf(s) + PawnDelta[dir][1]));
            setSq(PawnAttacks[BLACK][s], File(fileOf(s) - PawnDelta[dir][0]), Rank(rankOf(s) - PawnDelta[dir][1]));
        }

    for (Square s = SQ_A1; s <= SQ_H8; ++s)
        for (int dir = 0; dir < 8; ++dir) {
            setSq(KnightAttacks[s], File(fileOf(s) + KnightDelta[dir][0]), Rank(rankOf(s) + KnightDelta[dir][1]));
            setSq(KingAttacks[s], File(fileOf(s) + KingDelta[dir][0]), Rank(rankOf(s) + KingDelta[dir][1]));
        }

    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        initSliderAttacks(s, BishopTable, BishopMagics[s], BishopDelta);
        initSliderAttacks(s, RookTable, RookMagics[s], RookDelta);
    }
}

Bitboard pawnAttacks(Color c, Square s) { return PawnAttacks[c][s]; }
Bitboard knightAttacks(Square s) { return KnightAttacks[s]; }
Bitboard bishopAttacks(Square s, Bitboard occ) { return BishopTable[s].offset[sliderIndex(occ, BishopTable[s])]; }
Bitboard rookAttacks(Square s, Bitboard occ) { return RookTable[s].offset[sliderIndex(occ, RookTable[s])]; }
Bitboard queenAttacks(Square s, Bitboard occ) { return bishopAttacks(s, occ) | rookAttacks(s, occ); }
Bitboard kingAttacks(Square s) { return KingAttacks[s]; }

Bitboard pawnLeftAttacks(Bitboard pawns, Bitboard targets, Color c) {
    return targets & (c == WHITE ? (pawns << 7) & ~FILE_HBB : (pawns >> 7) & ~FILE_ABB);
}
Bitboard pawnRightAttacks(Bitboard pawns, Bitboard targets, Color c) {
    return targets & (c == WHITE ? (pawns << 9) & ~FILE_ABB : (pawns >> 9) & ~FILE_HBB);
}
Bitboard pawnAdvance(Bitboard pawns, Bitboard occ, Color c) {
    return ~occ & (c == WHITE ? (pawns << 8) : (pawns >> 8));
}
Bitboard pawnEnPassantAttacks(Bitboard pawns, Square epSq, Color c) {
    return epSq != SQ_NONE ? pawnAttacks(~c, epSq) & pawns : 0;
}
