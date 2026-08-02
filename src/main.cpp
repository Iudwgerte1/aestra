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
#include <iostream>

#include "attacks.hpp"
#include "bitboards.hpp"
#include "board.hpp"
#include "masks.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include "zobrist.hpp"

struct BenchPosition {
    const char* fen;
    int depth;
    uint64_t expected;
};

static const BenchPosition BenchSuite[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083},
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292},
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487},
    {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, 164075551},
};

void runSuite(bool bench) {
    Board board;
    StateInfo st;
    for (const auto& p : BenchSuite) {
        board.setPos(p.fen, &st);
        uint64_t n = board.perft(p.depth);
        if (bench)
            std::cout << n << " nodes (depth " << p.depth << ")" << std::endl;
        else
            std::cout << p.fen << " depth " << p.depth << " " << (n == p.expected ? "pass" : "fail") << std::endl;
    }
}

int main(int argc, char* argv[]) {
    initAttacks();
    initMasks();
    initZobrist();

    bool bench = argc > 1 && std::string(argv[1]) == "bench";
    auto t0 = std::chrono::steady_clock::now();
    runSuite(bench);
    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1 - t0).count();
    uint64_t nodes = 505793427ull;
    if (bench) std::cout << "Bench: " << secs << " s, " << nodes / secs / 1e6 << " Mn/s" << std::endl;

    return 0;
}
