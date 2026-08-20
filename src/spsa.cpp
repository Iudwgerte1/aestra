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

// SPSA tuning interface: prints the OpenBench SPSA Input text and UCI
// option lines for every tunable parameter, and applies setoption values.

#include "spsa.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

#include "search.hpp"

void printSPSAInfo() {
    for (const auto& [name, param] : spsaParams) {
        std::cout << name << ", ";
        if (std::holds_alternative<SPSAParam<int>>(param)) {
            const auto& p = std::get<SPSAParam<int>>(param);
            std::cout << "int, " << p.currValue << ", " << p.min << ", " << p.max << ", " << p.Cend << ", " << p.Rend
                      << std::endl;
        } else {
            const auto& p = std::get<SPSAParam<float>>(param);
            std::cout << "float, " << p.currValue << ", " << p.min << ", " << p.max << ", " << p.Cend << ", " << p.Rend
                      << std::endl;
        }
    }
}

void printSPSAUCI() {
    for (const auto& [name, param] : spsaParams) {
        if (std::holds_alternative<SPSAParam<int>>(param)) {
            const auto& p = std::get<SPSAParam<int>>(param);
            std::cout << "option name " << name << " type spin default " << p.currValue << " min " << p.min << " max "
                      << p.max << std::endl;
        } else {
            const auto& p = std::get<SPSAParam<float>>(param);
            std::cout << "option name " << name << " type string default " << p.currValue << std::endl;
        }
    }
}

bool setSPSAParam(const std::string& name, const std::string& value) {
    auto it = spsaParams.find(name);
    if (it == spsaParams.end()) return false;

    try {
        if (std::holds_alternative<SPSAParam<int>>(it->second)) {
            auto& p = std::get<SPSAParam<int>>(it->second);
            p.currValue = std::clamp(std::stoi(value), p.min, p.max);
        } else {
            auto& p = std::get<SPSAParam<float>>(it->second);
            p.currValue = std::clamp(std::stof(value), p.min, p.max);
        }
    } catch (const std::exception&) {
        return false;
    }

    initSearch();
    return true;
}
