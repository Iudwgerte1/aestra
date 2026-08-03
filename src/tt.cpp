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
    if (m || (uint16_t)k != key16) move16 = (uint16_t)m;
    if ((uint16_t)k != key16 || d > depth8 || b == BOUND_EXACT) {
        key16 = (uint16_t)k;
        value16 = (int16_t)v;
        genBound8 = (uint8_t)(TT.generation8 | b);
        depth8 = (uint8_t)d;
    }
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

TTEntry* TTable::probe(Key k, bool& found) const {
    TTEntry* const tte = getFirstEntry(k);
    const uint16_t key16 = (uint16_t)k;

    for (int i = 0; i < 4; ++i)
        if (!tte[i].key16 || tte[i].key16 == key16) return found = (bool)tte[i].key16, &tte[i];

    TTEntry* replace = tte;
    for (int i = 0; i < 4; ++i)
        if (replace->depth8 - ((259 + generation8 - replace->genBound8) & 0xFC) >
            tte[i].depth8 - ((259 + generation8 - tte[i].genBound8) & 0xFC))
            replace = &tte[i];

    return found = false, replace;
}
