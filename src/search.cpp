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

#include "search.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

#include "board.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "tt.hpp"

std::mutex ioMutex;

static constexpr int pieceValues[PIECE_TYPE_NB] = {0, 100, 300, 320, 500, 1000, 20000, 0};

static void checkLimit(SearchState& ss) {
    if (ss.limits.nodes && ss.nodes >= (uint64_t)ss.limits.nodes)
        ss.stop = true;
    else if (ss.timeLimit) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - ss.startTime).count() >= ss.timeLimit)
            ss.stop = true;
    }
}

static inline Value valueToTT(Value v, int ply) {
    return v >= mateIn(MAX_PLY) ? Value(int(v) + ply) : v <= matedIn(MAX_PLY) ? Value(int(v) - ply) : v;
}

static inline Value valueFromTT(Value v, int ply) {
    return v >= mateIn(MAX_PLY) ? Value(int(v) - ply) : v <= matedIn(MAX_PLY) ? Value(int(v) + ply) : v;
}

static int scoreMove(SearchState& ss, Board& board, Move m, Move ttMove, int ply) {
    if (m == ttMove) return 5000000;

    Square to = toSq(m);
    MoveType mt = moveType(m);

    if (mt == PROMOTION) return 2000000 + pieceValues[promoPiece(m)];
    if (mt == EN_PASSANT) return 1000000 + pieceValues[PAWN] * 16 - pieceValues[PAWN];
    if (board.pieceOn(to) != NO_PIECE)
        return 1000000 + pieceValues[typeOf(board.pieceOn(to))] * 16 - pieceValues[typeOf(board.pieceOn(fromSq(m)))];

    if (m == ss.killers[ply][0]) return 900000;
    if (m == ss.killers[ply][1]) return 800000;
    return ss.history[board.turn()][fromSq(m)][to];
}

static Move pickNext(SearchState& ss, MoveList& moves, int idx) {
    int best = idx;
    for (int i = idx + 1; i < moves.size(); ++i)
        if (ss.moveScores[i] > ss.moveScores[best]) best = i;

    std::swap(moves.moves[idx], moves.moves[best]);
    std::swap(ss.moveScores[idx], ss.moveScores[best]);
    return moves.moves[idx];
}

static Value quiescence(SearchState& ss, Board& board, Value alpha, Value beta, int ply) {
    ++ss.nodes;
    if ((ss.nodes & 1023) == 0) checkLimit(ss);
    if (ss.stop) return VALUE_ZERO;

    if (ply >= MAX_PLY) return evaluate(board);

    bool inCheck = board.kingAttackers();
    Value eval = evaluate(board);

    if (!inCheck) {
        if (eval >= beta) return eval;
        if (eval > alpha) alpha = eval;
    }

    MoveList moves;
    if (inCheck) {
        genLegalMoves(board, moves);
        if (moves.empty()) return matedIn(ply);
    } else
        genNoisyMoves(board, moves);

    for (int i = 0; i < moves.size(); ++i) ss.moveScores[i] = scoreMove(ss, board, moves[i], MOVE_NONE, ply);

    for (int i = 0; i < moves.size(); ++i) {
        Move m = pickNext(ss, moves, i);

        StateInfo newSi;
        board.doMove(m, newSi);
        if (board.checkers(~board.turn())) {
            board.undoMove(m);
            continue;
        }

        Value score = -quiescence(ss, board, -beta, -alpha, ply + 1);
        board.undoMove(m);

        if (ss.stop) return VALUE_ZERO;
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

static Value negamax(SearchState& ss, Board& board, Value alpha, Value beta, int depth, int ply) {
    ss.pvLen[ply] = ply;

    ++ss.nodes;
    if ((ss.nodes & 1023) == 0) checkLimit(ss);
    if (ss.stop) return VALUE_ZERO;

    bool inCheck = board.kingAttackers();
    bool isPV = int(beta) - int(alpha) > 1;

    if (ply > 0) {
        if (board.isDraw(std::min(ply, board.state()->pliesFromNull))) return VALUE_DRAW;

        alpha = std::max(alpha, matedIn(ply));
        beta = std::min(beta, mateIn(ply + 1));
        if (alpha >= beta) return alpha;

        ss.seldepth = std::max(ss.seldepth, ply);
    }

    if (depth <= 0) return quiescence(ss, board, alpha, beta, ply);
    if (ply >= MAX_PLY) return evaluate(board);

    bool found = false;
    TTEntry* tte = TT.probe(board.key(), found);
    Move ttMove = found ? tte->move() : MOVE_NONE;
    if (found && !isPV && tte->depth() >= depth) {
        Value ttValue = valueFromTT(tte->value(), ply);
        if (tte->bound() == BOUND_EXACT || (tte->bound() == BOUND_LOWER && ttValue >= beta) ||
            (tte->bound() == BOUND_UPPER && ttValue <= alpha))
            return ttValue;
    }

    if (!isPV && depth >= 3 && !inCheck && board.count(ALL_PIECES) > 6) {
        if (evaluate(board) >= beta) return beta;

        StateInfo newSi;
        board.doNullMove(newSi);
        Value score = -negamax(ss, board, -beta, -beta + 1, depth - 4, ply + 1);
        board.undoNullMove();

        if (ss.stop) return VALUE_ZERO;
        if (score >= beta) return beta;
    }

    MoveList moves;
    genQuietMoves(board, moves);
    genNoisyMoves(board, moves);

    if (moves.empty()) return inCheck ? matedIn(ply) : VALUE_DRAW;

    if (ply == 0 && ss.threadIdx)
        std::rotate(moves.moves, moves.moves + ss.threadIdx % moves.size(), moves.moves + moves.size());

    for (int i = 0; i < moves.size(); ++i) ss.moveScores[i] = scoreMove(ss, board, moves[i], ttMove, ply);

    Value oldAlpha = alpha;
    Move bestMove = MOVE_NONE;
    Value bestScore = -VALUE_INFINITE;
    int moveCount = 0;

    for (int i = 0; i < moves.size(); ++i) {
        Move m = pickNext(ss, moves, i);

        StateInfo newSi;
        board.doMove(m, newSi);
        if (board.checkers(~board.turn())) {
            board.undoMove(m);
            continue;
        }
        ++moveCount;

        bool givesCheck = board.kingAttackers();
        int newDepth = depth - 1 + (givesCheck ? 1 : 0);

        Value score;
        bool doFullSearch = moveCount == 1;
        if (!doFullSearch) {
            int reduction = 0;
            if (depth >= 3 && moveCount >= 4 && !inCheck && !givesCheck && moveType(m) == NORMAL)
                reduction = 1 + moveCount / 8;

            score = -negamax(ss, board, -alpha - 1, -alpha, newDepth - reduction, ply + 1);
            if (ss.stop) {
                board.undoMove(m);
                return VALUE_ZERO;
            }
            if (reduction && score > alpha) score = -negamax(ss, board, -alpha - 1, -alpha, newDepth, ply + 1);
            if (score > alpha && score < beta) doFullSearch = true;
        }
        if (doFullSearch) score = -negamax(ss, board, -beta, -alpha, newDepth, ply + 1);

        board.undoMove(m);
        if (ss.stop) return VALUE_ZERO;

        if (score > bestScore) bestScore = score;

        if (score > alpha) {
            alpha = score;
            bestMove = m;

            ss.pvTable[ply][ply] = m;
            for (int j = ply + 1; j < ss.pvLen[ply + 1]; ++j) ss.pvTable[ply][j] = ss.pvTable[ply + 1][j];
            ss.pvLen[ply] = ss.pvLen[ply + 1];

            if (score >= beta) {
                if (moveType(m) == NORMAL && board.pieceOn(toSq(m)) == NO_PIECE) {
                    if (ss.killers[ply][0] != m) {
                        ss.killers[ply][1] = ss.killers[ply][0];
                        ss.killers[ply][0] = m;
                    }
                    int& h = ss.history[board.turn()][fromSq(m)][toSq(m)];
                    h = std::clamp(h + depth * depth, int(-VALUE_MATE), int(VALUE_MATE));
                }
                break;
            }
        }
    }

    if (moveCount == 0) {
        Value score = inCheck ? matedIn(ply) : VALUE_DRAW;
        tte->save(board.key(), valueToTT(score, ply), BOUND_EXACT, depth, MOVE_NONE);
        return score;
    }

    Bound bound = bestScore >= beta ? BOUND_LOWER : bestScore <= oldAlpha ? BOUND_UPPER : BOUND_EXACT;
    tte->save(board.key(), valueToTT(bestScore, ply), bound, depth, bestMove);

    return bestScore;
}

void printInfo(const SearchState& ss, const Board& board, int depth, Value score) {
    uint64_t time =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - ss.startTime).count();
    uint64_t nps = time ? ss.nodes * 1000 / time : 0;

    std::lock_guard<std::mutex> lock(ioMutex);
    std::cout << "info depth " << depth << " seldepth " << ss.seldepth << " score ";
    if (std::abs(score) >= VALUE_MATE - MAX_PLY) {
        int moves = (VALUE_MATE - std::abs(score) + 1) / 2;
        std::cout << "mate " << (score > 0 ? moves : -moves);
    } else {
        std::cout << "cp " << score;
    }
    std::cout << " nodes " << ss.nodes << " nps " << nps << " time " << time << " pv";
    for (int i = 0; i < ss.pvLen[0]; ++i) std::cout << " " << board.move2str(ss.pvTable[0][i]);
    std::cout << std::endl;
}

SearchResult search(SearchState& ss, Board& board, const Limits& limits,
                    const std::function<void(int, Value)>& report) {
    ss.limits = limits;
    ss.stop = false;
    ss.nodes = 0;
    ss.seldepth = 0;
    ss.startTime = std::chrono::steady_clock::now();
    TT.newSearch();
    std::fill(&ss.killers[0][0], &ss.killers[0][0] + MAX_PLY * 2, MOVE_NONE);
    std::memset(ss.history, 0, sizeof(ss.history));

    SearchResult result;

    if (limits.movetime > 0) {
        ss.timeLimit = limits.movetime;
    } else if (limits.wtime || limits.btime) {
        int timeLeft = board.turn() == WHITE ? limits.wtime : limits.btime;
        int inc = board.turn() == WHITE ? limits.winc : limits.binc;
        int moveTime = timeLeft / 20 + inc * 3 / 4;
        ss.timeLimit = std::clamp(moveTime, 10, std::max(10, timeLeft / 4));
    } else {
        ss.timeLimit = 0;
    }

    int maxDepth = limits.infinite ? MAX_PLY - 1 : std::min(limits.depth, MAX_PLY - 1);

    Value bestScore = VALUE_NONE;
    for (int depth = 1; depth <= maxDepth && !ss.stop; ++depth) {
        Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;

        if (depth >= 5 && isValid(bestScore) && std::abs(bestScore) < VALUE_MATE) {
            const int delta = 25;
            alpha = std::max(-VALUE_INFINITE, bestScore - delta);
            beta = std::min(VALUE_INFINITE, bestScore + delta);
        }

        Value score = negamax(ss, board, alpha, beta, depth, 0);
        if (ss.stop) break;

        while ((score <= alpha || score >= beta) && std::abs(score) < VALUE_MATE) {
            if (score <= alpha)
                alpha = -VALUE_INFINITE;
            else if (score >= beta)
                beta = VALUE_INFINITE;
            score = negamax(ss, board, alpha, beta, depth, 0);
            if (ss.stop) break;
        }
        if (ss.stop) break;

        bestScore = score;
        result.bestMove = ss.pvTable[0][0];
        result.score = score;
        result.depth = depth;
        result.seldepth = ss.seldepth;
        result.nodes = ss.nodes;

        report(depth, score);

        if (bestScore >= mateIn(2)) break;
    }

    return result;
}
