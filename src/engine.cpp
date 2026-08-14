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

#include "engine.hpp"
#include "evaluate.hpp"

#include "tt.hpp"

Engine::Engine() : states(new std::deque<StateInfo>(1)) {}

void Engine::setPosition(const std::string& fen, const std::vector<Move>& moves) {
    mainThread.waitForSearchFinished();

    states.reset(new std::deque<StateInfo>(1));
    board.setPos(fen, &states->back());

    for (Move m : moves) applyMove(m);
}

void Engine::applyMove(Move m) {
    states->emplace_back();
    board.doMove(m, states->back());
}

void Engine::go(const Limits& limits, const std::function<void(int, Value)>& report,
                const std::function<void(const SearchResult&)>& done) {
    mainThread.limits = limits;
    mainThread.startSearching(board, report, done);
}

void Engine::stop() { mainThread.stopSearching(); }

void Engine::quit() {
    mainThread.stopSearching();
    mainThread.waitForSearchFinished();
}

void Engine::waitForSearchFinished() { mainThread.waitForSearchFinished(); }

void Engine::setOption(const std::string& name, const std::string& value) {
    if (name == "Hash") {
        mainThread.waitForSearchFinished();
        TT.setSize((size_t)std::stoi(value));
    } else if (name == "Threads") {
        mainThread.setThreads(std::stoi(value));
    } else if (name == "UseNNUE") {
        UseNNUE = value == "true";
    }
}
