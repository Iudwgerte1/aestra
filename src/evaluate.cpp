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

constexpr Score BISHOP_PAIR = S(16, 74);
constexpr Score PASSED_PAWNS[8] = {SCORE_ZERO, S(-9, 18),  S(-8, 25), S(-4, 47),
                                   S(20, 68),  S(33, 123), S(75, 51), SCORE_ZERO};
constexpr Score BISHOP_MOBILITY[14] = {S(-20, -10), S(-14, -79), S(-32, -87), S(-29, -49), S(-22, -29),
                                       S(-20, -15), S(-12, 4),   S(-5, 15),   S(-2, 26),   S(-1, 33),
                                       S(1, 37),    S(6, 33),    S(7, 29),    S(48, 17)};
constexpr Score ROOK_MOBILITY[15] = {S(-25, -12), S(-12, -5),  S(-24, -66), S(-18, -44), S(-16, -23),
                                     S(-13, -18), S(-12, -10), S(-11, -4),  S(-8, 0),    S(-5, 4),
                                     S(-2, 9),    S(2, 11),    S(4, 13),    S(9, 12),    S(24, 0)};
constexpr Score QUEEN_MOBILITY[28] = {S(-30, -15),  S(-15, -6),  S(-6, 0),    S(-49, -17), S(-40, -183), S(-21, -169),
                                      S(-15, -137), S(-14, -91), S(-13, -58), S(-12, -39), S(-11, -23),  S(-10, -10),
                                      S(-7, 0),     S(-5, 8),    S(-2, 14),   S(1, 21),    S(3, 27),     S(6, 30),
                                      S(6, 35),     S(8, 36),    S(7, 42),    S(9, 40),    S(7, 47),     S(12, 38),
                                      S(11, 39),    S(20, 30),   S(47, 4),    S(125, -25)};
constexpr Score PAWN_DOUBLED = S(-25, -18);
constexpr Score PAWN_ISOLATED = S(-6, -19);
constexpr Score CONNECTED_PAWN[SQUARE_NB] = {
    S(0, 0),    S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),    S(0, 0),   S(0, 0),   S(5, -1),  S(1, 7),
    S(8, 12),   S(2, 18),  S(18, 28), S(-3, -7), S(-1, 19), S(-5, -18), S(9, 11),  S(16, 15), S(23, 22), S(23, 19),
    S(26, 25),  S(23, 15), S(22, 11), S(22, 8),  S(6, 9),   S(17, 17),  S(16, 19), S(20, 31), S(20, 27), S(16, 15),
    S(30, 11),  S(25, 5),  S(1, 18),  S(13, 25), S(22, 33), S(23, 28),  S(33, 31), S(31, 32), S(18, 26), S(19, 13),
    S(20, 29),  S(16, 55), S(41, 62), S(67, 73), S(72, 66), S(66, 61),  S(50, 37), S(16, 32), S(47, 72), S(75, 93),
    S(72, 100), S(76, 94), S(78, 93), S(58, 86), S(67, 91), S(42, 67),  S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),
    S(0, 0),    S(0, 0),   S(0, 0),   S(0, 0)};
constexpr Score ROOK_OPEN_FILE = S(31, 10);
constexpr Score ROOK_SEMI_OPEN_FILE = S(12, 10);
constexpr Score ROOK_SEVENTH = S(2, 30);
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
        if (RanksBB[rankOf(s)] & ourPawns) pawnScore += PAWN_DOUBLED;
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
        if (!ourPawns && !enemyPawns) rookScore += ROOK_OPEN_FILE;
        else if (!ourPawns && enemyPawns) rookScore += ROOK_SEMI_OPEN_FILE;

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
