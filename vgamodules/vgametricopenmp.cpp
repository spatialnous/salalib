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
        comm->CommPostMessage(Communicator::NUM_RECORDS,
                              static_cast<size_t>(m_map.getFilledPointCount()));
    }

    const auto refs = getRefVector(attributes);

    size_t count = 0;

    std::vector<DataPoint> colData(attributes.getNumRows());

    int i, n = int(attributes.getNumRows());

#if defined(_OPENMP)
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
#endif
    for (i = 0; i < n; i++) {
        if (m_gatesOnly) {
#if defined(_OPENMP)
#pragma omp atomic
#endif
            count++;
            continue;
        }

        auto ui = static_cast<size_t>(i);

        DataPoint &dp = colData[ui];

        std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
        const auto graph = getGraph(analysisData, refs, false);

        auto &ad0 = analysisData.at(ui);

        auto [totalDepth, totalAngle, euclidDepth, totalNodes] =
            traverseSum(analysisData, graph, refs, m_radius, ad0);

        if (m_legacyWriteMiscs) {
            // kept to achieve parity in binary comparison with old versions
            ad0.point.dummyMisc = ad0.visitedFromBin;
            ad0.point.dummyDist = ad0.dist;
            ad0.point.dummyCumangle = ad0.cumAngle;
        }

        dp.mspa = float(double(totalAngle) / double(totalNodes));
        dp.mspl = float(double(totalDepth) / double(totalNodes));
        dp.dist = float(double(euclidDepth) / double(totalNodes));
        dp.count = float(totalNodes);

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

    auto mspaCol = result.getColumnIndex(mspaColText);
    auto msplCol = result.getColumnIndex(msplColText);
    auto distCol = result.getColumnIndex(distColText);
    auto countCol = result.getColumnIndex(countColText);

    auto dataIter = colData.begin();
    for (size_t ridx = 0; ridx < attributes.getNumRows(); ridx++) {
        result.setValue(ridx, mspaCol, dataIter->mspa);
        result.setValue(ridx, msplCol, dataIter->mspl);
        result.setValue(ridx, distCol, dataIter->dist);
        result.setValue(ridx, countCol, dataIter->count);
        dataIter++;
    }

    result.completed = true;

    return result;
}
