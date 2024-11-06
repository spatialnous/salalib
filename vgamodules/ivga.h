// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface to handle different kinds of VGA analysis

#include "../ianalysis.h"
#include "../pointmap.h"

#include <numeric>
#include <string>

class IVGA : public IAnalysis {
  protected:
    const PointMap &m_map;

  protected:
    struct AnalysisData {
        const Point &point;
        const PixelRef ref;
        size_t attributeDataRow;
        int visitedFromBin = 0;

        // used to speed up graph analysis (not sure whether or not it breaks it!)
        PixelRef diagonalExtent;

        float dist = 0.0f;
        float cumAngle = 0.0f;
        float linkCost = 0.0f;
        AnalysisData(const Point &point, const PixelRef ref, size_t attributeDataRow,
                     int visitedFromBin, PixelRef diagonalExtent, float dist, float cumAngle)
            : point(point), ref(ref), attributeDataRow(attributeDataRow),
              visitedFromBin(visitedFromBin), diagonalExtent(diagonalExtent), dist(dist),
              cumAngle(cumAngle) {}
    };

  protected:
    template <class T> using ADRefVector = std::vector<std::tuple<std::reference_wrapper<T>, int>>;

    template <class T>
    std::vector<PixelRef> getRefVector(const std::vector<T> &analysisData) const {
        std::vector<PixelRef> refs;
        refs.reserve(analysisData.size());
        for (auto &ad : analysisData) {
            refs.push_back(ad.ref);
        }
        return refs;
    }

    std::vector<PixelRef> getRefVector(const AttributeTable &attributes) const {
        std::vector<PixelRef> refs;
        refs.reserve(attributes.getNumRows());
        for (auto &row : attributes) {
            refs.push_back(row.getKey().value);
        }
        return refs;
    }

    size_t getRefIdx(const std::vector<PixelRef> &refs, const PixelRef ref) const {
        auto it = std::find(refs.begin(), refs.end(), ref);
        if (it == refs.end())
            throw std::out_of_range("Ref " + std::to_string(ref) + " not in refs");
        return static_cast<size_t>(std::distance(refs.begin(), it));
    }

    std::optional<size_t> getRefIdxOptional(const std::vector<PixelRef> &refs,
                                            const PixelRef ref) const {
        auto it = std::find(refs.begin(), refs.end(), ref);
        if (it == refs.end())
            return std::nullopt;
        return static_cast<size_t>(std::distance(refs.begin(), it));
    }

  public:
    IVGA(const PointMap &map) : m_map(map) {}

    virtual void
    copyResultToMap(const std::vector<std::string> &colNames,
                    const depthmapX::RowMatrix<double> &colValues, PointMap &map,
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

            auto colIdxIt = newColIndxs.begin();
            auto colStatsIt = columnStats->begin();
            for (; colIdxIt != newColIndxs.end(); colIdxIt++, colStatsIt++) {
                attributes.getColumn(*colIdxIt).setStats(*colStatsIt);
            }
        }
    }
};
