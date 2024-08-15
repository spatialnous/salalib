// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaangular.h"

AnalysisResult VGAAngular::run(Communicator *comm) {
    auto &attributes = m_map.getAttributeTable();

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    std::string mean_depth_col_text = getColumnWithRadius(Column::ANGULAR_MEAN_DEPTH,    //
                                                          m_radius, m_map.getRegion());  //
    std::string total_detph_col_text = getColumnWithRadius(Column::ANGULAR_TOTAL_DEPTH,  //
                                                           m_radius, m_map.getRegion()); //
    std::string count_col_text = getColumnWithRadius(Column::ANGULAR_NODE_COUNT,         //
                                                     m_radius, m_map.getRegion());       //

    AnalysisResult result({mean_depth_col_text, total_detph_col_text, count_col_text},
                          attributes.getNumRows());

    int mean_depth_col = result.getColumnIndex(mean_depth_col_text);
    int count_col = result.getColumnIndex(count_col_text);
    int total_depth_col = result.getColumnIndex(total_detph_col_text);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, false);

    int count = 0;

    for (auto &ad0 : analysisData) {

        if (m_gates_only) {
            count++;
            continue;
        }
        for (auto &ad1 : analysisData) {
            ad1.m_visitedFromBin = 0;
            ad1.m_dist = 0.0f;
            ad1.m_cumAngle = -1.0f;
        }

        float totalAngle = 0.0f;
        int totalNodes = 0;

        std::tie(totalAngle, totalNodes) = traverseSum(analysisData, graph, refs, m_radius, ad0);

        if (totalNodes > 0) {
            result.setValue(ad0.m_attributeDataRow, mean_depth_col,
                            float(double(totalAngle) / double(totalNodes)));
        }
        result.setValue(ad0.m_attributeDataRow, total_depth_col, totalAngle);
        result.setValue(ad0.m_attributeDataRow, count_col, float(totalNodes));

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
