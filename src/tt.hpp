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

#include <memory>

#include "types.hpp"

struct TTEntry {
    Move move() const { return (Move)move16; }
    Value value() const { return (Value)value16; }
    Depth depth() const { return (Depth)depth8; }
    Bound bound() const { return (Bound)(genBound8 & 0x3); }
    void save(Key k, Value v, Bound b, Depth d, Move m);

private:
    friend class TTable;

    uint16_t key16;
    uint16_t move16;
    int16_t value16;
    uint8_t genBound8;
    uint8_t depth8;
};

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
    TTEntry* probe(Key k, bool& found) const;

    TTEntry* getFirstEntry(Key k) const { return &buckets[mulHi64(k, bucketsSize)].entries[0]; }

private:
    friend struct TTEntry;

    std::unique_ptr<TTBucket[]> buckets;
    size_t bucketsSize;
    uint8_t generation8;
};

extern TTable TT;

#endif  // TT_HPP
