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

#include "uci.hpp"

#include <deque>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "board.hpp"
#include "thread.hpp"
#include "tt.hpp"

namespace {

Board board;
StateListPtr states(new std::deque<StateInfo>(1));
MainThread mainThread;

void printUci() {
    std::cout << "id name Aestra" << std::endl;
    std::cout << "id author Iudwgerte1" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 256" << std::endl;
    std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
    std::cout << "uciok" << std::endl;
}

void setPosition(std::istringstream& iss) {
    mainThread.waitForSearchFinished();

    std::string token;
    iss >> token;

    if (token == "startpos") {
        states = StateListPtr(new std::deque<StateInfo>(1));
        board.setPos(STARTPOS, &states->back());
        iss >> token;
    } else if (token == "fen") {
        std::string fen;
        while (iss >> token && token != "moves") fen += token + " ";
        states = StateListPtr(new std::deque<StateInfo>(1));
        board.setPos(fen, &states->back());
    }

    if (token == "moves") {
        while (iss >> token) {
            Move m = board.str2move(token);
            if (m == MOVE_NONE) break;
            states->emplace_back();
            board.doMove(m, states->back());
        }
    }
}

void setGo(std::istringstream& iss) {
    std::string token;
    mainThread.limits = Limits();

    while (iss >> token) {
        if (token == "depth") {
            iss >> mainThread.limits.depth;
        } else if (token == "movetime") {
            iss >> mainThread.limits.movetime;
        } else if (token == "nodes") {
            iss >> mainThread.limits.nodes;
        } else if (token == "wtime") {
            iss >> mainThread.limits.wtime;
        } else if (token == "btime") {
            iss >> mainThread.limits.btime;
        } else if (token == "winc") {
            iss >> mainThread.limits.winc;
        } else if (token == "binc") {
            iss >> mainThread.limits.binc;
        } else if (token == "movestogo") {
            iss >> mainThread.limits.movestogo;
        } else if (token == "infinite") {
            mainThread.limits.infinite = true;
        }
    }

    mainThread.startSearching(board);
}

}

void uciLoop() {
    std::string cmd, token;
    std::cout << "id name Aestra" << std::endl;
    std::cout << "id author Iudwgerte1" << std::endl;
    std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
    std::cout << "uciok" << std::endl;

    while (std::getline(std::cin, cmd)) {
        std::istringstream iss(cmd);
        iss >> std::skipws >> token;

        if (token == "uci") {
            printUci();
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "setoption") {
            std::string name, value;
            iss >> name;
            iss >> name;
            if (name == "Hash") {
                iss >> value;
                iss >> value;
                TT.setSize((size_t)std::stoi(value));
            } else if (name == "Threads") {
                iss >> value;
                iss >> value;
                mainThread.setThreads(std::stoi(value));
            }
        } else if (token == "ucinewgame") {
            TT.clear();
        } else if (token == "position") {
            setPosition(iss);
        } else if (token == "go") {
            setGo(iss);
        } else if (token == "stop") {
            mainThread.stopSearching();
            mainThread.waitForSearchFinished();
        } else if (token == "quit") {
            mainThread.stopSearching();
            mainThread.waitForSearchFinished();
            break;
        }
    }
}
