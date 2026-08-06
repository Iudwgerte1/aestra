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

#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

#include "board.hpp"
#include "types.hpp"

extern std::mutex ioMutex;

struct Limits {
    int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0;
    int movetime = 0;
    int depth = MAX_PLY;
    int nodes = 0;
    bool infinite = false;
};

struct SearchResult {
    Move bestMove = MOVE_NONE;
    Value score = VALUE_NONE;
    int depth = 0;
    int seldepth = 0;
    uint64_t nodes = 0;
};

struct SearchState {
    Limits limits;
    std::atomic<bool> stop{false};
    uint64_t nodes = 0;
    int seldepth = 0;
    Move pvTable[MAX_PLY][MAX_PLY];
    int pvLen[MAX_PLY];
    Move killers[MAX_PLY][2];
    int history[COLOR_NB][SQUARE_NB][SQUARE_NB];
    int moveScores[MAX_MOVES];
    std::chrono::steady_clock::time_point startTime;
    int timeLimit = 0;
    int softTime = 0;
    int threadIdx = 0;
};

SearchResult search(SearchState& ss, Board& board, const Limits& limits, const std::function<void(int, Value)>& report);

void printInfo(const SearchState& ss, const Board& board, int depth, Value score);

#endif  // SEARCH_HPP