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
#include "nnue.hpp"

bool UseNNUE = true;

#define S(mg, eg) makeScore(Value(mg), Value(eg))

constexpr Score BISHOP_PAIR = S(17, 74);
constexpr Score PASSED_PAWNS[8] = {SCORE_ZERO, S(-13, 15), S(-12, 20), S(-6, 42),
                                   S(21, 63),  S(34, 118), S(89, 38),  SCORE_ZERO};
constexpr Score BISHOP_MOBILITY[14] = {S(-20, -10), S(-29, -92), S(-49, -92), S(-46, -54), S(-39, -35),
                                       S(-36, -20), S(-29, -1),  S(-21, 9),   S(-18, 21),  S(-18, 27),
                                       S(-15, 31),  S(-10, 27),  S(-10, 23),  S(32, 11)};
constexpr Score ROOK_MOBILITY[15] = {S(-25, -12), S(-12, -5),  S(-29, -76), S(-24, -54), S(-21, -32),
                                     S(-19, -27), S(-18, -18), S(-17, -13), S(-14, -9),  S(-10, -5),
                                     S(-7, 0),    S(-4, 2),    S(-1, 4),    S(4, 3),     S(19, -9)};
constexpr Score QUEEN_MOBILITY[28] = {S(-30, -15),  S(-15, -6),   S(-6, 0),    S(-41, -18), S(-41, -204), S(-26, -186),
                                      S(-20, -152), S(-20, -104), S(-18, -72), S(-18, -52), S(-16, -37),  S(-15, -23),
                                      S(-13, -13),  S(-10, -6),   S(-7, 0),    S(-5, 8),    S(-2, 14),    S(0, 17),
                                      S(0, 22),     S(3, 23),     S(2, 29),    S(3, 26),    S(1, 33),     S(7, 25),
                                      S(5, 26),     S(13, 17),    S(18, 7),    S(82, -17)};
constexpr Score PAWN_DOUBLED = S(-11, -19);
constexpr Score PAWN_ISOLATED = S(-4, -18);
constexpr Score CONNECTED_PAWN[SQUARE_NB] = {
    S(0, 0),    S(0, 0),    S(0, 0),    S(0, 0),    S(0, 0),    S(0, 0),    S(0, 0),   S(0, 0),   S(5, -6),  S(1, 7),
    S(7, 6),    S(2, 16),   S(17, 25),  S(-3, -12), S(-2, 18),  S(-6, -23), S(8, 6),   S(14, 10), S(21, 18), S(22, 15),
    S(25, 21),  S(20, 11),  S(19, 6),   S(19, 3),   S(4, 5),    S(14, 13),  S(15, 16), S(19, 28), S(19, 23), S(14, 12),
    S(24, 6),   S(24, 1),   S(-1, 15),  S(10, 22),  S(18, 31),  S(21, 26),  S(30, 28), S(28, 29), S(14, 21), S(17, 9),
    S(19, 26),  S(12, 52),  S(38, 61),  S(63, 71),  S(67, 64),  S(62, 58),  S(44, 33), S(13, 28), S(57, 65), S(119, 75),
    S(104, 87), S(134, 73), S(141, 71), S(87, 77),  S(110, 76), S(45, 57),  S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),
    S(0, 0),    S(0, 0),    S(0, 0),    S(0, 0)};
constexpr Score ROOK_OPEN_FILE = S(32, 12);
constexpr Score ROOK_SEMI_OPEN_FILE = S(16, 14);
constexpr Score ROOK_SEVENTH = S(-12, 13);
constexpr Score TEMPO[COLOR_NB] = {S(10, 13), S(-10, -13)};

#undef S

Score evalPawns(const Board& b, Color c) {
    Bitboard ourPawns = b.pieces(c, PAWN);
    Bitboard tempPawns = b.pieces(c, PAWN);

    Score pawnScore = SCORE_ZERO;

    while (tempPawns) {
        Square s = popLsb(tempPawns);

        Bitboard stoppers = b.pieces(~c, PAWN) & passedPawnMask(c, s);

        if (!stoppers) pawnScore += PASSED_PAWNS[rankOf(s) ^ (c == WHITE ? 0 : 7)];

        if (connectedPawnMask(c, s) & ourPawns) pawnScore += CONNECTED_PAWN[int(s) ^ (c == WHITE ? 0 : 56)];
        if ((adjacentFiles(s) & (ourPawns ^ s)) == 0) pawnScore += PAWN_ISOLATED;
        if (FilesBB[fileOf(s)] & (ourPawns ^ s)) pawnScore += PAWN_DOUBLED;
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

        Bitboard ourPawns = b.pieces(c, PAWN) & FilesBB[fileOf(s)];
        Bitboard enemyPawns = b.pieces(~c, PAWN) & FilesBB[fileOf(s)];
        if (!ourPawns && !enemyPawns)
            rookScore += ROOK_OPEN_FILE;
        else if (!ourPawns && enemyPawns)
            rookScore += ROOK_SEMI_OPEN_FILE;

        if (rankOf(s) == (c == WHITE ? RANK_7 : RANK_2)) rookScore += ROOK_SEVENTH;
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
    if (UseNNUE) return nnueParams.evaluate(b.getAccumulators(b.turn()), b.getAccumulators(~b.turn()));

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
