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

#include "evaluate.hpp"
Value evaluate(const Board& b) {
    Score score = b.psqtScore();
    score += TEMPO[b.turn()];

    int mg = mgValue(score), eg = egValue(score);

    int phase = 24 - popcount(b.pieces(QUEEN)) * 4
                   - popcount(b.pieces(ROOK)) * 2
                   - popcount(b.pieces(KNIGHT, BISHOP));
    phase = (phase * 256 + 12) / 24;

    int eval = (mg * (256 - phase) + eg * phase) / 256;
    eval = b.turn() == WHITE ? eval : -eval;

    return Value(eval);
}
