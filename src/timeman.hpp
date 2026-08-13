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

#ifndef TIMEMAN_HPP
#define TIMEMAN_HPP

#include <chrono>

#include "types.hpp"

struct Limits;

using TimePoint = std::chrono::steady_clock::time_point;

class TimeManager {
public:
    void init(const Limits& limits, Color us);

    int elapsed() const;
    bool hardLimitReached() const { return timeLimit && elapsed() >= timeLimit; }
    bool softLimitReached() const { return softTime && elapsed() > softTime; }
    int hardMs() const { return timeLimit; }
    int softMs() const { return softTime; }

private:
    TimePoint startTime;
    int softTime = 0;
    int timeLimit = 0;
};

#endif  // TIMEMAN_HPP
