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

// The UCI adapter: command parsing, move notation, and all info/bestmove
// string formatting. The engine module sits behind this seam.

#include "uci.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "engine.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "tt.hpp"

namespace {

Engine engine;

void printUci() {
    std::cout << "id name Aestra 2.0.0" << std::endl;
    std::cout << "id author Iudwgerte1" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 16" << std::endl;
    std::cout << "option name Hash type spin default 16 min 1 max 1048576" << std::endl;
    std::cout << "option name UseNNUE type check default true" << std::endl;
    std::cout << "uciok" << std::endl;
}

Move str2move(const Board& b, const std::string& s) {
    if (s == "0000") return MOVE_NULL;
    Square from = makeSquare(File(s[0] - 'a'), Rank(s[1] - '1'));
    Square to = makeSquare(File(s[2] - 'a'), Rank(s[3] - '1'));
    if (s.size() == 5) {
        PieceType pt;
        switch (s[4]) {
            case 'n':
                pt = KNIGHT;
                break;
            case 'b':
                pt = BISHOP;
                break;
            case 'r':
                pt = ROOK;
                break;
            case 'q':
                pt = QUEEN;
                break;
            default:
                return MOVE_NONE;
        }
        return makeMove(from, to, pt);
    }
    MoveList ml;
    genLegalMoves(b, ml);
    for (int i = 0; i < ml.size(); ++i) {
        Move m = ml[i];
        if (fromSq(m) == from && toSq(m) == to) return m;
    }
    return MOVE_NONE;
}

void printInfo(const SearchState& ss, int depth, Value score) {
    uint64_t time = ss.tm.elapsed();
    uint64_t nps = time ? ss.nodes * 1000 / time : 0;

    std::lock_guard<std::mutex> lock(ioMutex);
    std::cout << "info depth " << depth << " seldepth " << ss.seldepth << " score ";
    if (std::abs(score) >= VALUE_MATE - MAX_PLY) {
        int moves = (VALUE_MATE - std::abs(score) + 1) / 2;
        std::cout << "mate " << (score > 0 ? moves : -moves);
    } else {
        std::cout << "cp " << score;
    }
    std::cout << " nodes " << ss.nodes << " nps " << nps << " time " << time << " pv";
    for (int i = 0; i < ss.stack[0].pvLen; ++i) std::cout << " " << move2str(ss.stack[0].pv[i]);
    std::cout << std::endl;
}

void printBestmove(const SearchResult& r) {
    std::lock_guard<std::mutex> lock(ioMutex);
    std::cout << "bestmove " << (r.bestMove != MOVE_NONE ? move2str(r.bestMove) : "0000") << std::endl;
}

void setPosition(std::istringstream& iss) {
    engine.stop();
    engine.waitForSearchFinished();

    std::string token;
    std::string fen;
    iss >> token;

    if (token == "startpos") {
        fen = STARTPOS;
        iss >> token;
    } else if (token == "fen") {
        while (iss >> token && token != "moves") fen += token + " ";
    }

    engine.setPosition(fen, {});

    if (token == "moves") {
        while (iss >> token) {
            Move m = str2move(engine.position(), token);
            if (m == MOVE_NONE) break;
            engine.applyMove(m);
        }
    }
}

void setGo(std::istringstream& iss) {
    std::string token;
    Limits limits;

    while (iss >> token) {
        if (token == "perft") {
            int depth;
            iss >> depth;
            uint64_t nodes = perft(engine.position(), depth);
            std::cout << "Total nodes: " << nodes << std::endl;
            return;
        }
        if (token == "depth") {
            iss >> limits.depth;
        } else if (token == "movetime") {
            iss >> limits.movetime;
        } else if (token == "nodes") {
            iss >> limits.nodes;
        } else if (token == "wtime") {
            iss >> limits.wtime;
        } else if (token == "btime") {
            iss >> limits.btime;
        } else if (token == "winc") {
            iss >> limits.winc;
        } else if (token == "binc") {
            iss >> limits.binc;
        } else if (token == "movestogo") {
            iss >> limits.movestogo;
        } else if (token == "infinite") {
            limits.infinite = true;
        }
    }

    engine.go(limits, [](int depth, Value score) { printInfo(engine.state(), depth, score); }, printBestmove);
}

}  // namespace

void uciLoop() {
    engine.setPosition(STARTPOS, {});

    std::string cmd, token;

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
            iss >> value;
            iss >> value;
            engine.setOption(name, value);
        } else if (token == "ucinewgame") {
            TT.clear();
        } else if (token == "position") {
            setPosition(iss);
        } else if (token == "go") {
            setGo(iss);
        } else if (token == "stop") {
            engine.stop();
            engine.waitForSearchFinished();
        } else if (token == "quit") {
            engine.quit();
            break;
        } else if (token == "board") {
            std::cout << engine.position() << std::endl;
        } else if (token == "eval") {
            std::cout << evaluate(engine.position()) << std::endl;
        }
    }
}
