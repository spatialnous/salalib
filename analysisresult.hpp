// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/simplematrix.hpp"

#include "pointmap.hpp"
#include "shapegraph.hpp"
#include "shapemap.hpp"

#include <algorithm>
#include <string>
#include <vector>

struct AnalysisResult {
    bool completed = false;

  private:
    [[maybe_unused]] unsigned _padding0 : 3 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  public:
    std::optional<std::vector<AttributeColumnStats>> columnStats = std::nullopt;

    void addAttribute(std::string attribute) {
        auto colIt = std::find(m_newAttributes.begin(), m_newAttributes.end(), attribute);
        if (colIt == m_newAttributes.end()) {
            m_newAttributes.push_back(attribute);
        }
    }
    const std::vector<std::string> &getAttributes() const { return m_newAttributes; }
    size_t getColumnIndex(const std::string &column) {
        auto iter = std::find(m_newAttributes.begin(), m_newAttributes.end(), column);
        if (iter == m_newAttributes.end()) {
            std::stringstream message;
            message << "Unknown column name " << column;
            throw std::out_of_range(message.str());
        }
        return static_cast<size_t>(std::distance(m_newAttributes.begin(), iter));
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
        : _padding0(0), _padding1(0), m_newAttributes(attributeNames),
          m_attributeDatata(depthmapX::RowMatrix<double>(rowCount, attributeNames.size())),
          m_newShapeMaps(), m_newPointMaps(), m_newShapeGraphs() {
        m_attributeDatata.initialiseValues(defValue);
    }

    depthmapX::RowMatrix<double> getAttributeData() const { return m_attributeDatata; }

  protected:
    std::vector<std::string> m_newAttributes = std::vector<std::string>();
    depthmapX::RowMatrix<double> m_attributeDatata;
    std::vector<ShapeMap> m_newShapeMaps;
    std::vector<PointMap> m_newPointMaps;
    std::vector<ShapeGraph> m_newShapeGraphs;
};

struct AppendableAnalysisResult : public AnalysisResult {
    bool firstRun = true;

  private:
    [[maybe_unused]] unsigned _padding0 : 3 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  public:
    void append(const AnalysisResult &other) {
        if (firstRun) {
            completed = other.completed;
            firstRun = false;
        } else {
            completed &= other.completed;
        }
        m_newAttributes.insert(m_newAttributes.end(), other.getAttributes().begin(),
                               other.getAttributes().end());
    }
};
