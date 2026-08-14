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

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "attacks.hpp"
#include "bench.hpp"
#include "bitboards.hpp"
#include "board.hpp"
#include "datagen.hpp"
#include "evaluate.hpp"
#include "masks.hpp"
#include "movegen.hpp"
#include "nnue.hpp"
#include "psqt.hpp"
#include "search.hpp"
#include "thread.hpp"
#include "tt.hpp"
#include "types.hpp"
#include "uci.hpp"
#include "zobrist.hpp"

int main(int argc, char* argv[]) {
    initAttacks();
    initMasks();
    initNNUE();
    initPSQT();
    initZobrist();

    if (argc > 1 && strcmp(argv[1], "bench") == 0) {
        runBench(argc, argv);
        return 0;
    } else if (argc > 1 && strcmp(argv[1], "datagen") == 0) {
        int threads = 1, nodes = 5000, target = 1000000000;
        bool bullet_format = false;

        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "--threads") == 0)
                threads = i + 1 < argc ? atoi(argv[++i]) : 0;
            else if (strcmp(argv[i], "--nodes") == 0)
                nodes = i + 1 < argc ? atoi(argv[++i]) : 0;
            else if (strcmp(argv[i], "--target") == 0)
                target = i + 1 < argc ? atoi(argv[++i]) : 0;
            else if (strcmp(argv[i], "--bullet") == 0)
                bullet_format = true;
        }

        if (nodes < 1) {
            fprintf(stderr, "datagen: --nodes must be >= 1\n");
            return 1;
        }
        if (threads < 1) {
            fprintf(stderr, "datagen: --threads must be >= 1\n");
            return 1;
        }
        if (target < 1) {
            fprintf(stderr, "datagen: --target must be >= 1\n");
            return 1;
        }

        datagen(threads, nodes, target, bullet_format);

        return 0;
    }

    uciLoop();

    return 0;
}
