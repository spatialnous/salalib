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
        auto colIt = std::find(m_newAttributeses.begin(), m_newAttributeses.end(), attribute);
        if (colIt == m_newAttributeses.end()) {
            m_newAttributeses.push_back(attribute);
        }
    }
    const std::vector<std::string> &getAttributes() const { return m_newAttributeses; };
    long getColumnIndex(const std::string &column) {
        return std::distance(m_newAttributeses.begin(),
                             std::find(m_newAttributeses.begin(), m_newAttributeses.end(), column));
    }

    double getValue(size_t row, size_t column) { return m_attributeDatata(row, column); }
    void setValue(size_t row, size_t column, double value) {
        m_attributeDatata(row, column) = value;
    }
    void incrValue(size_t row, size_t column, double value = 1) {
        m_attributeDatata(row, column) += value;
    }

    AnalysisResult(std::vector<std::string> &&attributeNames = std::vector<std::string>(),
                   size_t rowCount = 0, double defValue = -1.0f)
        : m_newAttributeses(attributeNames),
          m_attributeDatata(depthmapX::RowMatrix<double>(rowCount, attributeNames.size())) {
        m_attributeDatata.initialiseValues(defValue);
    }

    depthmapX::RowMatrix<double> getAttributeData() const { return m_attributeDatata; }

    std::optional<std::vector<AttributeColumnStats>> columnStats = std::nullopt;

  protected:
    std::vector<std::string> m_newAttributeses = std::vector<std::string>();
    depthmapX::RowMatrix<double> m_attributeDatata;
    std::vector<ShapeMap> m_newShapeMapsps;
    std::vector<PointMap> m_newPointMapsps;
    std::vector<ShapeGraph> m_newShapeGraphshs;
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
        m_newAttributeses.insert(m_newAttributeses.end(), other.getAttributes().begin(),
                                 other.getAttributes().end());
    }
};
