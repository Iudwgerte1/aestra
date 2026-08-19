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

#include "datagen.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "board.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "tt.hpp"
#include "types.hpp"

namespace {

constexpr int MAX_GAME_PLY = 400;

struct Adjudicator {
    int winStreak = 0;
    int drawStreak = 0;
    int outcome = 0;

    void update(Value whiteScore) {
        int s = std::abs(int(whiteScore));
        winStreak = s >= 600 ? winStreak + 1 : 0;
        drawStreak = s <= 50 ? drawStreak + 1 : 0;
        if (winStreak >= 8)
            outcome = whiteScore > 0 ? 1 : -1;
        else if (drawStreak >= 8)
            outcome = 0;
    }

    bool done() const { return winStreak >= 8 || drawStreak >= 8; }
};

struct RecordedPosition {
    std::string fen;
    Key key;
    Value whiteScore;
    Move bestMove;

    bool operator<(const RecordedPosition& other) const { return key < other.key; }
};

struct GameData {
    std::set<RecordedPosition> records;
    int outcome = 0;
};

struct ResumeInfo {
    uint64_t lines = 0;
    uint64_t truncateSize = 0;
    bool hasTail = false;
    bool hasFormat = false;
    bool isBullet = false;
};

ResumeInfo inspectOutput(const char* path) {
    ResumeInfo info;
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return info;

    in.seekg(0, std::ios::end);
    std::streamoff fileSize = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<char> buf(1 << 20);
    uint64_t bytes = 0;
    uint64_t lastNewline = 0;
    bool haveLast = false;
    std::string firstLine;
    bool collectFirstLine = true;

    while (in.read(buf.data(), buf.size()) || in.gcount() > 0) {
        std::streamsize got = in.gcount();
        for (std::streamsize i = 0; i < got; ++i) {
            char c = buf[i];
            if (collectFirstLine) {
                if (c == '\n') {
                    if (!firstLine.empty()) collectFirstLine = false;
                } else if (firstLine.size() < 256) {
                    firstLine += c;
                }
            }
            if (c == '\n') {
                lastNewline = bytes + (uint64_t)i;
                haveLast = true;
                ++info.lines;
            }
        }
        bytes += (uint64_t)got;
    }

    if (haveLast && (uint64_t)fileSize > lastNewline + 1) {
        info.hasTail = true;
        info.truncateSize = lastNewline + 1;
    }
    if (!firstLine.empty()) {
        if (firstLine.find('|') != std::string::npos) {
            info.hasFormat = true;
            info.isBullet = true;
        } else if (firstLine.find(';') != std::string::npos) {
            info.hasFormat = true;
        }
    }
    return info;
}

bool playGame(GameData& gd, Board& board, SearchState& ss, std::deque<StateInfo>& states, PRNG& rng, int nodes) {
    int si = 0;
    board.setPos(STARTPOS, &states[0]);

    for (int i = 0; i < 12; ++i) {
        MoveList moves;
        genLegalMoves(board, moves);
        if (moves.empty() || board.isDraw(0)) return false;
        board.doMove(moves[rng.rand64() % moves.size()], states[++si]);
    }

    Value we = board.turn() == WHITE ? evaluate(board) : -evaluate(board);
    if (std::abs(int(we)) > 100) return false;

    Limits limits;
    limits.nodes = nodes;

    Adjudicator adj;

    while (true) {
        MoveList moves;
        genLegalMoves(board, moves);

        if (moves.empty()) {
            gd.outcome = board.checkers(board.turn()) ? (board.turn() == WHITE ? -1 : 1) : 0;
            break;
        }
        if (board.isDraw(0) || board.Ply() >= MAX_GAME_PLY) {
            gd.outcome = 0;
            break;
        }

        ss.stop = false;
        SearchResult result = search(ss, board, limits, [](int, Value) {});

        Value whiteScore = board.turn() == WHITE ? result.score : -result.score;
        Move bestMove = result.bestMove != MOVE_NONE ? result.bestMove : moves[0];

        gd.records.insert({board.fen(), board.key(), whiteScore, bestMove});

        if (board.Ply() >= 40) {
            adj.update(whiteScore);
            if (adj.done()) {
                gd.outcome = adj.outcome;
                break;
            }
        }

        board.doMove(bestMove, states[++si]);
    }

    return true;
}

std::vector<RecordedPosition> filterRecords(const std::set<RecordedPosition>& records, Board& scratch, StateInfo& s0,
                                            StateInfo& s1) {
    std::vector<RecordedPosition> kept;
    kept.reserve(records.size());

    for (const RecordedPosition& r : records) {
        scratch.setPos(r.fen, &s0);

        if (scratch.checkers(scratch.turn())) continue;
        if (std::abs(int(r.whiteScore)) >= VALUE_MATE - MAX_PLY) continue;

        Move m = r.bestMove;
        if (scratch.pieceOn(toSq(m)) != NO_PIECE || moveType(m) == EN_PASSANT) continue;

        scratch.doMove(m, s1);
        if (scratch.checkers(scratch.turn())) continue;

        kept.push_back(r);
    }

    return kept;
}

std::string formatLine(const RecordedPosition& r, int outcome, bool bullet) {
    if (bullet)
        return r.fen + " | " + std::to_string(int(r.whiteScore)) + " | " +
               (outcome > 0   ? "1.0"
                : outcome < 0 ? "0.0"
                              : "0.5");
    return r.fen + "; [" + (outcome > 0 ? "1-0" : outcome < 0 ? "0-1" : "1/2-1/2") + "]";
}

void worker(int idx, int nodes, int target, bool bullet, std::atomic<uint64_t>& total, std::atomic<uint64_t>& games,
            std::atomic<int>& active, std::ofstream& out) {
    PRNG rng(0x9E3779B97F4A7C15ull ^ (uint64_t(idx) * 0xD1B54A32D192ED03ull) ^
             uint64_t(std::chrono::steady_clock::now().time_since_epoch().count()));

    SearchState ss;
    Board board;
    std::deque<StateInfo> states(512);
    Board scratch;
    StateInfo s0, s1;

    while (total.load(std::memory_order_relaxed) < (uint64_t)target) {
        GameData gd;
        if (!playGame(gd, board, ss, states, rng, nodes)) continue;

        std::vector<RecordedPosition> kept = filterRecords(gd.records, scratch, s0, s1);

        std::vector<std::string> lines;
        lines.reserve(kept.size());
        for (const RecordedPosition& r : kept) lines.push_back(formatLine(r, gd.outcome, bullet));

        {
            std::lock_guard<std::mutex> lock(ioMutex);
            uint64_t remaining = (uint64_t)target - total.load(std::memory_order_relaxed);
            size_t n = lines.size() < remaining ? lines.size() : (size_t)remaining;
            for (size_t i = 0; i < n; ++i) out << lines[i] << "\n";
            total.fetch_add(n, std::memory_order_relaxed);
        }
        games.fetch_add(1, std::memory_order_relaxed);
    }

    active.fetch_sub(1, std::memory_order_relaxed);
}

}  // namespace

void datagen(int threads, int nodes, int target, bool bullet_format, bool resume) {
    threads = std::max(1, threads);

    TT.clear();

    uint64_t existing = 0;
    if (resume) {
        ResumeInfo info = inspectOutput("output.txt");
        if (info.lines >= (uint64_t)target) {
            fprintf(stderr, "datagen: output.txt already has %llu lines >= target %d; nothing to do\n",
                    (unsigned long long)info.lines, target);
            return;
        }
        if (info.hasFormat && info.isBullet != bullet_format) {
            fprintf(stderr, "datagen: --resume: output.txt format (%s) does not match requested format (%s)\n",
                    info.isBullet ? "bullet" : "plain", bullet_format ? "bullet" : "plain");
            exit(1);
        }
        if (info.hasTail) {
            try {
                std::filesystem::resize_file("output.txt", info.truncateSize);
            } catch (const std::filesystem::filesystem_error& e) {
                fprintf(stderr, "datagen: --resume: cannot truncate partial line in output.txt: %s\n", e.what());
                exit(1);
            }
        }
        existing = info.lines;
    }

    std::ofstream out("output.txt", std::ios::binary | (resume ? std::ios::app : std::ios::out));
    if (!out.is_open()) {
        fprintf(stderr, "datagen: cannot open output.txt for writing\n");
        exit(1);
    }

    std::atomic<uint64_t> total{existing};
    std::atomic<uint64_t> games{0};
    std::atomic<int> active{threads};

    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (int i = 0; i < threads; ++i)
        workers.emplace_back(worker, i, nodes, target, bullet_format, std::ref(total), std::ref(games),
                             std::ref(active), std::ref(out));

    auto start = std::chrono::steady_clock::now();

    const uint64_t REPORT_INTERVAL = 100000;
    uint64_t nextMilestone = (existing / REPORT_INTERVAL + 1) * REPORT_INTERVAL;

    while (active.load(std::memory_order_relaxed) > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        uint64_t t = total.load(std::memory_order_relaxed);
        if (t >= nextMilestone) {
            uint64_t secs =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
            uint64_t g = games.load(std::memory_order_relaxed);
            int left = target - t;
            int left_time = left * secs / (t - existing);
            printf("%llu/%d positions (%.1f%%) | %llu games | %.0f pos/s\n", nextMilestone, target, 100.0 * t / target,
                   (unsigned long long)g, secs && t > existing ? 1.0 * (t - existing) / secs : 0.0);
            printf("Estimated remaining time: %d h %d m %d s\n", left_time / 3600, left_time % 3600 / 60,
                   left_time % 60);
            nextMilestone += REPORT_INTERVAL;
        }
    }

    for (std::thread& th : workers) th.join();

    uint64_t secs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
    int elapsed = int(secs);
    printf("datagen complete\n");
    printf("positions written: %llu / %d\n", (unsigned long long)total.load(), target);
    printf("games played:      %llu\n", (unsigned long long)games.load());
    printf("elapsed:           %02d:%02d:%02d\n", elapsed / 3600, elapsed % 3600 / 60, elapsed % 60);
    printf("throughput:        %.0f pos/s\n",
           secs && total.load() > existing ? 1.0 * (total.load() - existing) / secs : 0.0);
    printf("threads: %d  nodes/move: %d  format: %s\n", threads, nodes, bullet_format ? "bullet" : "plain");
    out.flush();
}
