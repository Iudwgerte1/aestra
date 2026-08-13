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

#ifndef TT_HPP
#define TT_HPP

#include <atomic>
#include <memory>

#include "types.hpp"

struct TTEntry {
    std::atomic<uint64_t> data;

    uint64_t load() const { return data.load(std::memory_order_relaxed); }
    void store(uint64_t v) { data.store(v, std::memory_order_relaxed); }

    void save(Key k, Value v, Bound b, Depth d, Move m);
};

static inline Move entryMove(uint64_t e) { return (Move)((e >> 16) & 0xFFFF); }
static inline Value entryValue(uint64_t e) { return (Value)((e >> 32) & 0xFFFF); }
static inline Bound entryBound(uint64_t e) { return (Bound)((e >> 48) & 0x3); }
static inline Depth entryDepth(uint64_t e) { return (Depth)((e >> 56) & 0xFF); }

inline Value valueToTT(Value v, int ply) {
    return v >= mateIn(MAX_PLY) ? Value(int(v) + ply) : v <= matedIn(MAX_PLY) ? Value(int(v) - ply) : v;
}

inline Value valueFromTT(Value v, int ply) {
    return v >= mateIn(MAX_PLY) ? Value(int(v) - ply) : v <= matedIn(MAX_PLY) ? Value(int(v) + ply) : v;
}

struct TTBucket {
    TTEntry entries[4];
};

static inline uint64_t mulHi64(uint64_t a, uint64_t b) { return (__uint128_t(a) * __uint128_t(b)) >> 64; }

class TTable {
public:
    TTable();

    void clear();
    void setSize(size_t mb);

    void newSearch() { generation8 += 4; }
    TTEntry* probe(Key k, uint64_t& entry, bool& found) const;

    TTEntry* getFirstEntry(Key k) const { return &buckets[mulHi64(k, bucketsSize)].entries[0]; }

private:
    friend struct TTEntry;

    std::unique_ptr<TTBucket[]> buckets;
    size_t bucketsSize;
    uint8_t generation8;
};

extern TTable TT;

#endif  // TT_HPP
