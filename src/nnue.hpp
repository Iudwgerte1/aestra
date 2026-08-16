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

#ifndef NNUE_HPP
#define NNUE_HPP

#include <algorithm>
#include <array>
#include <string>

#include "types.hpp"

class Board;

inline constexpr size_t INPUT_SIZE = 768;
inline constexpr size_t HIDDEN_SIZE = 128;

inline constexpr int16_t QA = 255;
inline constexpr int16_t QB = 64;
inline constexpr int16_t QAB = QA * QB;

inline constexpr Value SCALE = Value(400);

inline const std::string NETWORK_PATH = "mulberry.bin";

constexpr int32_t screlu(int16_t x) {
    const int32_t y = std::clamp<int32_t>(x, 0, QA);
    return y * y;
}

struct alignas(64) Accumulator {
    std::array<int16_t, HIDDEN_SIZE> vals;

    void init();

    void addFeature(size_t idx);

    void removeFeature(size_t idx);
};

struct Network {
    std::array<int16_t, INPUT_SIZE * HIDDEN_SIZE> featureWeights;
    std::array<int16_t, HIDDEN_SIZE> featureBias;
    std::array<int16_t, 2 * HIDDEN_SIZE> outputWeights;
    int16_t outputBias;

    Value evaluate(const Accumulator& us, const Accumulator& them) const {
        int32_t output = 0;
        for (size_t i = 0; i < HIDDEN_SIZE; ++i) output += screlu(us.vals[i]) * static_cast<int32_t>(outputWeights[i]);
        for (size_t i = 0; i < HIDDEN_SIZE; ++i)
            output += screlu(them.vals[i]) * static_cast<int32_t>(outputWeights[HIDDEN_SIZE + i]);
        output /= QA;
        output += outputBias;
        output *= SCALE;
        output /= QAB;
        return Value(output);
    }
};

extern Network nnueParams;

inline void Accumulator::init() { vals = nnueParams.featureBias; }

inline void Accumulator::addFeature(size_t idx) {
    for (size_t i = 0; i < HIDDEN_SIZE; ++i) vals[i] += nnueParams.featureWeights[idx * HIDDEN_SIZE + i];
}

inline void Accumulator::removeFeature(size_t idx) {
    for (size_t i = 0; i < HIDDEN_SIZE; ++i) vals[i] -= nnueParams.featureWeights[idx * HIDDEN_SIZE + i];
}

void initNNUE();

#endif  // NNUE_HPP
