// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricopenmp.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

AnalysisResult VGAMetricOpenMP::run(Communicator *comm) {

#if !defined(_OPENMP)
    std::cerr << "OpenMP NOT available, only running on a single core" << std::endl;
    m_forceCommUpdatesMasterThread = false;
#else
    if (m_limitToThreads.has_value()) {
        omp_set_num_threads(m_limitToThreads.value());
    }
#endif

    auto &attributes = m_map.getAttributeTable();

    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    const auto refs = getRefVector(attributes);

    int count = 0;

    std::vector<DataPoint> colData(attributes.getNumRows());

    int i, n = int(attributes.getNumRows());

#if defined(_OPENMP)
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
#endif
    for (i = 0; i < n; i++) {
        if (m_gatesOnly) {
#if defined(_OPENMP)
#pragma omp critical
#endif
            count++;
            continue;
        }

        DataPoint &dp = colData[i];

        std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
        const auto graph = getGraph(analysisData, refs, false);

        auto &ad0 = analysisData.at(i);

        auto [totalDepth, totalAngle, euclidDepth, totalNodes] =
            traverseSum(analysisData, graph, refs, m_radius, ad0);

        if (m_legacyWriteMiscs) {
            // kept to achieve parity in binary comparison with old versions
            ad0.m_point.dummyMisc = ad0.m_visitedFromBin;
            ad0.m_point.dummyDist = ad0.m_dist;
            ad0.m_point.dummyCumangle = ad0.m_cumAngle;
        }

        dp.m_mspa = float(double(totalAngle) / double(totalNodes));
        dp.m_mspl = float(double(totalDepth) / double(totalNodes));
        dp.m_dist = float(double(euclidDepth) / double(totalNodes));
        dp.m_count = float(totalNodes);

#if defined(_OPENMP)
#pragma omp atomic
#endif
        count++; // <- increment count

#if defined(_OPENMP)
        // only executed by the main thread if requested
        if (!m_forceCommUpdatesMasterThread || omp_get_thread_num() == 0)
#endif
            if (comm) {
                if (qtimer(atime, 500)) {
                    if (comm->IsCancelled()) {
                        throw Communicator::CancelledException();
                    }
                    comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
                }
            }
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

    int mspaCol = result.getColumnIndex(mspaColText);
    int msplCol = result.getColumnIndex(msplColText);
    int distCol = result.getColumnIndex(distColText);
    int countCol = result.getColumnIndex(countColText);

    auto dataIter = colData.begin();
    for (size_t i = 0; i < attributes.getNumRows(); i++) {
        result.setValue(i, mspaCol, dataIter->m_mspa);
        result.setValue(i, msplCol, dataIter->m_mspl);
        result.setValue(i, distCol, dataIter->m_dist);
        result.setValue(i, countCol, dataIter->m_count);
        dataIter++;
    }

    result.completed = true;

    return result;
}
