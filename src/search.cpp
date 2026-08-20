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
#include "spsa.hpp"
#include "tt.hpp"

std::mutex ioMutex;

static constexpr int pieceValues[PIECE_TYPE_NB] = {0, 100, 300, 320, 500, 1000, 20000, 0};

static constexpr int MAX_HISTORY = 100000;

static int LMR_REDUCTIONS[MAX_PLY][MAX_MOVES];

void initSearch() {
    memset(LMR_REDUCTIONS, 0, sizeof(LMR_REDUCTIONS));

    for (int i = 1; i < MAX_PLY; ++i)
        for (int j = 1; j < MAX_MOVES; ++j)
            LMR_REDUCTIONS[i][j] = std::clamp<int>(
                std::get<SPSAParam<float>>(spsaParams["lmrBias"]).currValue +
                    std::log(i) * std::log(j) / std::get<SPSAParam<float>>(spsaParams["lmrDivisor"]).currValue,
                1, i - 1);
}

static Value futilityMargin(Depth d) {
    return Value(std::get<SPSAParam<int>>(spsaParams["futilityCoeff"]).currValue * d);
}

static void checkLimit(SearchState& ss) {
    if (ss.limits.nodes && ss.nodes >= (uint64_t)ss.limits.nodes)
        ss.stop = true;
    else if (ss.tm.hardLimitReached())
        ss.stop = true;
}

static int scoreMove(Stack* stack, Board& board, Move m, Move ttMove) {
    if (m == ttMove) return 5000000;

    Square from = fromSq(m);
    Square to = toSq(m);
    MoveType mt = moveType(m);

    if (mt == PROMOTION) return 2000000 + pieceValues[promoPiece(m)];
    if (mt == EN_PASSANT) return 1000000 + pieceValues[PAWN] * 16 - pieceValues[PAWN];
    if (board.pieceOn(to) != NO_PIECE)
        return 1000000 + pieceValues[typeOf(board.pieceOn(to))] * 16 - pieceValues[typeOf(board.pieceOn(fromSq(m)))];

    if (m == stack->killers[0]) return 900000;
    if (m == stack->killers[1]) return 800000;
    return stack->ss->butterflyHistory[board.turn()][from][to];
}

static Move pickNext(Stack* stack, MoveList& moves, int idx) {
    int best = idx;
    for (int i = idx + 1; i < moves.size(); ++i)
        if (stack->moveScores[i] > stack->moveScores[best]) best = i;

    std::swap(moves.moves[idx], moves.moves[best]);
    std::swap(stack->moveScores[idx], stack->moveScores[best]);
    return moves.moves[idx];
}

static Value qsearch(Stack* stack, Board& board, Value alpha, Value beta) {
    ++stack->ss->nodes;
    if ((stack->ss->nodes & 1023) == 0) checkLimit(*stack->ss);
    if (stack->ss->stop) return VALUE_ZERO;

    if (stack->ply >= MAX_PLY) return evaluate(board);

    if (stack->ply > stack->ss->seldepth) stack->ss->seldepth = stack->ply;

    stack->pvLen = stack->ply;

    bool inCheck = board.kingAttackers();
    Value eval = evaluate(board);

    if (board.isDraw(stack->ply)) return VALUE_DRAW;

    if (!inCheck) {
        if (eval >= beta) return eval;
        if (eval > alpha) alpha = eval;
    }

    MoveList moves;
    if (inCheck) {
        genLegalMoves(board, moves);
        if (moves.empty()) return matedIn(stack->ply);
    } else
        genLegalMoves(board, moves, false, true);

    for (int i = 0; i < moves.size(); ++i) stack->moveScores[i] = scoreMove(stack, board, moves[i], MOVE_NONE);

    for (int i = 0; i < moves.size(); ++i) {
        Move m = pickNext(stack, moves, i);

        (stack + 1)->ply = stack->ply + 1;
        (stack + 1)->skipEarlyPruning = false;

        StateInfo newSi;
        board.doMove(m, newSi);
        Value score = -qsearch(stack + 1, board, -beta, -alpha);
        board.undoMove(m);

        if (stack->ss->stop) return VALUE_ZERO;
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

static Value negamax(Stack* stack, Board& board, Value alpha, Value beta, int depth) {
    ++stack->ss->nodes;
    if ((stack->ss->nodes & 1023) == 0) checkLimit(*stack->ss);
    if (stack->ss->stop) return VALUE_ZERO;

    if (stack->ply >= MAX_PLY) return evaluate(board);

    if (stack->ply > stack->ss->seldepth) stack->ss->seldepth = stack->ply;

    stack->pvLen = stack->ply;

    Value eval = VALUE_NONE;
    Value staticEval = evaluate(board);

    bool inCheck = board.kingAttackers();
    bool isPV = int(beta) - int(alpha) > 1;
    bool isRoot = isPV && stack->ply == 0;

    if (board.isDraw(stack->ply)) return VALUE_DRAW;

    if (!isRoot) {
        alpha = std::max(alpha, matedIn(stack->ply));
        beta = std::min(beta, mateIn(stack->ply + 1));
        if (alpha >= beta) return alpha;
    }

    if (depth <= 0) return qsearch(stack, board, alpha, beta);

    bool ttHit = false;
    uint64_t ttEntry = 0;
    TTEntry* tte = TT.probe(board.key(), ttEntry, ttHit);
    Value ttValue = ttHit ? valueFromTT(entryValue(ttEntry), stack->ply) : VALUE_NONE;
    Move ttMove = ttHit ? entryMove(ttEntry) : MOVE_NONE;

    if (!isPV && ttHit && entryDepth(ttEntry) >= depth && ttValue != VALUE_NONE &&
        ((ttValue >= beta ? (entryBound(ttEntry) & BOUND_LOWER) : (entryBound(ttEntry) & BOUND_UPPER)))) {
        return ttValue;
    }

    if (inCheck) {
        eval = VALUE_NONE;
        goto moves_loop;
    } else if (ttHit) {
        if (ttValue != VALUE_NONE)
            if (entryBound(ttEntry) & (ttValue > eval ? BOUND_LOWER : BOUND_UPPER)) eval = ttValue;
    }

    if (stack->skipEarlyPruning) goto moves_loop;

    if (!isPV && depth < 4 && ttMove == MOVE_NONE &&
        eval + std::get<SPSAParam<int>>(spsaParams["razoringMargin"]).currValue <= alpha) {
        if (depth <= 1) return qsearch(stack, board, alpha, beta);
        Value ralpha = alpha - std::get<SPSAParam<int>>(spsaParams["razoringMargin"]).currValue;
        Value v = qsearch(stack, board, ralpha, ralpha + 1);
        if (v <= ralpha) return v;
    }

    if (!isRoot && depth < 7 && eval - futilityMargin(depth) >= beta && eval < VALUE_MATE &&
        popcount(board.pieces()) > 6)
        return eval;

    if (!isPV && eval >= beta && depth >= 3 && !inCheck && popcount(board.pieces()) > 6) {
        StateInfo newSi;
        Depth R = std::get<SPSAParam<int>>(spsaParams["nmpBias"]).currValue +
                  (depth / std::get<SPSAParam<int>>(spsaParams["nmpDivisor"]).currValue);
        board.doNullMove(newSi);
        stack->skipEarlyPruning = true;
        Value score = -negamax(stack, board, -beta, -beta + 1, depth - R);
        stack->skipEarlyPruning = false;
        board.undoNullMove();

        if (score >= beta) return beta;
    }

    if (depth >= 6 && !ttHit &&
        (isPV || staticEval + std::get<SPSAParam<int>>(spsaParams["iidMargin"]).currValue >= beta)) {
        Depth d = std::get<SPSAParam<int>>(spsaParams["iidCoeff"]).currValue * depth /
                      std::get<SPSAParam<int>>(spsaParams["iidDivisor"]).currValue -
                  std::get<SPSAParam<int>>(spsaParams["iidBias"]).currValue;
        stack->skipEarlyPruning = true;
        negamax(stack, board, alpha, beta, d);
        stack->skipEarlyPruning = false;

        tte = TT.probe(board.key(), ttEntry, ttHit);
        ttMove = ttHit ? entryMove(ttEntry) : MOVE_NONE;
    }
moves_loop:
    MoveList moves;
    genLegalMoves(board, moves);

    if (moves.empty()) return inCheck ? matedIn(stack->ply) : VALUE_DRAW;

    if (stack->ply == 0 && stack->ss->threadIdx)
        std::rotate(moves.moves, moves.moves + stack->ss->threadIdx % moves.size(), moves.moves + moves.size());

    for (int i = 0; i < moves.size(); ++i) stack->moveScores[i] = scoreMove(stack, board, moves[i], ttMove);

    Value oldAlpha = alpha;
    Move bestMove = MOVE_NONE;
    Value bestScore = -VALUE_INFINITE;
    int moveCount = 0;

    for (int i = 0; i < moves.size(); ++i) {
        Move m = pickNext(stack, moves, i);

        (stack + 1)->ply = stack->ply + 1;
        (stack + 1)->skipEarlyPruning = false;

        StateInfo newSi;
        board.doMove(m, newSi);
        ++moveCount;

        bool givesCheck = board.kingAttackers();
        int newDepth = depth - 1 + (givesCheck ? 1 : 0);

        Value score;
        bool doFullSearch = moveCount == 1;
        if (!doFullSearch) {
            int reduction = 0;
            if (depth >= 3 && moveCount >= 4 && !inCheck && !givesCheck && moveType(m) == NORMAL)
                reduction = LMR_REDUCTIONS[depth][moveCount];

            score = -negamax(stack + 1, board, -alpha - 1, -alpha, newDepth - reduction);
            if (stack->ss->stop) {
                board.undoMove(m);
                return VALUE_ZERO;
            }
            if (reduction && score > alpha) score = -negamax(stack + 1, board, -alpha - 1, -alpha, newDepth);
            if (score > alpha && score < beta) doFullSearch = true;
        }
        if (doFullSearch) score = -negamax(stack + 1, board, -beta, -alpha, newDepth);

        board.undoMove(m);
        if (stack->ss->stop) return VALUE_ZERO;

        if (score > bestScore) bestScore = score;

        if (score > alpha) {
            alpha = score;
            bestMove = m;

            stack->pv[stack->ply] = m;
            int pvLenChild = stack->ply + 1 < MAX_PLY ? (stack + 1)->pvLen : stack->ply + 1;
            for (int j = stack->ply + 1; j < pvLenChild; ++j) stack->pv[j] = (stack + 1)->pv[j];
            stack->pvLen = pvLenChild;

            if (score >= beta) {
                if (moveType(m) == NORMAL && board.pieceOn(toSq(m)) == NO_PIECE) {
                    if (stack->killers[0] != m) {
                        stack->killers[1] = stack->killers[0];
                        stack->killers[0] = m;
                    }
                    int bonus = std::clamp(depth * depth, -MAX_HISTORY, MAX_HISTORY);
                    int& h = stack->ss->butterflyHistory[board.turn()][fromSq(m)][toSq(m)];
                    h += bonus - h * abs(bonus) / MAX_HISTORY;
                }
                break;
            }
        }
    }

    if (moveCount == 0) {
        Value score = inCheck ? matedIn(stack->ply) : VALUE_DRAW;
        tte->save(board.key(), valueToTT(score, stack->ply), BOUND_EXACT, depth, MOVE_NONE);
        return score;
    }

    Bound bound = bestScore >= beta ? BOUND_LOWER : bestScore <= oldAlpha ? BOUND_UPPER : BOUND_EXACT;
    tte->save(board.key(), valueToTT(bestScore, stack->ply), bound, depth, bestMove);

    return bestScore;
}

SearchResult search(SearchState& ss, Board& board, const Limits& limits,
                    const std::function<void(int, Value)>& report) {
    ss.limits = limits;
    ss.nodes = 0;
    ss.seldepth = 0;
    ss.tm.init(limits, board.turn());
    std::memset(ss.butterflyHistory, 0, sizeof(ss.butterflyHistory));
    std::memset(ss.stack, 0, sizeof(ss.stack));
    for (auto& frame : ss.stack) frame.ss = &ss;
    ss.stack[0].ply = 0;
    ss.stack[0].skipEarlyPruning = false;

    SearchResult result;

    int maxDepth = limits.infinite ? MAX_PLY - 1 : std::min(limits.depth, MAX_PLY - 1);

    Value bestScore = VALUE_NONE;
    bool stable = true;
    Move prevBest = MOVE_NONE;
    for (int depth = 1; depth <= maxDepth && !ss.stop; ++depth) {
        if (depth > 1 && ss.tm.softMs()) {
            int elapsed = ss.tm.elapsed();
            if (elapsed >= ss.tm.hardMs()) break;
            if (elapsed > ss.tm.softMs() && stable) break;
        }

        Value alpha = -VALUE_INFINITE, beta = VALUE_INFINITE;

        if (depth >= 5 && isValid(bestScore) && std::abs(bestScore) < VALUE_MATE) {
            const int delta = 25;
            alpha = std::max(-VALUE_INFINITE, bestScore - delta);
            beta = std::min(VALUE_INFINITE, bestScore + delta);
        }

        Value score = negamax(ss.stack, board, alpha, beta, depth);
        if (ss.stop) break;

        bool reSearched = false;
        while ((score <= alpha || score >= beta) && std::abs(score) < VALUE_MATE) {
            reSearched = true;
            if (score <= alpha)
                alpha = -VALUE_INFINITE;
            else if (score >= beta)
                beta = VALUE_INFINITE;
            score = negamax(ss.stack, board, alpha, beta, depth);
            if (ss.stop) break;
        }
        if (ss.stop) break;

        stable = !reSearched && (ss.stack[0].pv[0] == prevBest);
        prevBest = ss.stack[0].pv[0];

        bestScore = score;
        result.bestMove = ss.stack[0].pv[0];
        result.score = score;
        result.depth = depth;
        result.seldepth = ss.seldepth;
        result.nodes = ss.nodes;

        report(depth, score);

        if (ss.tm.softMs() && std::abs(score) >= VALUE_MATE - MAX_PLY) break;
    }

    return result;
}

uint64_t perft(Board& board, int depth, bool isRoot) {
    if (depth == 0) return 1;

    MoveList moves;
    genLegalMoves(board, moves);

    uint64_t nodes = 0;
    for (int i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        StateInfo newSi;
        board.doMove(m, newSi);
        uint64_t n = perft(board, depth - 1, false);
        board.undoMove(m);
        if (isRoot) std::cout << move2str(m) << ": " << n << std::endl;
        nodes += n;
    }

    return nodes;
}
