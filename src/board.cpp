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

#include "board.hpp"

#include <cstring>
#include <iomanip>
#include <sstream>
#include <string_view>

#include "bitboards.hpp"
#include "movegen.hpp"
#include "zobrist.hpp"

constexpr std::string_view PieceToChar(" PNBRQK  pnbrqk");
std::string sq2str(Square s) { return std::string{char('a' + fileOf(s)), char('1' + rankOf(s))}; }

std::ostream& operator<<(std::ostream& os, const Board& board) {
    os << "\n +---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_8;; --r) {
        for (File f = FILE_A; f <= FILE_H; ++f) os << " | " << PieceToChar[board.pieceOn(makeSquare(f, r))];

        os << " | " << (1 + r) << "\n +---+---+---+---+---+---+---+---+\n";

        if (r == RANK_1) break;
    }

    os << "   a   b   c   d   e   f   g   h\n"
       << "\nFen: " << board.fen() << "\nKey: " << std::hex << std::uppercase << std::setfill('0') << std::setw(16)
       << board.key() << std::setfill(' ') << std::dec << "\n";

    return os;
}

void Board::setPos(const std::string& fen, StateInfo* si) {
    unsigned char token;
    std::istringstream ss(fen);

    memset(reinterpret_cast<char*>(this), 0, sizeof(Board));
    memset(si, 0, sizeof(StateInfo));
    st = si;

    ss >> std::noskipws;

    int file = FILE_A;
    int rank = RANK_8;

    for (;;) {
        ss >> token;
        if (isspace(token)) break;
        if (isdigit(token)) {
            const int diff = token - '0';
            file += diff;
        } else if (token == '/') {
            --rank;
            file = FILE_A;
        } else {
            putPiece(Piece(PieceToChar.find(token)), makeSquare(File(file), Rank(rank)));
            ++file;
        }
    }

    ss >> token;
    stm = token == 'w' ? WHITE : BLACK;

    ss >> token;
    for (;;) {
        if (!(ss >> token)) break;
        if (isspace(token)) break;
        if (token == '-') {
            ss >> std::ws;
            break;
        }

        if (token == 'K') st->castlingRights |= WHITE_OO;
        if (token == 'Q') st->castlingRights |= WHITE_OOO;
        if (token == 'k') st->castlingRights |= BLACK_OO;
        if (token == 'q') st->castlingRights |= BLACK_OOO;
    }

    ss >> token;
    if (token == '-')
        st->epSquare = SQ_NONE;
    else {
        File f = File(token - 'a');
        ss >> token;
        Rank r = Rank(token - '1');
        st->epSquare = makeSquare(f, r);
    }

    ss >> std::skipws >> st->halfMoves >> gamePly;
    gamePly = std::max(2 * (gamePly - 1), 0) + (stm == BLACK);

    setState();
}

void Board::setState() {
    st->key = 0;

    for (Bitboard b = pieces(); b;) {
        Square s = popLsb(b);
        Piece p = pieceOn(s);
        st->key ^= ZobristKeys[p][s];
    }

    if (st->epSquare != SQ_NONE) st->key ^= ZobristEnPassantKeys[fileOf(st->epSquare)];

    if (stm == BLACK) st->key ^= ZobristTurnKey;

    st->key ^= ZobristCastlingKeys[st->castlingRights];

    st->kingAttackers = checkers(stm);
}

std::string Board::fen() const {
    std::stringstream ss;

    int emptyCnt;
    for (Rank r = RANK_8;; --r) {
        for (File f = FILE_A; f <= FILE_H; ++f) {
            for (emptyCnt = 0; f <= FILE_H && empty(makeSquare(f, r)); ++f) ++emptyCnt;
            if (emptyCnt) ss << emptyCnt;
            if (f <= FILE_H) ss << PieceToChar[pieceOn(makeSquare(f, r))];
        }
        if (r == RANK_1) break;
        ss << "/";
    }

    ss << (stm == WHITE ? " w " : " b ");

    if (canCastle(WHITE_OO)) ss << "K";
    if (canCastle(WHITE_OOO)) ss << "Q";
    if (canCastle(BLACK_OO)) ss << "k";
    if (canCastle(BLACK_OOO)) ss << "q";
    if (!canCastle(ANY_CASTLING)) ss << "-";

    ss << (epSquare() == SQ_NONE ? " - " : " " + sq2str(epSquare()) + " ") << st->halfMoves << " "
       << 1 + (gamePly - (stm == BLACK)) / 2;

    return ss.str();
}

// clang-format off
constexpr int castlingRightsMask[SQUARE_NB] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11
};
// clang-format on

void Board::doMove(Move m, StateInfo& newSi) {
    Key k = st->key ^ ZobristTurnKey;

    std::memcpy(&newSi, st, offsetof(StateInfo, key));
    newSi.prev = st;
    st = &newSi;

    ++gamePly;
    ++st->pliesFromNull;
    ++st->halfMoves;

    Color us = stm;
    Color them = ~us;
    Square from = fromSq(m);
    Square to = toSq(m);
    Piece p = pieceOn(from);
    Piece captured = moveType(m) == EN_PASSANT ? makePiece(them, PAWN) : pieceOn(to);

    if (moveType(m) == CASTLING) {
        Square rFrom = makeSquare((to > from ? FILE_H : FILE_A), rankOf(from));
        Square rTo = makeSquare((to > from ? FILE_F : FILE_D), rankOf(from));
        Piece r = makePiece(us, ROOK);

        removePiece(rFrom);
        putPiece(r, rTo);
        removePiece(from);
        putPiece(p, to);

        k ^= ZobristKeys[r][rFrom] ^ ZobristKeys[r][rTo];
        k ^= ZobristKeys[p][from] ^ ZobristKeys[p][to];
    } else if (moveType(m) == EN_PASSANT) {
        Square epPawnSquare = makeSquare(fileOf(to), rankOf(from));
        removePiece(epPawnSquare);
        removePiece(from);
        putPiece(p, to);
        captured = makePiece(them, PAWN);

        k ^= ZobristKeys[p][from] ^ ZobristKeys[p][to] ^ ZobristKeys[captured][epPawnSquare];
        st->halfMoves = 0;
    } else if (moveType(m) == PROMOTION) {
        removePiece(from);
        if (captured != NO_PIECE) removePiece(to);
        putPiece(makePiece(us, promoPiece(m)), to);

        k ^= ZobristKeys[p][from] ^ ZobristKeys[makePiece(us, promoPiece(m))][to];
    } else {
        removePiece(from);
        if (captured != NO_PIECE) removePiece(to);
        putPiece(p, to);

        k ^= ZobristKeys[p][from] ^ ZobristKeys[p][to];
    }

    if (st->epSquare != SQ_NONE) {
        k ^= ZobristEnPassantKeys[fileOf(st->epSquare)];
        st->epSquare = SQ_NONE;
    }

    k ^= ZobristCastlingKeys[st->castlingRights];
    st->castlingRights &= castlingRightsMask[from] & castlingRightsMask[to];
    k ^= ZobristCastlingKeys[st->castlingRights];

    if (typeOf(p) == PAWN) {
        st->halfMoves = 0;
        if ((int(to) ^ int(from)) == 16) {
            Bitboard adj =
                (fileOf(to) > FILE_A ? squareBB(to + WEST) : 0ull) | (fileOf(to) < FILE_H ? squareBB(to + EAST) : 0ull);
            if (adj & typeBB[PAWN] & colorBB[them]) {
                st->epSquare = to + (us == WHITE ? SOUTH : NORTH);
                k ^= ZobristEnPassantKeys[fileOf(st->epSquare)];
            }
        }
    }
    if (captured != NO_PIECE && moveType(m) != EN_PASSANT) {
        k ^= ZobristKeys[captured][to];
        st->halfMoves = 0;
    }

    st->key = k;
    st->capturedPiece = captured;
    stm = ~stm;
    st->kingAttackers = checkers(stm);
}

void Board::undoMove(Move m) {
    stm = ~stm;

    Color us = stm;
    Square from = fromSq(m);
    Square to = toSq(m);
    Piece p = pieceOn(to);

    if (moveType(m) == PROMOTION) {
        p = makePiece(us, PAWN);
        removePiece(to);
        if (st->capturedPiece != NO_PIECE) putPiece(st->capturedPiece, to);
        putPiece(p, from);
    } else if (moveType(m) == CASTLING) {
        Square rFrom = makeSquare((to > from ? FILE_H : FILE_A), rankOf(from));
        Square rTo = makeSquare((to > from ? FILE_F : FILE_D), rankOf(from));
        Piece r = makePiece(us, ROOK);

        removePiece(rTo);
        putPiece(r, rFrom);
        removePiece(to);
        putPiece(p, from);
    } else if (moveType(m) == EN_PASSANT) {
        Square epPawnSquare = makeSquare(fileOf(to), rankOf(from));
        removePiece(to);
        putPiece(p, from);
        putPiece(st->capturedPiece, epPawnSquare);
    } else {
        removePiece(to);
        putPiece(p, from);
        if (st->capturedPiece != NO_PIECE) putPiece(st->capturedPiece, to);
    }

    st = st->prev;
    --gamePly;
}

void Board::doNullMove(StateInfo& newSi) {
    Key k = st->key ^ ZobristTurnKey;

    std::memcpy(&newSi, st, offsetof(StateInfo, key));
    newSi.prev = st;
    st = &newSi;

    ++gamePly;
    st->pliesFromNull = 0;
    ++st->halfMoves;

    if (st->epSquare != SQ_NONE) {
        k ^= ZobristEnPassantKeys[fileOf(st->epSquare)];
        st->epSquare = SQ_NONE;
    }

    st->key = k;
    st->capturedPiece = NO_PIECE;
    stm = ~stm;
    st->kingAttackers = checkers(stm);
}

void Board::undoNullMove() {
    stm = ~stm;
    st = st->prev;
    --gamePly;
}

bool Board::isDraw(int ply) const { return isRepetition(ply) || isRule50() || isInsufficientMaterial(); }

bool Board::isRepetition(int ply) const {
    Key k = st->key;
    int cnt = 0;

    int bound = std::min(st->halfMoves, gamePly);
    if (st->pliesFromNull < ply) bound = std::min(bound, st->pliesFromNull);

    StateInfo* prev = st->prev;
    for (int i = 1; prev && i <= bound; ++i, prev = prev->prev) {
        if (k == prev->key && (++cnt + (i <= ply)) == 2) return true;
    }
    return false;
}

bool Board::isRule50() const { return st->halfMoves > 99; }

bool Board::isInsufficientMaterial() const {
    if (typeBB[QUEEN] | typeBB[ROOK] | typeBB[PAWN]) return false;

    Bitboard whiteMinors = colorBB[WHITE] & (typeBB[KNIGHT] | typeBB[BISHOP]);
    Bitboard blackMinors = colorBB[BLACK] & (typeBB[KNIGHT] | typeBB[BISHOP]);

    if (popcount(whiteMinors) + popcount(blackMinors) <= 1) return true;

    if (!typeBB[KNIGHT] && popcount(whiteMinors) == 1 && popcount(blackMinors) == 1) {
        Square wB = lsb(whiteMinors);
        Square bB = lsb(blackMinors);
        if (((fileOf(wB) + rankOf(wB) + fileOf(bB) + rankOf(bB)) & 1) == 0) return true;
    }

    return false;
}

std::string Board::move2str(Move m) const {
    std::string str;
    str += sq2str(fromSq(m));
    str += sq2str(toSq(m));
    if (moveType(m) == PROMOTION) str += (" pnbrqk")[promoPiece(m)];
    return str;
}

Move Board::str2move(const std::string& s) const {
    if (s == "0000") return MOVE_NULL;
    Square from = makeSquare(File(s[0] - 'a'), Rank(s[1] - '1'));
    Square to = makeSquare(File(s[2] - 'a'), Rank(s[3] - '1'));
    if (s.size() == 5) {
        PieceType pt;
        switch (s[4]) {
            case 'n':
                pt = KNIGHT;
                break;
            case 'b':
                pt = BISHOP;
                break;
            case 'r':
                pt = ROOK;
                break;
            case 'q':
                pt = QUEEN;
                break;
            default:
                return MOVE_NONE;
        }
        return makeMove(from, to, pt);
    }
    MoveList ml;
    genLegalMoves(*this, ml);
    for (int i = 0; i < ml.size(); ++i) {
        Move m = ml[i];
        if (fromSq(m) == from && toSq(m) == to) return m;
    }
    return MOVE_NONE;
}

uint64_t Board::perft(int ply, bool quiet) {
    if (ply == 0) return 1;
    MoveList ml;
    genLegalMoves(*this, ml);

    uint64_t nodes = 0;
    for (int i = 0; i < ml.size(); ++i) {
        Move m = ml[i];
        uint64_t n = 0;
        StateInfo tempSi;
        doMove(m, tempSi);
        n = perft(ply - 1);
        if (!quiet) std::cout << move2str(m) << ": " << n << std::endl;
        nodes += n;
        undoMove(m);
    }

    if (!quiet) std::cout << "Total nodes: " << nodes << std::endl;

    return nodes;
}
