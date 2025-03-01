// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricdepthlinkcost.hpp"

AnalysisResult VGAMetricDepthLinkCost::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    // custom linking costs from the attribute table
    AnalysisResult result({Column::METRIC_STEP_DEPTH}, attributes.getNumRows());

    auto pathLengthColIdx = result.getColumnIndex(Column::METRIC_STEP_DEPTH);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes, Column::LINK_METRIC_COST);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);

    AnalysisColumn pathLengthCol;
    {
        auto traversalResult = traverse(analysisData, graph, refs, -1, m_pixelsFrom);
        pathLengthCol = std::move(traversalResult[1]);
    }

    for (size_t i = 0; i < analysisData.size(); i++) {
        result.setValue(i, pathLengthColIdx, pathLengthCol.getValue(i));
    }

    result.completed = true;

    return result;
}
