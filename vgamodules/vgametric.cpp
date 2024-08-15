// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametric.h"

AnalysisResult VGAMetric::run(Communicator *comm) {

    auto &attributes = m_map.getAttributeTable();

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    std::string mspa_col_text = getColumnWithRadius(Column::METRIC_MEAN_SHORTEST_PATH_ANGLE,    //
                                                    m_radius, m_map.getRegion());               //
    std::string mspl_col_text = getColumnWithRadius(Column::METRIC_MEAN_SHORTEST_PATH_DISTANCE, //
                                                    m_radius, m_map.getRegion());               //
    std::string dist_col_text = getColumnWithRadius(Column::METRIC_MEAN_STRAIGHT_LINE_DISTANCE, //
                                                    m_radius, m_map.getRegion());               //
    std::string count_col_text = getColumnWithRadius(Column::METRIC_NODE_COUNT,                 //
                                                     m_radius, m_map.getRegion());              //

    AnalysisResult result({mspa_col_text, mspl_col_text, dist_col_text, count_col_text},
                          attributes.getNumRows());

    int mspa_col = result.getColumnIndex(mspa_col_text);
    int mspl_col = result.getColumnIndex(mspl_col_text);
    int dist_col = result.getColumnIndex(dist_col_text);
    int count_col = result.getColumnIndex(count_col_text);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, false);

    size_t count = 0;
    for (auto &ad0 : analysisData) {
        if (m_gates_only) {
            count++;
            continue;
        }

        for (auto &ad2 : analysisData) {
            ad2.m_visitedFromBin = 0;
            ad2.m_dist = -1.0f;
            ad2.m_cumAngle = 0.0f;
        }

        auto [totalDepth, totalAngle, euclidDepth, totalNodes] =
            traverseSum(analysisData, graph, refs, m_radius, ad0);

        result.setValue(ad0.m_attributeDataRow, mspa_col,                 //
                        float(double(totalAngle) / double(totalNodes)));  //
        result.setValue(ad0.m_attributeDataRow, mspl_col,                 //
                        float(double(totalDepth) / double(totalNodes)));  //
        result.setValue(ad0.m_attributeDataRow, dist_col,                 //
                        float(double(euclidDepth) / double(totalNodes))); //
        result.setValue(ad0.m_attributeDataRow, count_col,                //
                        float(totalNodes));                               //

        count++; // <- increment count

        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
            }
        }
    }

    result.completed = true;

    return result;
}
