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

#ifndef BOARD_HPP
#define BOARD_HPP

#include <array>
#include <deque>
#include <iostream>
#include <memory>
#include <string>

#include "attacks.hpp"
#include "bitboards.hpp"
#include "types.hpp"

struct StateInfo {
    int castlingRights;
    int halfMoves;
    int pliesFromNull;
    Square epSquare;

    Key key;
    StateInfo* prev;
    Piece capturedPiece;
};

typedef std::unique_ptr<std::deque<StateInfo>> StateListPtr;

class Board {
public:
    Board() = default;
    Board(const Board&) = delete;
    Board& operator=(const Board&) = delete;

    void setPos(const std::string& fen, StateInfo* si);
    std::string fen() const;

    Bitboard pieces() const;
    template <typename... PieceTypes>
    Bitboard pieces(PieceTypes... pts) const;
    Bitboard pieces(Color c) const;
    template <typename... PieceTypes>
    Bitboard pieces(Color c, PieceTypes... pts) const;
    Piece pieceOn(Square s) const;
    const std::array<Piece, SQUARE_NB>& pieceArray() const;
    Square epSquare() const;
    bool empty(Square s) const;
    int count(PieceType pt) const;
    int count(Piece p) const;

    bool canCastle(CastlingRights cr) const;

    Bitboard attackersTo(Square s, Color c) const;
    Bitboard checkers(Color c) const;

    void doMove(Move m, StateInfo& newSi);
    void undoMove(Move m);
    void doNullMove(StateInfo& newSi);
    void undoNullMove();

    std::string move2str(Move m) const;
    Move str2move(const std::string& s) const;

    Key key() const;

    Color turn() const;
    int Ply() const;
    bool isDraw(int ply) const;
    bool isRepetition(int ply) const;
    bool isRule50() const;
    bool isInsufficientMaterial() const;

    StateInfo* state() const;

    void putPiece(Piece p, Square s);
    void removePiece(Square s);

    uint64_t perft(int ply, bool quiet = true);

private:
    void setState();

    std::array<Piece, SQUARE_NB> board;
    std::array<Bitboard, PIECE_TYPE_NB> typeBB;
    std::array<Bitboard, COLOR_NB> colorBB;

    int pieceCount[PIECE_NB];
    StateInfo* st;
    int gamePly;
    Color stm;
};

std::ostream& operator<<(std::ostream& os, const Board& b);

inline void Board::putPiece(Piece p, Square s) {
    board[s] = p;
    typeBB[ALL_PIECES] |= typeBB[typeOf(p)] |= s;
    colorBB[colorOf(p)] |= s;
    pieceCount[p]++;
    pieceCount[makePiece(colorOf(p), ALL_PIECES)]++;
}
inline void Board::removePiece(Square s) {
    Piece p = board[s];

    typeBB[ALL_PIECES] ^= s;
    typeBB[typeOf(p)] ^= s;
    colorBB[colorOf(p)] ^= s;
    board[s] = NO_PIECE;
    pieceCount[p]--;
    pieceCount[makePiece(colorOf(p), ALL_PIECES)]--;
}

inline Bitboard Board::pieces() const { return typeBB[ALL_PIECES]; }

template <typename... PieceTypes>
inline Bitboard Board::pieces(PieceTypes... pts) const {
    return (typeBB[pts] | ...);
}

inline Bitboard Board::pieces(Color c) const { return colorBB[c]; }

template <typename... PieceTypes>
inline Bitboard Board::pieces(Color c, PieceTypes... pts) const {
    return pieces(c) & pieces(pts...);
}

inline Piece Board::pieceOn(Square s) const { return board[s]; }

inline const std::array<Piece, SQUARE_NB>& Board::pieceArray() const { return board; }

inline Square Board::epSquare() const { return st->epSquare; }

inline bool Board::empty(Square s) const { return board[s] == NO_PIECE; }

inline int Board::count(PieceType pt) const {
    return pieceCount[makePiece(WHITE, pt)] + pieceCount[makePiece(BLACK, pt)];
}

inline int Board::count(Piece p) const { return pieceCount[p]; }

inline bool Board::canCastle(CastlingRights cr) const { return st->castlingRights & cr; }

inline Bitboard Board::attackersTo(Square s, Color c) const {
    return (pawnAttacks(~c, s) & pieces(c, PAWN)) | (knightAttacks(s) & pieces(c, KNIGHT)) |
           (bishopAttacks(s, pieces()) & (pieces(c, BISHOP) | pieces(c, QUEEN))) |
           (rookAttacks(s, pieces()) & (pieces(c, ROOK) | pieces(c, QUEEN))) | (kingAttacks(s) & pieces(c, KING));
}

inline Bitboard Board::checkers(Color c) const { return attackersTo(lsb(pieces(c, KING)), ~c); }

inline Key Board::key() const { return st->key; }

inline Color Board::turn() const { return stm; }

inline int Board::Ply() const { return gamePly; }

inline StateInfo* Board::state() const { return st; }

#endif  // BOARD_HPP
