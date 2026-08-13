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

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <functional>
#include <string>
#include <vector>

#include "board.hpp"
#include "search.hpp"
#include "thread.hpp"

class Engine {
public:
    Engine();

    void setPosition(const std::string& fen, const std::vector<Move>& moves);
    void applyMove(Move m);
    void go(const Limits& limits, const std::function<void(int, Value)>& report,
            const std::function<void(const SearchResult&)>& done);
    void stop();
    void quit();
    void waitForSearchFinished();
    void setOption(const std::string& name, const std::string& value);

    Board& position() { return board; }
    const SearchState& state() const { return mainThread.state(); }

private:
    Board board;
    StateListPtr states;
    MainThread mainThread;
};

#endif  // ENGINE_HPP
