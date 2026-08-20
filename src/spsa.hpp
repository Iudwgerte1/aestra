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

#ifndef SPSA_HPP
#define SPSA_HPP

#include <map>
#include <string>
#include <variant>

template <typename T>
struct SPSAParam {
    T currValue;
    T min;
    T max;
    float Cend;
    float Rend;
};

typedef std::variant<SPSAParam<int>, SPSAParam<float>> SPSAParamVar;

inline std::map<std::string, SPSAParamVar> spsaParams = {
    {"lmrBias", SPSAParam<float>{0.75f, 0.0f, 1.0f, 1.0f, 0.002f}},
    {"lmrDivisor", SPSAParam<float>{2.25f, 1.0f, 5.0f, 1.0f, 0.002f}},
    {"futilityCoeff", SPSAParam<int>{150, 50, 500, 5.0f, 0.002f}},
    {"iidMargin", SPSAParam<int>{250, 100, 500, 5.0f, 0.002f}},
    {"razoringMargin", SPSAParam<int>{500, 100, 2000, 5.0f, 0.002f}},
    {"iidBias", SPSAParam<float>{2.0f, 0.0f, 10, 1.0f, 0.002f}},
    {"iidCoeff", SPSAParam<float>{0.75, 0.3f, 1.0f, 1.0f, 0.002f}},
    {"nmpBias", SPSAParam<float>{7.0f, 0, 10, 1.0f, 0.002f}},
    {"nmpCoeff", SPSAParam<float>{0.33f, 0.0f, 0.75f, 1.0f, 0.002f}}};

void printSPSAInfo();
void printSPSAUCI();
bool setSPSAParam(const std::string& name, const std::string& value);

#endif  // SPSA_HPP
