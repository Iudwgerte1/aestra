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

#include "tt.hpp"

#include <algorithm>
#include <cstring>

static_assert(sizeof(TTEntry) == 8, "TTEntry size must be 8 bytes");
static_assert(sizeof(TTBucket) == 32, "TTBucket size must be 32 bytes");

TTable TT;

void TTEntry::save(Key k, Value v, Bound b, Depth d, Move m) {
    const uint64_t e = load();
    const uint16_t key16 = (uint16_t)k;
    uint64_t newE = e;

    if (m || key16 != (uint16_t)e) newE = (newE & ~0xFFFF0000ull) | (uint64_t(uint16_t(m)) << 16);

    if (key16 != (uint16_t)e || d > entryDepth(e) || b == BOUND_EXACT)
        newE = (newE & 0xFFFF0000ull) | key16 | (uint64_t(uint16_t(v)) << 32) |
               (uint64_t(uint8_t(TT.generation8 | b)) << 48) | (uint64_t(uint8_t(d)) << 56);

    store(newE);
}

TTable::TTable() { setSize(16); }

void TTable::clear() {
    if (buckets) memset(buckets.get(), 0, bucketsSize * sizeof(TTBucket));
}

void TTable::setSize(size_t mb) {
    size_t entries = mb * 1024 * 1024 / sizeof(TTEntry);
    while (entries & (entries - 1)) entries &= entries - 1;
    entries = std::max<size_t>(entries, 1024);

    buckets.reset(new TTBucket[entries / 4]);
    bucketsSize = entries / 4;
}

TTEntry* TTable::probe(Key k, uint64_t& entry, bool& found) const {
    TTEntry* const tte = getFirstEntry(k);
    const uint16_t key16 = (uint16_t)k;

    for (int i = 0; i < 4; ++i) {
        entry = tte[i].load();
        if (!(uint16_t)entry || (uint16_t)entry == key16) return found = (bool)(uint16_t)entry, &tte[i];
    }

    TTEntry* replace = tte;
    uint64_t replaceEntry = tte[0].load();
    for (int i = 0; i < 4; ++i) {
        uint64_t e = tte[i].load();
        if (entryDepth(replaceEntry) - ((259 + generation8 - ((replaceEntry >> 48) & 0xFF)) & 0xFC) >
            entryDepth(e) - ((259 + generation8 - ((e >> 48) & 0xFF)) & 0xFC))
            replace = &tte[i], replaceEntry = e;
    }

    return found = false, replace;
}
