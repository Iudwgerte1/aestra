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

#ifndef THREAD_HPP
#define THREAD_HPP

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "types.hpp"
#include "search.hpp"

class Thread {
public:
    explicit Thread(size_t idx = 0);
    virtual ~Thread();

    void startSearching(Board& board);
    void stopSearching();
    void waitForSearchFinished();

    Limits limits;
    SearchResult result;

protected:
    virtual void search();
    virtual void report(int, Value) {}

    SearchState ss;
    Board* rootBoard = nullptr;

private:
    std::thread stdThread;
    Board ownBoard;
    size_t threadIdx = 0;
};

class MainThread : public Thread {
public:
    MainThread() : Thread(0) {}

    void startSearching(Board& board);
    void stopSearching();
    void waitForSearchFinished();

    void setThreads(int n);
    int threadCount() const { return 1 + (int)helpers.size(); }

    void search() override;
    void report(int depth, Value score) override;

private:
    std::vector<std::unique_ptr<Thread>> helpers;

};

#endif  // THREAD_HPP
