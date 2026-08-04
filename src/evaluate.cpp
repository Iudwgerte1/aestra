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

#include "attacks.hpp"
#include "masks.hpp"

#define S(mg, eg) makeScore(Value(mg), Value(eg))

constexpr Score BISHOP_PAIR = S(26, 81);
constexpr Score PASSED_PAWNS[8] = {SCORE_ZERO, S(-16, -5), S(-23, 3), S(-21, 32),
                                   S(7, 66),   S(-3, 152), S(21, 95), SCORE_ZERO};
constexpr Score BISHOP_MOBILITY[14] = {S(-20, -10), S(-66, -83), S(-48, -74), S(-35, -37), S(-19, -23),
                                       S(-12, -10), S(3, 11),    S(13, 19),   S(22, 32),   S(24, 37),
                                       S(30, 44),   S(32, 40),   S(35, 41),   S(64, 31)};
constexpr Score ROOK_MOBILITY[15] = {S(-25, -12), S(-12, -5), S(-37, -47), S(-28, -24), S(-19, -13),
                                     S(-13, -8),  S(-9, -1),  S(-5, 7),    S(2, 11),    S(11, 14),
                                     S(20, 19),   S(28, 22),  S(34, 27),   S(46, 30),   S(53, 27)};
constexpr Score QUEEN_MOBILITY[28] = {S(-30, -15),  S(-15, -6),  S(-6, 0),    S(-54, -17), S(-62, -181), S(-17, -163),
                                      S(-17, -133), S(-13, -80), S(-10, -54), S(-8, -31),  S(-3, -23),   S(0, -6),
                                      S(3, 9),      S(6, 15),    S(10, 25),   S(11, 37),   S(12, 45),    S(15, 55),
                                      S(16, 63),    S(18, 69),   S(20, 82),   S(25, 79),   S(26, 86),    S(37, 81),
                                      S(42, 78),    S(59, 75),   S(105, 55),  S(172, 23)};
constexpr Score TEMPO[COLOR_NB] = {S(25, 29), S(-25, -29)};

#undef S

Score evalPawns(const Board& b, Color c) {
    Bitboard tempPawns = b.pieces(c, PAWN);

    Score pawnScore = SCORE_ZERO;

    while (tempPawns) {
        Square s = popLsb(tempPawns);

        Bitboard stoppers = b.pieces(~c, PAWN) & passedPawnMask(c, s);

        if (!stoppers) pawnScore += PASSED_PAWNS[rankOf(s) ^ (c == WHITE ? 0 : 7)];
    }

    return pawnScore;
}

Score evalBishops(const Board& b, Color c) {
    Bitboard tempBishops = b.pieces(c, BISHOP);
    Bitboard occ = b.pieces();

    Score bishopScore = SCORE_ZERO;

    if (popcount(tempBishops) >= 2) bishopScore += BISHOP_PAIR;

    while (tempBishops) {
        Square s = popLsb(tempBishops);

        bishopScore += BISHOP_MOBILITY[popcount(bishopAttacks(s, occ))];
    }

    return bishopScore;
}

Score evalRooks(const Board& b, Color c) {
    Bitboard tempRooks = b.pieces(c, ROOK);
    Bitboard occ = b.pieces();

    Score rookScore = SCORE_ZERO;

    while (tempRooks) {
        Square s = popLsb(tempRooks);

        rookScore += ROOK_MOBILITY[popcount(rookAttacks(s, occ))];
    }

    return rookScore;
}

Score evalQueens(const Board& b, Color c) {
    Bitboard tempQueens = b.pieces(c, QUEEN);
    Bitboard occ = b.pieces();

    Score queenScore = SCORE_ZERO;

    while (tempQueens) {
        Square s = popLsb(tempQueens);

        queenScore += QUEEN_MOBILITY[popcount(queenAttacks(s, occ))];
    }

    return queenScore;
}

Value evaluate(const Board& b) {
    Score score = b.psqtScore();

    score += evalPawns(b, WHITE) - evalPawns(b, BLACK);
    score += evalBishops(b, WHITE) - evalBishops(b, BLACK);
    score += evalRooks(b, WHITE) - evalRooks(b, BLACK);
    score += evalQueens(b, WHITE) - evalQueens(b, BLACK);

    score += TEMPO[b.turn()];

    int mg = mgValue(score), eg = egValue(score);

    int phase = 24 - popcount(b.pieces(QUEEN)) * 4 - popcount(b.pieces(ROOK)) * 2 - popcount(b.pieces(KNIGHT, BISHOP));
    phase = (phase * 256 + 12) / 24;

    int eval = (mg * (256 - phase) + eg * phase) / 256;
    eval = b.turn() == WHITE ? eval : -eval;

    return Value(eval);
}
