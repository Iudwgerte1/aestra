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
#include <cstring>
#include <iostream>

#include "board.hpp"
#include "search.hpp"

Thread::Thread(size_t idx) : threadIdx(idx) {}

Thread::~Thread() {
    stopSearching();
    waitForSearchFinished();
}

void Thread::startSearching(Board& b) {
    if (stdThread.joinable()) stdThread.join();

    std::memcpy(&ownBoard, &b, sizeof(Board));
    rootBoard = &ownBoard;
    stdThread = std::thread([this]() { search(); });
}

void Thread::stopSearching() { ss.stop = true; }

void Thread::waitForSearchFinished() {
    if (stdThread.joinable()) stdThread.join();
}

void Thread::search() {
    ss.threadIdx = threadIdx;
    result = ::search(ss, *rootBoard, limits, [this](int depth, Value score) { report(depth, score); });
}

void MainThread::startSearching(Board& board) {
    for (auto& h : helpers) {
        h->limits = limits;
        h->startSearching(board);
    }
    Thread::startSearching(board);
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
    n = std::clamp(n, 1, 256);
    if (n == threadCount()) return;

    stopSearching();
    waitForSearchFinished();
    helpers.clear();
    helpers.reserve(n - 1);
    for (int i = 1; i < n; ++i) helpers.emplace_back(new Thread(i));
}

void MainThread::search() {
    Thread::search();

    std::lock_guard<std::mutex> lock(ioMutex);
    std::cout << "bestmove " << (result.bestMove != MOVE_NONE ? rootBoard->move2str(result.bestMove) : "0000")
              << std::endl;
}

void MainThread::report(int depth, Value score) { printInfo(ss, *rootBoard, depth, score); }
