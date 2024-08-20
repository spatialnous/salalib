// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisualglobalopenmp.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

AnalysisResult VGAVisualGlobalOpenMP::run(Communicator *comm) {

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

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_STEPS, 1);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
        comm->CommPostMessage(Communicator::NUM_RECORDS, attributes.getNumRows());
    }

    std::vector<DataPoint> colData(attributes.getNumRows());

    int i, n = int(attributes.getNumRows());

#if defined(_OPENMP)
#pragma omp parallel for
#endif
    for (i = 0; i < n; i++) {
        if ((m_map.getPoint(refs[i]).contextfilled() && !refs[i].iseven()) || (m_gatesOnly)) {
#if defined(_OPENMP)
#pragma omp atomic
#endif
            count++;
            continue;
        }
        DataPoint &dp = colData[i];

        std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
        const auto graph = getGraph(analysisData, refs, false);

        auto &ad0 = analysisData.at(i);

        auto [totalDepth, totalNodes, distribution] =
            traverseSum(analysisData, graph, refs, m_radius, ad0);

        // only set to single float precision after divide
        // note -- total_nodes includes this one -- mean depth as per p.108 Social Logic of Space

        dp.count = float(totalNodes); // note: total nodes includes this one;

        // ERROR !!!!!!
        if (totalNodes > 1) {
            double meanDepth = double(totalDepth) / double(totalNodes - 1);
            dp.depth = float(meanDepth);
            // total nodes > 2 to avoid divide by 0 (was > 3)
            if (totalNodes > 2 && meanDepth > 1.0) {
                double ra = 2.0 * (meanDepth - 1.0) / double(totalNodes - 2);
                // d-value / p-values from Depthmap 4 manual, note: node_count includes this one
                double rraD = ra / pafmath::dvalue(totalNodes);
                double rraP = ra / pafmath::pvalue(totalNodes);
                double integTk = pafmath::teklinteg(totalNodes, totalDepth);
                dp.integDv = float(1.0 / rraD);
                dp.integPv = float(1.0 / rraP);

                if (totalDepth - totalNodes + 1 > 1) {
                    dp.integTk = float(integTk);
                } else {
                    dp.integTk = -1.0f;
                }
            } else {
                dp.integDv = -1.0f;
                dp.integPv = -1.0f;
                dp.integTk = -1.0f;
            }
            double entropy = 0.0, relEntropy = 0.0, factorial = 1.0;
            // n.b., this distribution contains the root node itself in distribution[0]
            // -> chopped from entropy to avoid divide by zero if only one node
            for (size_t k = 1; k < distribution.size(); k++) {
                if (distribution[k] > 0) {
                    double prob = double(distribution[k]) / double(totalNodes - 1);
                    entropy -= prob * log2(prob);
                    // Formula from Turner 2001, "Depthmap"
                    factorial *= double(k + 1);
                    double q = (pow(meanDepth, double(k)) / double(factorial)) * exp(-meanDepth);
                    relEntropy += (float)prob * log2(prob / q);
                }
            }
            dp.entropy = float(entropy);
            dp.relEntropy = float(relEntropy);
        } else {
            dp.depth = -1.0f;
            dp.entropy = -1.0f;
            dp.relEntropy = -1.0f;
        }

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
            ad0.point.dummyExtent = ad0.diagonalExtent;
        }
    }

    // n.b. these must be entered in alphabetical order to preserve col indexing:
    // dX simple version test // TV
    std::string entropyColText = getColumnWithRadius(Column::VISUAL_ENTROPY, m_radius);
    std::string integDvColText = getColumnWithRadius(Column::VISUAL_INTEGRATION_HH, m_radius);
    std::string integPvColText = getColumnWithRadius(Column::VISUAL_INTEGRATION_PV, m_radius);
    std::string integTkColText = getColumnWithRadius(Column::VISUAL_INTEGRATION_TK, m_radius);
    std::string depthColText = getColumnWithRadius(Column::VISUAL_MEAN_DEPTH, m_radius);
    std::string countColText = getColumnWithRadius(Column::VISUAL_NODE_COUNT, m_radius);
    std::string relEntropyColText = getColumnWithRadius(Column::VISUAL_REL_ENTROPY, m_radius);

    AnalysisResult result({entropyColText, integDvColText, integPvColText, integTkColText,
                           depthColText, countColText, relEntropyColText},
                          attributes.getNumRows());

    int entropyCol = attributes.getColumnIndex(entropyColText);
    int integDvCol = attributes.getColumnIndex(integDvColText);
    int integPvCol = attributes.getColumnIndex(integPvColText);
    int integTkCol = attributes.getColumnIndex(integTkColText);
    int depthCol = attributes.getColumnIndex(depthColText);
    int countCol = attributes.getColumnIndex(countColText);
    int relEntropyCol = attributes.getColumnIndex(relEntropyColText);

    auto dataIter = colData.begin();
    for (size_t i = 0; i < attributes.getNumRows(); i++) {
        result.setValue(i, integDvCol, dataIter->integDv);
        result.setValue(i, integPvCol, dataIter->integPv);
        result.setValue(i, integTkCol, dataIter->integTk);
        result.setValue(i, countCol, dataIter->count);
        result.setValue(i, depthCol, dataIter->depth);
        result.setValue(i, entropyCol, dataIter->entropy);
        result.setValue(i, relEntropyCol, dataIter->relEntropy);
        dataIter++;
    }

    result.completed = true;

    return result;
}
