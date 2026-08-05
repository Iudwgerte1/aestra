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

#include "bench.hpp"

#include <chrono>

#include "board.hpp"
#include "search.hpp"
#include "thread.hpp"
#include "tt.hpp"

void runBench(int argc, char* argv[]) {
    static const char* Benchmarks[] = {
#include "bench.csv"
        ""};

    Board board;
    MainThread mThread;
    Limits limits;

    Value scores[256];
    double times[256];
    uint64_t nodes[256];
    Move bestMoves[256];

    uint64_t totalNodes = 0;
    double totalTime = 0;

    int depth = argc > 2 ? atoi(argv[2]) : 13;
    int nthreads = argc > 3 ? atoi(argv[3]) : 1;
    int mbhash = argc > 4 ? atoi(argv[4]) : 16;

    TT.setSize(mbhash);
    mThread.setThreads(nthreads);

    limits.depth = depth;

    mThread.limits = limits;

    for (int i = 0; strcmp(Benchmarks[i], ""); ++i) {
        auto startTime = std::chrono::steady_clock::now();

        auto states = StateListPtr(new std::deque<StateInfo>(1));
        board.setPos(Benchmarks[i], &states->back());
        mThread.startSearching(board);

        times[i] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
        nodes[i] = mThread.result.nodes;
        bestMoves[i] = mThread.result.bestMove;
        scores[i] = mThread.result.score;

        totalNodes += nodes[i];
        totalTime += times[i];

        TT.clear();
    }

    mThread.waitForSearchFinished();

    std::cout << "Bench: " << totalNodes << " nodes " << int(1000.0f * totalNodes / totalTime) << " nps" << std::endl;
}