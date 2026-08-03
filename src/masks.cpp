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

#include "masks.hpp"

#include "attacks.hpp"
#include "bitboards.hpp"

int DistanceBetween[SQUARE_NB][SQUARE_NB];
Bitboard BitsBetween[SQUARE_NB][SQUARE_NB];

void initMasks() {
    for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
        for (Square sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
            DistanceBetween[sq1][sq2] =
                std::max(std::abs(int(fileOf(sq1)) - int(fileOf(sq2))), std::abs(int(rankOf(sq1)) - int(rankOf(sq2))));

    for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
        for (Square sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
            if (bishopAttacks(sq1, 0ull) & sq2)
                BitsBetween[sq1][sq2] = bishopAttacks(sq1, squareBB(sq2)) & bishopAttacks(sq2, squareBB(sq1));

    for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
        for (Square sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
            if (rookAttacks(sq1, 0ull) & sq2)
                BitsBetween[sq1][sq2] = rookAttacks(sq1, squareBB(sq2)) & rookAttacks(sq2, squareBB(sq1));
}

int distanceBetween(Square sq1, Square sq2) { return DistanceBetween[sq1][sq2]; }
Bitboard bitsBetween(Square sq1, Square sq2) { return BitsBetween[sq1][sq2]; }
