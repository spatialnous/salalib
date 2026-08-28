// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2026 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricdepth.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>
#include <utility>

AnalysisResult VGAMetricDepth::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    AnalysisResult result({Column::METRIC_STEP_PENN_DISTANCE,
                           Column::METRIC_STEP_SHORTEST_PATH_ANGLE,
                           Column::METRIC_STEP_SHORTEST_PATH_LENGTH,
                           Column::METRIC_STRAIGHT_LINE_DISTANCE},
                          static_cast<size_t>(m_map.getFilledPointCount()));

    // n.b., insert columns sets values to -1 if the column already exists
    auto pathAngleColIdx = result.getColumnIndex(Column::METRIC_STEP_SHORTEST_PATH_ANGLE);
    auto pathLengthColIdx = result.getColumnIndex(Column::METRIC_STEP_SHORTEST_PATH_LENGTH);
    std::optional<size_t> pennDistColIdx = std::nullopt;
    std::optional<size_t> distColIdx = std::nullopt;
    if (m_originRefs.size() == 1) {
        // Note: Euclidean distance is currently only calculated from a single point
        pennDistColIdx = result.getColumnIndex(Column::METRIC_STEP_PENN_DISTANCE);
        distColIdx = result.getColumnIndex(Column::METRIC_STRAIGHT_LINE_DISTANCE);
    }

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);

    bool keepStats = true;
    AnalysisColumn pathAngleCol, pathLengthCol, euclidDistCol, pennDistCol;
    {
        auto traversalResult = traverse(
            analysisData, graph, refs, -1, m_originRefs, keepStats);
        pathAngleCol = std::move(traversalResult[0]);
        pathLengthCol = std::move(traversalResult[1]);
        euclidDistCol = std::move(traversalResult[2]);
        if (m_originRefs.size() == 1) {
            pennDistCol = AnalysisColumn(analysisData.size(), 0);
            for (size_t i = 0; i < analysisData.size(); i++) {
                pennDistCol.setValue(
                    i, std::max(0.0f, pathLengthCol.getValue(i) -
                        euclidDistCol.getValue(i)), true);
            }
        }
    }

    for (size_t i = 0; i < analysisData.size(); i++) {
        result.setValue(i, pathAngleColIdx, pathAngleCol.getValue(i));
        result.setValue(i, pathLengthColIdx, pathLengthCol.getValue(i));
        if (m_originRefs.size() == 1) {
            result.setValue(i, *distColIdx, euclidDistCol.getValue(i));
            result.setValue(i, *pennDistColIdx, pennDistCol.getValue(i));
        }
    }
    result.columnStats = {pennDistCol.getStats(), pathAngleCol.getStats(),
                          pathLengthCol.getStats(), euclidDistCol.getStats()};

    result.completed = true;

    return result;
}
