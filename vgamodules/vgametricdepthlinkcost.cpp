// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricdepthlinkcost.h"

AnalysisResult VGAMetricDepthLinkCost::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    // custom linking costs from the attribute table
    std::string linkMetricCostColName = Column::LINK_METRIC_COST;
    std::string pathLengthColName = Column::METRIC_STEP_DEPTH;
    AnalysisResult result({pathLengthColName}, attributes.getNumRows());

    int pathLengthColIdx = result.getColumnIndex(pathLengthColName);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes, linkMetricCostColName);
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
