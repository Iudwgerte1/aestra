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

// bench: the second adapter at the engine seam — set position, search at a
// fixed depth, collect results. No UCI strings involved.

#include "bench.hpp"

#include <chrono>
#include <iostream>

#include "engine.hpp"
#include "tt.hpp"

void runBench(int argc, char* argv[]) {
    static const char* Benchmarks[] = {
#include "bench.csv"
        ""};

    Engine engine;

    Value scores[256];
    double times[256];
    uint64_t nodes[256];
    Move bestMoves[256];

    uint64_t totalNodes = 0;
    double totalTime = 0;

    int depth = argc > 2 ? atoi(argv[2]) : 13;
    int nthreads = argc > 3 ? atoi(argv[3]) : 1;
    int mbhash = argc > 4 ? atoi(argv[4]) : 16;

    engine.setOption("Hash", std::to_string(mbhash));
    engine.setOption("Threads", std::to_string(nthreads));

    Limits limits;
    limits.depth = depth;

    SearchResult lastResult;

    for (int i = 0; strcmp(Benchmarks[i], ""); ++i) {
        auto startTime = std::chrono::steady_clock::now();

        engine.setPosition(Benchmarks[i], {});
        engine.go(limits, {}, [&](const SearchResult& r) { lastResult = r; });
        engine.waitForSearchFinished();

        times[i] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime)
                       .count();
        nodes[i] = lastResult.nodes;
        bestMoves[i] = lastResult.bestMove;
        scores[i] = lastResult.score;

        totalNodes += nodes[i];
        totalTime += times[i];

        TT.clear();
    }

    std::cout << "Bench: " << totalNodes << " nodes " << int(1000.0f * totalNodes / totalTime) << " nps" << std::endl;
}
