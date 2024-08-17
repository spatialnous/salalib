// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/simplematrix.h"

#include "salalib/pointmap.h"
#include "salalib/shapegraph.h"
#include "salalib/shapemap.h"

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
    long getColumnIndex(const std::string &column) {
        return std::distance(newAttributes.begin(),
                             std::find(newAttributes.begin(), newAttributes.end(), column));
    }

    double getValue(size_t row, size_t column) { return attributeData(row, column); }
    void setValue(size_t row, size_t column, double value) { attributeData(row, column) = value; }
    void incrValue(size_t row, size_t column, double value = 1) {
        attributeData(row, column) += value;
    }

    AnalysisResult(std::vector<std::string> &&attributeNames = std::vector<std::string>(),
                   size_t rowCount = 0, double defValue = -1.0f)
        : newAttributes(attributeNames),
          attributeData(depthmapX::RowMatrix<double>(rowCount, attributeNames.size())) {
        attributeData.initialiseValues(defValue);
    }

    depthmapX::RowMatrix<double> getAttributeData() const { return attributeData; }

    std::optional<std::vector<AttributeColumnStats>> columnStats = std::nullopt;

  protected:
    std::vector<std::string> newAttributes = std::vector<std::string>();
    depthmapX::RowMatrix<double> attributeData;
    std::vector<ShapeMap> newShapeMaps;
    std::vector<PointMap> newPointMaps;
    std::vector<ShapeGraph> newShapeGraphs;
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
        newAttributes.insert(newAttributes.end(), other.getAttributes().begin(),
                             other.getAttributes().end());
    }
};
