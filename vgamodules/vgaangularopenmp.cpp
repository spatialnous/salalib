// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaangularopenmp.hpp"
#if defined(_OPENMP)
#include <omp.h>
#endif

AnalysisResult VGAAngularOpenMP::run(Communicator *comm) {

#if !defined(_OPENMP)
    if (comm)
        comm->logWarning("OpenMP NOT available, only running on a single core");
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

    std::vector<AnalysisData> globalAnalysisData;
    globalAnalysisData.reserve(m_map.getAttributeTable().getNumRows());

    size_t rowCounter = 0;
    for (auto &attRow : attributes) {
        auto &point = m_map.getPoint(attRow.getKey().value);
        globalAnalysisData.push_back(AnalysisData(point, attRow.getKey().value, rowCounter, 0,
                                                  attRow.getKey().value, -1.0f, 0.0f));
        rowCounter++;
    }

    const auto refs = getRefVector(globalAnalysisData);
    const auto graph = getGraph(globalAnalysisData, refs, false);

    size_t count = 0;

    std::vector<DataPoint> colData(attributes.getNumRows());

    auto n = static_cast<int>(attributes.getNumRows());

#if defined(_OPENMP)
#pragma omp parallel for default(shared) schedule(dynamic)
#endif
    for (int i = 0; i < n; i++) {
        if (m_gatesOnly) {
#if defined(_OPENMP)
#pragma omp atomic
#endif
            count++;
            continue;
        }

        DataPoint &dp = colData[static_cast<size_t>(i)];

        std::vector<AnalysisData> analysisData;
        analysisData.reserve(m_map.getAttributeTable().getNumRows());

        size_t localRowCounter = 0;
        for (auto &attRow : attributes) {
            auto &point = m_map.getPoint(attRow.getKey().value);
            analysisData.push_back(AnalysisData(point, attRow.getKey().value, localRowCounter, 0,
                                                attRow.getKey().value, 0.0f, -1.0f));
            localRowCounter++;
        }

        float totalAngle = 0.0f;
        int totalNodes = 0;

        auto &ad0 = analysisData.at(static_cast<size_t>(i));

        std::tie(totalAngle, totalNodes) = traverseSum(analysisData, graph, refs, m_radius, ad0);

        if (totalNodes > 0) {
            dp.meanDepth = static_cast<float>(static_cast<double>(totalAngle) /
                                              static_cast<double>(totalNodes));
        }
        dp.totalDepth = totalAngle;
        dp.count = static_cast<float>(totalNodes);

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

        if (m_legacyWriteMiscs) {
            // kept to achieve parity in binary comparison with old versions
            ad0.point.dummyMisc = ad0.visitedFromBin;
            ad0.point.dummyCumangle = ad0.cumAngle;
        }
    }

    // n.b. these must be entered in alphabetical order to preserve col indexing:
    std::string meanDepthColText = getColumnWithRadius(Column::ANGULAR_MEAN_DEPTH,    //
                                                       m_radius, m_map.getRegion());  //
    std::string totalDetphColText = getColumnWithRadius(Column::ANGULAR_TOTAL_DEPTH,  //
                                                        m_radius, m_map.getRegion()); //
    std::string countColText = getColumnWithRadius(Column::ANGULAR_NODE_COUNT,        //
                                                   m_radius, m_map.getRegion());      //

    AnalysisResult result({meanDepthColText, totalDetphColText, countColText},
                          attributes.getNumRows());

    auto meanDepthCol = result.getColumnIndex(meanDepthColText.c_str());
    auto totalDepthCol = result.getColumnIndex(totalDetphColText.c_str());
    auto countCol = result.getColumnIndex(countColText.c_str());

    auto dataIter = colData.begin();
    for (size_t ridx = 0; ridx < attributes.getNumRows(); ridx++) {
        result.setValue(ridx, meanDepthCol, dataIter->meanDepth);
        result.setValue(ridx, totalDepthCol, dataIter->totalDepth);
        result.setValue(ridx, countCol, dataIter->count);
        dataIter++;
    }

    result.completed = true;

    return result;
}
