// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricdepth.h"

AnalysisResult VGAMetricDepth::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    AnalysisResult result({Column::METRIC_STEP_SHORTEST_PATH_ANGLE,
                           Column::METRIC_STEP_SHORTEST_PATH_LENGTH,
                           Column::METRIC_STRAIGHT_LINE_DISTANCE},
                          m_map.getFilledPointCount());

    // n.b., insert columns sets values to -1 if the column already exists
    auto pathAngleColIdx = result.getColumnIndex(Column::METRIC_STEP_SHORTEST_PATH_ANGLE);
    auto pathLengthColIdx = result.getColumnIndex(Column::METRIC_STEP_SHORTEST_PATH_LENGTH);
    std::optional<size_t> distColIdx = std::nullopt;
    if (m_originRefs.size() == 1) {
        // Note: Euclidean distance is currently only calculated from a single point
        distColIdx = result.getColumnIndex(Column::METRIC_STRAIGHT_LINE_DISTANCE);
    }

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);

    bool keepStats = true;
    AnalysisColumn pathAngleCol, pathLengthCol, euclidDistCol;
    {
        auto traversalResult = traverse(analysisData, graph, refs, -1, m_originRefs, keepStats);
        pathAngleCol = std::move(traversalResult[0]);
        pathLengthCol = std::move(traversalResult[1]);
        euclidDistCol = std::move(traversalResult[2]);
    }

    for (size_t i = 0; i < analysisData.size(); i++) {
        result.setValue(i, pathAngleColIdx, pathAngleCol.getValue(i));
        result.setValue(i, pathLengthColIdx, pathLengthCol.getValue(i));
        if (distColIdx.has_value()) {
            result.setValue(i, *distColIdx, euclidDistCol.getValue(i));
        }
    }
    result.columnStats = {pathAngleCol.getStats(), pathLengthCol.getStats(),
                          euclidDistCol.getStats()};

    result.completed = true;

    return result;
}
