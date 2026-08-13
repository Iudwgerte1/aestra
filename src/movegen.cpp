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

#include "movegen.hpp"

#include "attacks.hpp"
#include "bitboards.hpp"
#include "board.hpp"
#include "masks.hpp"
#include "types.hpp"

static inline Bitboard pinMask(Square kSq, Bitboard pinned, Square from) {
    return pinned & from ? lineBB(kSq, from) : ~Bitboard(0);
}

static bool attackedBy(const Board& board, Square s, Color them, Square kSq) {
    Bitboard occ = board.pieces() ^ squareBB(kSq);
    return (pawnAttacks(~them, s) & board.pieces(them, PAWN)) || (knightAttacks(s) & board.pieces(them, KNIGHT)) ||
           (kingAttacks(s) & board.pieces(them, KING)) ||
           (bishopAttacks(s, occ) & (board.pieces(them, BISHOP) | board.pieces(them, QUEEN))) ||
           (rookAttacks(s, occ) & (board.pieces(them, ROOK) | board.pieces(them, QUEEN)));
}

static void buildKingMoves(const Board& board, MoveList& moves, Square kSq, Color them, Bitboard targets) {
    Bitboard attacks = kingAttacks(kSq) & targets;

    while (attacks) {
        Square to = popLsb(attacks);
        if (!attackedBy(board, to, them, kSq)) moves.add(makeMove(kSq, to));
    }
}

static void buildPawnPromotions(MoveList& moves, Square from, Square to) {
    moves.add(makeMove(from, to, KNIGHT));
    moves.add(makeMove(from, to, BISHOP));
    moves.add(makeMove(from, to, ROOK));
    moves.add(makeMove(from, to, QUEEN));
}

void genQuietMoves(const Board& board, MoveList& moves) {
    const Direction Fwd = board.turn() == WHITE ? NORTH : SOUTH;
    const Bitboard Rank3Relative = board.turn() == WHITE ? RANK_3BB : RANK_6BB;

    Color us = board.turn();
    Color them = ~us;
    Bitboard usBB = board.pieces(us);
    Bitboard occupied = board.pieces();
    Bitboard checkers = board.kingAttackers();
    Square kSq = lsb(usBB & board.pieces(KING));
    Bitboard pinned = board.pinned(us);

    if (several(checkers)) {
        buildKingMoves(board, moves, kSq, them, ~occupied);
        return;
    }

    Bitboard destinations = !checkers ? ~occupied : bitsBetween(kSq, lsb(checkers));

    Bitboard pawns = usBB & board.pieces(PAWN);
    Bitboard knights = usBB & board.pieces(KNIGHT);
    Bitboard bishops = usBB & (board.pieces(BISHOP) | board.pieces(QUEEN));
    Bitboard rooks = usBB & (board.pieces(ROOK) | board.pieces(QUEEN));

    Bitboard loop;

    loop = pawns;
    while (loop) {
        Square from = popLsb(loop);
        Square to = from + Fwd;
        if (!(occupied & to) && !(PROMOTION_RANKS & to) && (pinMask(kSq, pinned, from) & to) && (destinations & to))
            moves.add(makeMove(from, to));
    }

    loop = pawns;
    while (loop) {
        Square from = popLsb(loop);
        Square one = from + Fwd;
        Square two = from + 2 * Fwd;
        if ((Rank3Relative & one) && !(occupied & one) && !(occupied & two) && (pinMask(kSq, pinned, from) & two) &&
            (destinations & two))
            moves.add(makeMove(from, two));
    }

    loop = knights;
    while (loop) {
        Square from = popLsb(loop);
        Bitboard attacks = knightAttacks(from) & destinations & pinMask(kSq, pinned, from);
        while (attacks) moves.add(makeMove(from, popLsb(attacks)));
    }

    loop = bishops;
    while (loop) {
        Square from = popLsb(loop);
        Bitboard attacks = bishopAttacks(from, occupied) & destinations & pinMask(kSq, pinned, from);
        while (attacks) moves.add(makeMove(from, popLsb(attacks)));
    }

    loop = rooks;
    while (loop) {
        Square from = popLsb(loop);
        Bitboard attacks = rookAttacks(from, occupied) & destinations & pinMask(kSq, pinned, from);
        while (attacks) moves.add(makeMove(from, popLsb(attacks)));
    }

    buildKingMoves(board, moves, kSq, them, ~occupied);

    if (!checkers) {
        Square kingSq = us == WHITE ? SQ_E1 : SQ_E8;
        if (board.canCastle(us == WHITE ? WHITE_OO : BLACK_OO)) {
            Square kTo = makeSquare(FILE_G, rankOf(kingSq));
            Square fSq = makeSquare(FILE_F, rankOf(kingSq));
            if (!(occupied & (kTo | fSq)) && !board.attackersTo(fSq, them) && !board.attackersTo(kTo, them))
                moves.add(makeMove(kingSq, kTo, CASTLING));
        }
        if (board.canCastle(us == WHITE ? WHITE_OOO : BLACK_OOO)) {
            Square kTo = makeSquare(FILE_C, rankOf(kingSq));
            Square bSq = makeSquare(FILE_B, rankOf(kingSq));
            Square dSq = makeSquare(FILE_D, rankOf(kingSq));
            if (!(occupied & (kTo | dSq | bSq)) && !board.attackersTo(dSq, them) && !board.attackersTo(kTo, them))
                moves.add(makeMove(kingSq, kTo, CASTLING));
        }
    }
}

void genNoisyMoves(const Board& board, MoveList& moves) {
    const Direction Fwd = board.turn() == WHITE ? NORTH : SOUTH;
    const Direction LeftAtt = board.turn() == WHITE ? NORTH_WEST : SOUTH_EAST;
    const Direction RightAtt = board.turn() == WHITE ? NORTH_EAST : SOUTH_WEST;

    Color us = board.turn();
    Color them = ~us;
    Bitboard usBB = board.pieces(us);
    Bitboard themBB = board.pieces(them);
    Bitboard occupied = usBB | themBB;
    Bitboard checkers = board.kingAttackers();
    Square kSq = lsb(usBB & board.pieces(KING));
    Bitboard pinned = board.pinned(us);

    if (several(checkers)) {
        buildKingMoves(board, moves, kSq, them, themBB);
        return;
    }

    Bitboard destinations = checkers ? checkers : themBB;
    Bitboard quietDestinations = !checkers ? ~occupied : bitsBetween(kSq, lsb(checkers));

    Bitboard pawns = usBB & board.pieces(PAWN);
    Bitboard knights = usBB & board.pieces(KNIGHT);
    Bitboard bishops = usBB & (board.pieces(BISHOP) | board.pieces(QUEEN));
    Bitboard rooks = usBB & (board.pieces(ROOK) | board.pieces(QUEEN));

    Square epSq = board.epSquare();
    Bitboard enPassantPawns = epSq != SQ_NONE ? pawnAttacks(~us, epSq) & pawns : Bitboard(0);

    Bitboard loop = enPassantPawns;
    while (loop) {
        Square from = popLsb(loop);
        Square captured = makeSquare(fileOf(epSq), rankOf(from));

        if (checkers && !(checkers == squareBB(captured))) continue;
        if (!(pinMask(kSq, pinned, from) & epSq)) continue;

        Bitboard finalOcc = (occupied ^ squareBB(from) ^ squareBB(captured)) | squareBB(epSq);
        Bitboard themRooksQueens = themBB & (board.pieces(ROOK) | board.pieces(QUEEN));
        Bitboard themBishopsQueens = themBB & (board.pieces(BISHOP) | board.pieces(QUEEN));
        if ((rookAttacks(kSq, finalOcc) & themRooksQueens) || (bishopAttacks(kSq, finalOcc) & themBishopsQueens))
            continue;

        moves.add(makeMove(from, epSq, EN_PASSANT));
    }

    loop = pawns;
    while (loop) {
        Square from = popLsb(loop);
        if ((us == WHITE && fileOf(from) > FILE_A) || (us == BLACK && fileOf(from) < FILE_H)) {
            Square to = from + LeftAtt;
            if ((themBB & to) && !(PROMOTION_RANKS & to) && (pinMask(kSq, pinned, from) & to) && (destinations & to))
                moves.add(makeMove(from, to));
        }
    }

    loop = pawns;
    while (loop) {
        Square from = popLsb(loop);
        if ((us == WHITE && fileOf(from) < FILE_H) || (us == BLACK && fileOf(from) > FILE_A)) {
            Square to = from + RightAtt;
            if ((themBB & to) && !(PROMOTION_RANKS & to) && (pinMask(kSq, pinned, from) & to) && (destinations & to))
                moves.add(makeMove(from, to));
        }
    }

    loop = pawns;
    while (loop) {
        Square from = popLsb(loop);
        Square to = from + Fwd;
        if (!(occupied & to) && (PROMOTION_RANKS & to) && (pinMask(kSq, pinned, from) & to) && (quietDestinations & to))
            buildPawnPromotions(moves, from, to);
    }

    loop = pawns;
    while (loop) {
        Square from = popLsb(loop);
        if ((us == WHITE && fileOf(from) > FILE_A) || (us == BLACK && fileOf(from) < FILE_H)) {
            Square to = from + LeftAtt;
            if ((themBB & to) && (PROMOTION_RANKS & to) && (pinMask(kSq, pinned, from) & to) && (destinations & to))
                buildPawnPromotions(moves, from, to);
        }
    }

    loop = pawns;
    while (loop) {
        Square from = popLsb(loop);
        if ((us == WHITE && fileOf(from) < FILE_H) || (us == BLACK && fileOf(from) > FILE_A)) {
            Square to = from + RightAtt;
            if ((themBB & to) && (PROMOTION_RANKS & to) && (pinMask(kSq, pinned, from) & to) && (destinations & to))
                buildPawnPromotions(moves, from, to);
        }
    }

    loop = knights;
    while (loop) {
        Square from = popLsb(loop);
        Bitboard attacks = knightAttacks(from) & destinations & pinMask(kSq, pinned, from);
        while (attacks) moves.add(makeMove(from, popLsb(attacks)));
    }

    loop = bishops;
    while (loop) {
        Square from = popLsb(loop);
        Bitboard attacks = bishopAttacks(from, occupied) & destinations & pinMask(kSq, pinned, from);
        while (attacks) moves.add(makeMove(from, popLsb(attacks)));
    }

    loop = rooks;
    while (loop) {
        Square from = popLsb(loop);
        Bitboard attacks = rookAttacks(from, occupied) & destinations & pinMask(kSq, pinned, from);
        while (attacks) moves.add(makeMove(from, popLsb(attacks)));
    }

    buildKingMoves(board, moves, kSq, them, themBB);
}

void genLegalMoves(const Board& board, MoveList& moves, bool quiet, bool noisy) {
    if (quiet) genQuietMoves(board, moves);
    if (noisy) genNoisyMoves(board, moves);
}
