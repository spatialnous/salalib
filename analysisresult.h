// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <string>
#include <vector>

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
