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
    const std::vector<std::string> &getAttributes() const { return newAttributes; };

  protected:
    std::vector<std::string> newAttributes = std::vector<std::string>();
};

struct AppendableAnalysisResult : public AnalysisResult {
    bool firstRun = true;
    void append(const AnalysisResult &other) {
        if (firstRun) {
            completed = other.completed;
            firstRun = false;
        } else {
            completed &= other.completed;
        }
        newAttributes.insert(newAttributes.end(),
                             other.getAttributes().begin(),
                             other.getAttributes().end());
    }
};
