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

#include "thread.hpp"

#include <algorithm>
#include <iostream>

#include "board.hpp"
#include "search.hpp"
#include "tt.hpp"

Thread::Thread(size_t idx) : threadIdx(idx) {}

Thread::~Thread() {
    stopSearching();
    waitForSearchFinished();
}

void Thread::startSearching(const std::string& fen, StateInfo* rootSi, const std::function<void(int, Value)>& report,
                            const std::function<void(const SearchResult&)>& done) {
    if (stdThread.joinable()) stdThread.join();
    ss.stop = false;

    reportCallback = report;
    doneCallback = done;

    ownBoard.setPos(fen, rootSi);
    rootBoard = &ownBoard;
    stdThread = std::thread([this]() { search(); });
}

void Thread::stopSearching() { ss.stop = true; }

void Thread::waitForSearchFinished() {
    if (stdThread.joinable()) stdThread.join();
}

void Thread::search() {
    ss.threadIdx = threadIdx;
    result = ::search(ss, *rootBoard, limits, [this](int depth, Value score) {
        if (reportCallback) reportCallback(depth, score);
    });
    if (doneCallback) doneCallback(result);
}

void MainThread::startSearching(Board& board, const std::function<void(int, Value)>& report,
                                const std::function<void(const SearchResult&)>& done) {
    TT.newSearch();

    threadStates = StateListPtr(new std::deque<StateInfo>(threadCount()));
    const std::string fen = board.fen();

    int idx = 0;
    for (auto& h : helpers) {
        h->limits = limits;
        h->startSearching(fen, &(*threadStates)[++idx], {}, {});
    }
    Thread::startSearching(fen, &(*threadStates)[0], report, done);
}

void MainThread::stopSearching() {
    Thread::stopSearching();
    for (auto& h : helpers) h->stopSearching();
}

void MainThread::waitForSearchFinished() {
    Thread::waitForSearchFinished();
    for (auto& h : helpers) h->waitForSearchFinished();
}

void MainThread::setThreads(int n) {
    n = std::clamp(n, 1, 16);
    if (n == threadCount()) return;

    stopSearching();
    waitForSearchFinished();
    helpers.clear();
    helpers.reserve(n - 1);
    for (int i = 1; i < n; ++i) helpers.emplace_back(new Thread(i));
}
