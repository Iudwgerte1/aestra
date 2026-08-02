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

#ifndef TYPES_HPP
#define TYPES_HPP

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>

typedef uint64_t Key;
typedef uint64_t Bitboard;

constexpr int MAX_MOVES = 256;
constexpr int MAX_PLY = 128;

enum Color : uint8_t { WHITE, BLACK, COLOR_NB = 2 };

enum CastlingRights : uint8_t {
    NO_CASTLING = 0,

    WHITE_OO = 1,
    WHITE_OOO = 2,
    BLACK_OO = 4,
    BLACK_OOO = 8,

    KING_SIDE = WHITE_OO | BLACK_OO,
    QUEEN_SIDE = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING = WHITE_CASTLING | BLACK_CASTLING,

    CASTLING_NB = 16
};

enum Bound : uint8_t { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };

enum Value : int16_t { VALUE_ZERO = 0, VALUE_DRAW = 0, VALUE_NONE = 32002, VALUE_INFINITE = 32001, VALUE_MATE = 32000 };

constexpr bool isValid(Value value) { return value != VALUE_NONE; }

constexpr Value mateIn(int ply) { return Value(int(VALUE_MATE) - ply); }
constexpr Value matedIn(int ply) { return Value(-int(VALUE_MATE) + ply); }

// clang-format off
enum PieceType : uint8_t {
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    ALL_PIECES = 0,
    PIECE_TYPE_NB = 8
};

enum Piece : uint8_t {
    NO_PIECE,
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};
// clang-format on

typedef int Depth;

// clang-format off
enum Square : uint8_t {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE = 64,

    SQUARE_NB = 64
};
// clang-format on

enum Direction : int8_t {
    NORTH = 8,
    EAST = 1,
    SOUTH = -NORTH,
    WEST = -EAST,

    NORTH_EAST = NORTH + EAST,
    NORTH_WEST = NORTH + WEST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST
};

enum File : uint8_t { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB };

enum Rank : uint8_t { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB };

enum Score : uint32_t { SCORE_ZERO };

constexpr Score makeScore(Value mg, Value eg) {
    return static_cast<Score>((static_cast<uint32_t>(static_cast<uint16_t>(mg)) << 16) |
                              static_cast<uint32_t>(static_cast<uint16_t>(eg)));
}

constexpr Value mgValue(Score s) { return static_cast<Value>(static_cast<int16_t>((static_cast<uint32_t>(s) >> 16))); }
constexpr Value egValue(Score s) { return static_cast<Value>(static_cast<int16_t>(static_cast<uint32_t>(s))); }

constexpr Score operator+(Score s1, Score s2) {
    return makeScore(Value(int(mgValue(s1)) + int(mgValue(s2))), Value(int(egValue(s1)) + int(egValue(s2))));
}
constexpr Score operator-(Score s1, Score s2) {
    return makeScore(Value(int(mgValue(s1)) - int(mgValue(s2))), Value(int(egValue(s1)) - int(egValue(s2))));
}
constexpr Score operator-(Score s) { return makeScore(Value(-int(mgValue(s))), Value(-int(egValue(s)))); }
constexpr Score& operator+=(Score& s, Score s2) { return s = s + s2; }
constexpr Score& operator-=(Score& s, Score s2) { return s = s - s2; }

#define ENABLE_INCR_OPERATORS(T)                                \
    constexpr T& operator++(T& d) { return d = T(int(d) + 1); } \
    constexpr T& operator--(T& d) { return d = T(int(d) - 1); }

ENABLE_INCR_OPERATORS(PieceType)
ENABLE_INCR_OPERATORS(Square)
ENABLE_INCR_OPERATORS(File)
ENABLE_INCR_OPERATORS(Rank)

#undef ENABLE_INCR_OPERATORS

constexpr Direction operator+(Direction d1, Direction d2) { return Direction(int(d1) + int(d2)); }
constexpr Direction operator*(int i, Direction d) { return Direction(i * int(d)); }

constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
constexpr Square& operator+=(Square& s, Direction d) { return s = s + d; }
constexpr Square& operator-=(Square& s, Direction d) { return s = s - d; }

constexpr Color operator~(Color c) { return Color(c ^ 1); }
constexpr Piece operator~(Piece p) { return Piece(p ^ 8); }

constexpr Square makeSquare(File f, Rank r) { return Square((r << 3) + f); }
constexpr File fileOf(Square s) { return File(int(s) & 7); }
constexpr Rank rankOf(Square s) { return Rank(int(s) >> 3); }

constexpr Piece makePiece(Color c, PieceType pt) { return Piece((c << 3) + pt); }
constexpr Color colorOf(Piece p) { return Color(p >> 3); }
constexpr PieceType typeOf(Piece p) { return PieceType(p & 7); }

enum MoveType : uint16_t {
    NORMAL,
    PROMOTION = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING = 3 << 14,
};

enum Move : uint16_t { MOVE_NONE = 0, MOVE_NULL = 65 };

struct MoveList {
    Move moves[MAX_MOVES];
    int len;

    MoveList() : len(0) {}
    void add(Move m) { moves[len++] = m; }
    Move operator[](int i) { return moves[i]; }
    int size() { return len; }
    void clear() { len = 0; }
    bool empty() { return len == 0; }
    bool contains(Move m) const { return std::find(moves, moves + len, m) != moves + len; }
};

constexpr Move makeMove(Square from, Square to, MoveType mt = NORMAL) { return Move(mt + (from << 6) + to); }
constexpr Move makeMove(Square from, Square to, PieceType pt) {
    return Move(PROMOTION + ((pt - KNIGHT) << 12) + (from << 6) + to);
}
constexpr Square fromSq(Move m) { return Square((m >> 6) & 63); }
constexpr Square toSq(Move m) { return Square(m & 63); }
constexpr PieceType promoPiece(Move m) { return PieceType(((m >> 12) & 3) + KNIGHT); }
constexpr MoveType moveType(Move m) { return MoveType(m & (3 << 14)); }

const std::string STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

#endif  // TYPES_HPP
