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

#include "genfens.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "board.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "types.hpp"

namespace {

constexpr int MAX_OPENING_EVAL = 100;
constexpr int DEFAULT_PLIES = 10;
constexpr int MAX_PLIES = 512;

struct GenfensConfig {
    int count = 0;
    uint64_t seed = 0;
    std::string book = "None";
    int plies = DEFAULT_PLIES;
};

std::string cleanToken(std::string token) {
    token.erase(std::remove(token.begin(), token.end(), ';'), token.end());
    return token;
}

bool validBoardField(const std::string& field) {
    int ranks = 0;
    int file = 0;
    int kings = 0;

    for (char c : field) {
        if (c == '/') {
            if (file != 8) return false;
            file = 0;
            ++ranks;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
        } else if (c == 'K' || c == 'k') {
            ++file;
            ++kings;
        } else if (std::string("pPnNbBrRqQ").find(c) != std::string::npos) {
            ++file;
        } else {
            return false;
        }
    }

    return ranks == 7 && file == 8 && kings == 2;
}

bool validFen(const std::string fields[4]) {
    if (!validBoardField(fields[0])) return false;
    if (fields[1] != "w" && fields[1] != "b") return false;
    if (fields[2] != "-" && fields[2].find_first_not_of("KQkq") != std::string::npos) return false;
    if (fields[3] != "-") {
        if (fields[3].size() != 2) return false;
        if (fields[3][0] < 'a' || fields[3][0] > 'h' || fields[3][1] < '1' || fields[3][1] > '8') return false;
    }
    return true;
}

std::string parseFen(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> token) tokens.push_back(cleanToken(token));

    if (tokens.size() < 4 || !validFen(&tokens[0])) return "";

    int halfmove = 0;
    int fullmove = 1;

    if (tokens.size() >= 6 && tokens[4].find_first_not_of("0123456789") == std::string::npos &&
        tokens[5].find_first_not_of("0123456789") == std::string::npos) {
        halfmove = std::atoi(tokens[4].c_str());
        fullmove = std::max(1, std::atoi(tokens[5].c_str()));
    } else {
        for (size_t i = 4; i < tokens.size(); ++i) {
            if (tokens[i] == "hmvc" && i + 1 < tokens.size())
                halfmove = std::max(0, std::atoi(tokens[++i].c_str()));
            else if (tokens[i] == "fmvn" && i + 1 < tokens.size())
                fullmove = std::max(1, std::atoi(tokens[++i].c_str()));
        }
    }

    return tokens[0] + " " + tokens[1] + " " + tokens[2] + " " + tokens[3] + " " + std::to_string(halfmove) + " " +
           std::to_string(fullmove);
}

std::vector<std::string> loadBookLines(const std::string& path) {
    std::vector<std::string> lines;

    std::ifstream in(path);
    if (!in.is_open()) return lines;

    std::string line;
    while (std::getline(in, line)) {
        std::string fen = parseFen(line);
        if (!fen.empty()) lines.push_back(fen);
    }

    return lines;
}

bool parseArgs(const std::string& args, GenfensConfig& cfg) {
    std::istringstream iss(args);
    std::string token;

    iss >> token;
    if (token != "genfens") return false;

    if (!(iss >> cfg.count) || cfg.count < 1) return false;

    bool haveSeed = false;
    while (iss >> token) {
        if (token == "seed") {
            std::string value;
            if (!(iss >> value)) return false;
            char* end = nullptr;
            cfg.seed = std::strtoull(value.c_str(), &end, 10);
            if (end == value.c_str()) return false;
            haveSeed = true;
        } else if (token == "book") {
            if (!(iss >> cfg.book)) return false;
        } else if (token == "plies") {
            int plies;
            if (!(iss >> plies)) return false;
            cfg.plies = plies < 1 ? 1 : plies > MAX_PLIES ? MAX_PLIES : plies;
        }
    }

    return haveSeed;
}

}  // namespace

int runGenfens(const std::string& args) {
    GenfensConfig cfg;
    if (!parseArgs(args, cfg)) {
        fprintf(stderr, "genfens: usage: genfens <count> seed <u64> book <None|path> [plies <n>]\n");
        return 1;
    }

    std::vector<std::string> lines;
    if (cfg.book != "None") lines = loadBookLines(cfg.book);

    printf("info string found %llu book lines\n", (unsigned long long)lines.size());
    fflush(stdout);

    if (lines.empty()) lines.push_back(STARTPOS);

    const uint64_t bookOffset = (uint32_t)(cfg.seed & 0xFFFFFFFFull);

    Board board;
    std::deque<StateInfo> states(MAX_PLIES + 1);

    for (int k = 0; k < cfg.count; ++k) {
        const uint64_t index = bookOffset + (uint64_t)k;
        const std::string& start = lines[index % lines.size()];

        for (int attempt = 0;; ++attempt) {
            PRNG rng(cfg.seed ^ (0x9E3779B97F4A7C15ull * index) ^ (0xD1B54A32D192ED03ull * (uint64_t)attempt));

            int si = 0;
            board.setPos(start, &states[0]);

            for (int p = 0; p < cfg.plies; ++p) {
                MoveList moves;
                genLegalMoves(board, moves);
                if (moves.empty()) break;
                board.doMove(moves[rng.rand64() % moves.size()], states[++si]);
            }

            if (std::abs(int(evaluate(board))) >= MAX_OPENING_EVAL) continue;

            printf("info string genfens %s\n", board.fen().c_str());
            fflush(stdout);
            break;
        }
    }

    return 0;
}
