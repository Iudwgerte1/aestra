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

#ifndef ZOBRIST_HPP
#define ZOBRIST_HPP

#include "types.hpp"

extern Key ZobristKeys[PIECE_NB][SQUARE_NB];
extern Key ZobristEnPassantKeys[FILE_NB];
extern Key ZobristCastlingKeys[CASTLING_NB];
extern Key ZobristTurnKey;

uint64_t rand64();

void initZobrist();

#endif  // ZOBRIST_HPP
