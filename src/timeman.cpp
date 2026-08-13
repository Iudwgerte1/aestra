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

#include "timeman.hpp"

#include <algorithm>

#include "search.hpp"

void TimeManager::init(const Limits& limits, Color us) {
    startTime = std::chrono::steady_clock::now();
    softTime = timeLimit = 0;

    if (limits.movetime > 0) {
        softTime = timeLimit = limits.movetime;
    } else if (limits.wtime || limits.btime) {
        int timeLeft = us == WHITE ? limits.wtime : limits.btime;
        int inc = us == WHITE ? limits.winc : limits.binc;
        int moveTime = limits.movestogo > 0 ? timeLeft / limits.movestogo : timeLeft / 20 + inc * 3 / 4;
        softTime = std::clamp(moveTime, 10, std::max(10, timeLeft / 4));
        timeLimit = std::min(softTime * 4, std::max(softTime, timeLeft / 4));
    }
}

int TimeManager::elapsed() const {
    return (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime)
        .count();
}
