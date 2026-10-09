// SPDX-FileCopyrightText: 2018-2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface to handle different kinds of VGA analysis

#include "../ianalysis.hpp"
#include "../latticemap.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class IVGA : public IAnalysis {
  protected:
    const LatticeMap &m_map;

  protected:
    struct AnalysisData {
        const Point &point;
        size_t attributeDataRow;
        const PixelRef ref;
        int visitedFromBin = 0;

        float dist = 0.0f;
        float cumAngle = 0.0f;
        float linkCost = 0.0f;

      private:
        [[maybe_unused]] unsigned _padding0 : 4 * 8;

      public:
        AnalysisData(const Point &pointIn, const PixelRef refIn, size_t attributeDataRowIn,
                     int visitedFromBinIn, float distIn, float cumAngleIn)
            : point(pointIn), attributeDataRow(attributeDataRowIn), ref(refIn),
              visitedFromBin(visitedFromBinIn), dist(distIn), cumAngle(cumAngleIn), _padding0(0) {}
    };

  protected:
    struct ADRef {
        uint32_t idx;
        int bin;
    };
    using ADRefVector = std::vector<ADRef>;

    std::vector<PixelRef> getRefVector(const AttributeTable &attributes) const {
        std::vector<PixelRef> refs;
        refs.reserve(attributes.getNumRows());
        for (auto &row : attributes) {
            refs.push_back(row.getKey().value);
        }
        return refs;
    }

    std::optional<size_t> getRefIdxOptional(const std::vector<PixelRef> &refs,
                                            const PixelRef ref) const {
        auto it = std::find(refs.begin(), refs.end(), ref);
        if (it == refs.end())
            return std::nullopt;
        return static_cast<size_t>(std::distance(refs.begin(), it));
    }

  public:
    IVGA(const LatticeMap &map) : m_map(map) {}

    virtual void
    copyResultToMap(const std::vector<std::string> &colNames,
                    const genlib::RowMatrix<double> &colValues, LatticeMap &map,
                    std::optional<std::vector<AttributeColumnStats>> columnStats = std::nullopt) {
        AttributeTable &attributes = map.getAttributeTable();

        for (const auto &colName : colNames) {
            attributes.insertOrResetColumn(colName);
        }
        std::vector<size_t> newColIndxs(colNames.size());
        auto colIdxIt = newColIndxs.begin();
        auto colNameIt = colNames.begin();
        for (; colNameIt != colNames.end(); colNameIt++, colIdxIt++) {
            *colIdxIt = attributes.getColumnIndex(*colNameIt);
        }
        auto colValuesIt = colValues.begin();
        auto rowIt = attributes.begin();
        for (; rowIt != attributes.end(); rowIt++) {
            colIdxIt = newColIndxs.begin();
            for (; colIdxIt != newColIndxs.end(); colIdxIt++, colValuesIt++) {
                rowIt->getRow().setValue(*colIdxIt, static_cast<float>(std::move(*colValuesIt)));
            }
        }
        if (columnStats.has_value()) {

            auto newColIdxIt = newColIndxs.begin();
            auto colStatsIt = columnStats->begin();
            for (; newColIdxIt != newColIndxs.end(); newColIdxIt++, colStatsIt++) {
                attributes.getColumn(*newColIdxIt).setStats(*colStatsIt);
            }
        }
    }
};
