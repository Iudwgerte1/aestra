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

#include "attacks.hpp"
#include "bitboards.hpp"
#include "board.hpp"
#include "masks.hpp"
#include "movegen.hpp"
#include "types.hpp"
#include "zobrist.hpp"

#include <chrono>

int main() {
    initAttacks();
    initZobrist();

    Board board;
    StateInfo st;
    board.setPos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &st);

    auto start = std::chrono::high_resolution_clock::now();
    uint64_t nodes = board.perft(6, false);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    std::cout << "Nodes per ms: " << nodes / std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;

    return 0;
}
