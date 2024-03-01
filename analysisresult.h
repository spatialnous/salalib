// Copyright (C) 2024 Petros Koutsolampros

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <vector>
#include <string>
#include <algorithm>

struct AnalysisResult {
    bool completed = false;
    void addAttribute(std::string attribute) {
        auto colIt = std::find(newAttributes.begin(), newAttributes.end(), attribute);
        if (colIt == newAttributes.end()) {
            newAttributes.push_back(attribute);
        }
    }
    std::vector<std::string> &getAttributes() { return newAttributes; };
private:
    std::vector<std::string> newAttributes = std::vector<std::string>();
};
