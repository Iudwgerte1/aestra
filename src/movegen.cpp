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

typedef Bitboard (*JumperFunc)(Square sq);
typedef Bitboard (*SliderFunc)(Square sq, Bitboard occ);

void buildEnpassantMoves(MoveList& moves, Bitboard attacks, Square epSq) {
    while (attacks) moves.add(makeMove(popLsb(attacks), epSq, EN_PASSANT));
}

void buildPawnMoves(MoveList& moves, Bitboard attacks, Direction delta) {
    while (attacks) {
        Square sq = popLsb(attacks);
        moves.add(makeMove(sq + delta, sq));
    }
}

void buildPawnPromotions(MoveList& moves, Bitboard attacks, Direction delta) {
    while (attacks) {
        Square sq = popLsb(attacks);
        moves.add(makeMove(sq + delta, sq, KNIGHT));
        moves.add(makeMove(sq + delta, sq, BISHOP));
        moves.add(makeMove(sq + delta, sq, ROOK));
        moves.add(makeMove(sq + delta, sq, QUEEN));
    }
}

void buildNormalMoves(MoveList& moves, Bitboard attacks, Square sq) {
    while (attacks) moves.add(makeMove(sq, popLsb(attacks)));
}

void buildJumperMoves(JumperFunc func, MoveList& moves, Bitboard pieces, Bitboard targets) {
    while (pieces) {
        Square from = popLsb(pieces);
        Bitboard jumps = func(from) & targets;
        while (jumps) {
            Square to = popLsb(jumps);
            moves.add(makeMove(from, to));
        }
    }
}

void buildSliderMoves(SliderFunc func, MoveList& moves, Bitboard pieces, Bitboard occ, Bitboard targets) {
    while (pieces) {
        Square from = popLsb(pieces);
        Bitboard sliders = func(from, occ) & targets;
        while (sliders) {
            Square to = popLsb(sliders);
            moves.add(makeMove(from, to));
        }
    }
}

void genQuietMoves(const Board& board, MoveList& moves) {
    const Direction Forward = board.turn() == WHITE ? SOUTH : NORTH;
    const Bitboard Rank3Relative = board.turn() == WHITE ? RANK_3BB : RANK_6BB;

    Bitboard us = board.pieces(board.turn());
    Bitboard them = board.pieces(~board.turn());
    Bitboard occupied = us | them;

    Bitboard pawns = us & board.pieces(PAWN);
    Bitboard knights = us & board.pieces(KNIGHT);
    Bitboard bishops = us & (board.pieces(BISHOP) | board.pieces(QUEEN));
    Bitboard rooks = us & (board.pieces(ROOK) | board.pieces(QUEEN));
    Bitboard kings = us & board.pieces(KING);

    if (several(board.kingAttackers())) return buildJumperMoves(&kingAttacks, moves, kings, ~occupied);

    Bitboard destinations = !board.kingAttackers() ? ~occupied : bitsBetween(lsb(kings), lsb(board.kingAttackers()));

    Bitboard pawnForwardOne = pawnAdvance(pawns, occupied, board.turn()) & ~PROMOTION_RANKS;
    Bitboard pawnForwardTwo = pawnAdvance(pawnForwardOne & Rank3Relative, occupied, board.turn());

    buildPawnMoves(moves, pawnForwardOne & destinations, Forward);
    buildPawnMoves(moves, pawnForwardTwo & destinations, 2 * Forward);

    buildJumperMoves(&knightAttacks, moves, knights, destinations);
    buildSliderMoves(&bishopAttacks, moves, bishops, occupied, destinations);
    buildSliderMoves(&rookAttacks, moves, rooks, occupied, destinations);
    buildJumperMoves(&kingAttacks, moves, kings, ~occupied);

    if (!board.kingAttackers()) {
        Square kingSq = board.turn() == WHITE ? SQ_E1 : SQ_E8;
        if (board.canCastle(board.turn() == WHITE ? WHITE_OO : BLACK_OO)) {
            Square kTo = makeSquare(FILE_G, rankOf(kingSq));
            Square fSq = makeSquare(FILE_F, rankOf(kingSq));
            if (!(occupied & (kTo | fSq)) && !board.attackersTo(fSq, ~board.turn()) &&
                !board.attackersTo(kTo, ~board.turn()))
                moves.add(makeMove(kingSq, kTo, CASTLING));
        }
        if (board.canCastle(board.turn() == WHITE ? WHITE_OOO : BLACK_OOO)) {
            Square kTo = makeSquare(FILE_C, rankOf(kingSq));
            Square bSq = makeSquare(FILE_B, rankOf(kingSq));
            Square dSq = makeSquare(FILE_D, rankOf(kingSq));
            if (!(occupied & (kTo | dSq | bSq)) && !board.attackersTo(dSq, ~board.turn()) &&
                !board.attackersTo(kTo, ~board.turn()))
                moves.add(makeMove(kingSq, kTo, CASTLING));
        }
    }
}

void genNoisyMoves(const Board& board, MoveList& moves) {
    const Direction Left = board.turn() == WHITE ? SOUTH_EAST : NORTH_WEST;
    const Direction Right = board.turn() == WHITE ? SOUTH_WEST : NORTH_EAST;
    const Direction Forward = board.turn() == WHITE ? SOUTH : NORTH;

    Bitboard us = board.pieces(board.turn());
    Bitboard them = board.pieces(~board.turn());
    Bitboard occupied = us | them;

    Bitboard pawns = us & board.pieces(PAWN);
    Bitboard knights = us & board.pieces(KNIGHT);
    Bitboard bishops = us & (board.pieces(BISHOP) | board.pieces(QUEEN));
    Bitboard rooks = us & (board.pieces(ROOK) | board.pieces(QUEEN));
    Bitboard kings = us & board.pieces(KING);

    if (several(board.kingAttackers())) return buildJumperMoves(&kingAttacks, moves, kings, them);

    Bitboard destinations = board.kingAttackers() ? board.kingAttackers() : them;

    Bitboard pawnEnpassant = pawnEnPassantAttacks(pawns, board.epSquare(), board.turn());
    Bitboard pawnLeft = pawnLeftAttacks(pawns, them, board.turn());
    Bitboard pawnRight = pawnRightAttacks(pawns, them, board.turn());
    Bitboard pawnPromoForward = pawnAdvance(pawns, occupied, board.turn()) & PROMOTION_RANKS;
    Bitboard pawnPromoLeft = pawnLeft & PROMOTION_RANKS;
    Bitboard pawnPromoRight = pawnRight & PROMOTION_RANKS;
    pawnLeft &= ~PROMOTION_RANKS;
    pawnRight &= ~PROMOTION_RANKS;

    buildEnpassantMoves(moves, pawnEnpassant, board.epSquare());
    buildPawnMoves(moves, pawnLeft & destinations, Left);
    buildPawnMoves(moves, pawnRight & destinations, Right);
    buildPawnPromotions(moves, pawnPromoForward, Forward);
    buildPawnPromotions(moves, pawnPromoLeft, Left);
    buildPawnPromotions(moves, pawnPromoRight, Right);

    buildJumperMoves(&knightAttacks, moves, knights, them);
    buildSliderMoves(&bishopAttacks, moves, bishops, occupied, them);
    buildSliderMoves(&rookAttacks, moves, rooks, occupied, them);
    buildJumperMoves(&kingAttacks, moves, kings, them);
}

void genLegalMoves(const Board& board, MoveList& moves, bool quiet) {
    MoveList pseudoLegals;

    genQuietMoves(board, pseudoLegals);
    if (!quiet) genNoisyMoves(board, pseudoLegals);

    Board tmpBoard;
    memcpy(&tmpBoard, &board, sizeof(Board));

    for (int i = 0; i < pseudoLegals.size(); ++i) {
        Move m = pseudoLegals[i];
        StateInfo tempSi;
        tmpBoard.doMove(m, tempSi);
        if (!tmpBoard.checkers(~tmpBoard.turn())) moves.add(m);
        tmpBoard.undoMove(m);
    }
}
