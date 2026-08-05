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

#include "zobrist.hpp"

Key ZobristKeys[PIECE_NB][SQUARE_NB];
Key ZobristEnPassantKeys[FILE_NB];
Key ZobristCastlingKeys[CASTLING_NB];
Key ZobristTurnKey;

void initZobrist() {
    PRNG prng(0x3FF6A09E667F3BCDull);

    for (PieceType pt = PAWN; pt <= KING; ++pt)
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
            ZobristKeys[makePiece(WHITE, pt)][sq] = prng.rand64();
            ZobristKeys[makePiece(BLACK, pt)][sq] = prng.rand64();
        }

    for (File f = FILE_A; f < FILE_NB; ++f) ZobristEnPassantKeys[f] = prng.rand64();

    for (int cr = 0; cr < CASTLING_NB; ++cr) ZobristCastlingKeys[cr] = prng.rand64();

    ZobristTurnKey = prng.rand64();
}
