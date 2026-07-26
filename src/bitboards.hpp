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

#ifndef BITBOARDS_HPP
#define BITBOARDS_HPP

#include <string>

#include "types.hpp"

constexpr Bitboard FILE_ABB = 0x0101010101010101ull;
constexpr Bitboard FILE_BBB = FILE_ABB << 1;
constexpr Bitboard FILE_CBB = FILE_ABB << 2;
constexpr Bitboard FILE_DBB = FILE_ABB << 3;
constexpr Bitboard FILE_EBB = FILE_ABB << 4;
constexpr Bitboard FILE_FBB = FILE_ABB << 5;
constexpr Bitboard FILE_GBB = FILE_ABB << 6;
constexpr Bitboard FILE_HBB = FILE_ABB << 7;

constexpr Bitboard RANK_1BB = 0xFFull;
constexpr Bitboard RANK_2BB = RANK_1BB << (1 * 8);
constexpr Bitboard RANK_3BB = RANK_1BB << (2 * 8);
constexpr Bitboard RANK_4BB = RANK_1BB << (3 * 8);
constexpr Bitboard RANK_5BB = RANK_1BB << (4 * 8);
constexpr Bitboard RANK_6BB = RANK_1BB << (5 * 8);
constexpr Bitboard RANK_7BB = RANK_1BB << (6 * 8);
constexpr Bitboard RANK_8BB = RANK_1BB << (7 * 8);

constexpr Bitboard FilesBB[FILE_NB] = {FILE_ABB, FILE_BBB, FILE_CBB, FILE_DBB, FILE_EBB, FILE_FBB, FILE_GBB, FILE_HBB};
constexpr Bitboard RanksBB[RANK_NB] = {RANK_1BB, RANK_2BB, RANK_3BB, RANK_4BB, RANK_5BB, RANK_6BB, RANK_7BB, RANK_8BB};

constexpr Bitboard WHITE_SQUARES = 0x55AA55AA55AA55AAull;
constexpr Bitboard BLACK_SQUARES = 0xAA55AA55AA55AA55ull;

constexpr Bitboard QUEEN_WING = FILE_ABB | FILE_BBB | FILE_CBB | FILE_DBB;
constexpr Bitboard KING_WING = FILE_EBB | FILE_FBB | FILE_GBB | FILE_HBB;

constexpr Bitboard PROMOTION_RANKS = RANK_1BB | RANK_8BB;

constexpr Bitboard squareBB(Square s) { return 1ull << s; }

constexpr Bitboard operator&(Bitboard b, Square s) { return b & squareBB(s); }
constexpr Bitboard operator|(Bitboard b, Square s) { return b | squareBB(s); }
constexpr Bitboard operator^(Bitboard b, Square s) { return b ^ squareBB(s); }
constexpr Bitboard& operator&=(Bitboard& b, Square s) { return b &= squareBB(s); }
constexpr Bitboard& operator|=(Bitboard& b, Square s) { return b |= squareBB(s); }
constexpr Bitboard& operator^=(Bitboard& b, Square s) { return b ^= squareBB(s); }

constexpr Bitboard operator&(Square s, Bitboard b) { return b & squareBB(s); }
constexpr Bitboard operator|(Square s, Bitboard b) { return b | squareBB(s); }
constexpr Bitboard operator^(Square s, Bitboard b) { return b ^ squareBB(s); }

constexpr Bitboard operator|(Square s1, Square s2) { return squareBB(s1) | squareBB(s2); }

inline int popcount(Bitboard b) { return __builtin_popcountll(b); }
inline Square lsb(Bitboard b) { return Square(__builtin_ctzll(b)); }
inline Square msb(Bitboard b) { return Square(63 ^ __builtin_clzll(b)); }
inline Square popLsb(Bitboard& b) {
    const Square s = lsb(b);
    b &= b - 1;
    return s;
}

inline bool several(Bitboard b) { return b & (b - 1); }

std::string prettyBitboard(Bitboard b);

#endif  // BITBOARDS_HPP
