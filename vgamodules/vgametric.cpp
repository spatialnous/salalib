// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametric.hpp"

AnalysisResult VGAMetric::run(Communicator *comm) {

    auto &attributes = m_map.getAttributeTable();

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS,
                              static_cast<size_t>(m_map.getFilledPointCount()));
    }

    std::string mspaColText = getColumnWithRadius(Column::METRIC_MEAN_SHORTEST_PATH_ANGLE,    //
                                                  m_radius, m_map.getRegion());               //
    std::string msplColText = getColumnWithRadius(Column::METRIC_MEAN_SHORTEST_PATH_DISTANCE, //
                                                  m_radius, m_map.getRegion());               //
    std::string distColText = getColumnWithRadius(Column::METRIC_MEAN_STRAIGHT_LINE_DISTANCE, //
                                                  m_radius, m_map.getRegion());               //
    std::string countColText = getColumnWithRadius(Column::METRIC_NODE_COUNT,                 //
                                                   m_radius, m_map.getRegion());              //

    AnalysisResult result({mspaColText, msplColText, distColText, countColText},
                          attributes.getNumRows());

    auto mspaCol = result.getColumnIndex(mspaColText);
    auto msplCol = result.getColumnIndex(msplColText);
    auto distCol = result.getColumnIndex(distColText);
    auto countCol = result.getColumnIndex(countColText);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, false);

    size_t count = 0;
    for (auto &ad0 : analysisData) {
        if (m_gatesOnly) {
            count++;
            continue;
        }

        for (auto &ad2 : analysisData) {
            ad2.visitedFromBin = 0;
            ad2.dist = -1.0f;
            ad2.cumAngle = 0.0f;
        }

        auto [totalDepth, totalAngle, euclidDepth, totalNodes] =
            traverseSum(analysisData, graph, refs, m_radius, ad0);

        result.setValue(ad0.attributeDataRow, mspaCol, //
                        static_cast<float>(static_cast<double>(totalAngle) /
                                           static_cast<double>(totalNodes))); //
        result.setValue(ad0.attributeDataRow, msplCol,                        //
                        static_cast<float>(static_cast<double>(totalDepth) /
                                           static_cast<double>(totalNodes))); //
        result.setValue(ad0.attributeDataRow, distCol,                        //
                        static_cast<float>(static_cast<double>(euclidDepth) /
                                           static_cast<double>(totalNodes))); //
        result.setValue(ad0.attributeDataRow, countCol,                       //
                        static_cast<float>(totalNodes));                      //

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
