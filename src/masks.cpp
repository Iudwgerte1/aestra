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
Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard AdjacentFiles[SQUARE_NB];
Bitboard PassedPawnMask[COLOR_NB][SQUARE_NB];
Bitboard ConnectedPawnMask[COLOR_NB][SQUARE_NB];

void initMasks() {
    for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
        for (Square sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2)
            DistanceBetween[sq1][sq2] =
                std::max(std::abs(int(fileOf(sq1)) - int(fileOf(sq2))), std::abs(int(rankOf(sq1)) - int(rankOf(sq2))));

    for (Square sq1 = SQ_A1; sq1 <= SQ_H8; ++sq1)
        for (Square sq2 = SQ_A1; sq2 <= SQ_H8; ++sq2) {
            if (bishopAttacks(sq1, 0ull) & sq2)
                BitsBetween[sq1][sq2] = bishopAttacks(sq1, squareBB(sq2)) & bishopAttacks(sq2, squareBB(sq1));
            if (rookAttacks(sq1, 0ull) & sq2)
                BitsBetween[sq1][sq2] = rookAttacks(sq1, squareBB(sq2)) & rookAttacks(sq2, squareBB(sq1));
        }

    const int LineDirs[8][2] = {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};

    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        for (int d = 0; d < 8; ++d) {
            const int df = LineDirs[d][0], dr = LineDirs[d][1];

            Bitboard fwd = 0;
            for (int f = int(fileOf(sq)) + df, r = int(rankOf(sq)) + dr; f >= 0 && f <= 7 && r >= 0 && r <= 7;
                 f += df, r += dr)
                fwd |= makeSquare(File(f), Rank(r));

            Bitboard bwd = 0;
            for (int f = int(fileOf(sq)) - df, r = int(rankOf(sq)) - dr; f >= 0 && f <= 7 && r >= 0 && r <= 7;
                 f -= df, r -= dr)
                bwd |= makeSquare(File(f), Rank(r));

            const Bitboard line = fwd | bwd;
            Bitboard copy = fwd;
            while (copy) LineBB[sq][popLsb(copy)] = line;
        }

    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
        File f = fileOf(sq);
        if (f > FILE_A) AdjacentFiles[sq] |= FilesBB[f - 1];
        if (f < FILE_H) AdjacentFiles[sq] |= FilesBB[f + 1];
    }

    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
        Bitboard whiteMask = 0, blackMask = 0;
        Bitboard files = FilesBB[fileOf(sq)] | AdjacentFiles[sq];
        for (int r = 0; r < rankOf(sq); ++r) blackMask |= RanksBB[r];
        for (int r = rankOf(sq) + 1; r <= 7; ++r) whiteMask |= RanksBB[r];
        PassedPawnMask[WHITE][sq] = whiteMask & files;
        PassedPawnMask[BLACK][sq] = blackMask & files;
    }

    for (Square sq = SQ_A2; sq <= SQ_H7; ++sq) {
        ConnectedPawnMask[WHITE][sq] = pawnAttacks(BLACK, sq) | pawnAttacks(BLACK, sq + NORTH);
        ConnectedPawnMask[BLACK][sq] = pawnAttacks(WHITE, sq) | pawnAttacks(WHITE, sq + SOUTH);
    }
}

int distanceBetween(Square sq1, Square sq2) { return DistanceBetween[sq1][sq2]; }
Bitboard bitsBetween(Square sq1, Square sq2) { return BitsBetween[sq1][sq2]; }
Bitboard lineBB(Square sq1, Square sq2) { return LineBB[sq1][sq2]; }
Bitboard adjacentFiles(Square sq) { return AdjacentFiles[sq]; }
Bitboard passedPawnMask(Color c, Square sq) { return PassedPawnMask[c][sq]; }
Bitboard connectedPawnMask(Color c, Square sq) { return ConnectedPawnMask[c][sq]; }
