// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisualglobaldepth.h"

AnalysisResult VGAVisualGlobalDepth::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    AnalysisResult result({Column::VISUAL_STEP_DEPTH}, attributes.getNumRows());

    // n.b., insert columns sets values to -1 if the column already exists
    auto colIdx = result.getColumnIndex(Column::VISUAL_STEP_DEPTH);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);

    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, false);

    auto sdCol = traverse(analysisData, graph, refs, -1.0f, m_originRefs)[0];

    for (size_t i = 0; i < analysisData.size(); i++) {
        result.setValue(i, colIdx, sdCol.getValue(i));
    }

    result.completed = true;
    return result;
}
