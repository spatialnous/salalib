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

    std::vector<DataPoint> col_data(attributes.getNumRows());

    int i, N = int(attributes.getNumRows());

#if defined(_OPENMP)
#pragma omp parallel for
#endif
    for (i = 0; i < N; i++) {
        if ((m_map.getPoint(refs[i]).contextfilled() && !refs[i].iseven()) || (m_gatesOnly)) {
#if defined(_OPENMP)
#pragma omp critical
#endif
            count++;
            continue;
        }
        DataPoint &dp = col_data[i];

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
            double mean_depth = double(totalDepth) / double(totalNodes - 1);
            dp.depth = float(mean_depth);
            // total nodes > 2 to avoid divide by 0 (was > 3)
            if (totalNodes > 2 && mean_depth > 1.0) {
                double ra = 2.0 * (mean_depth - 1.0) / double(totalNodes - 2);
                // d-value / p-values from Depthmap 4 manual, note: node_count includes this one
                double rra_d = ra / pafmath::dvalue(totalNodes);
                double rra_p = ra / pafmath::pvalue(totalNodes);
                double integ_tk = pafmath::teklinteg(totalNodes, totalDepth);
                dp.integ_dv = float(1.0 / rra_d);
                dp.integ_pv = float(1.0 / rra_p);

                if (totalDepth - totalNodes + 1 > 1) {
                    dp.integ_tk = float(integ_tk);
                } else {
                    dp.integ_tk = -1.0f;
                }
            } else {
                dp.integ_dv = -1.0f;
                dp.integ_pv = -1.0f;
                dp.integ_tk = -1.0f;
            }
            double entropy = 0.0, rel_entropy = 0.0, factorial = 1.0;
            // n.b., this distribution contains the root node itself in distribution[0]
            // -> chopped from entropy to avoid divide by zero if only one node
            for (size_t k = 1; k < distribution.size(); k++) {
                if (distribution[k] > 0) {
                    double prob = double(distribution[k]) / double(totalNodes - 1);
                    entropy -= prob * log2(prob);
                    // Formula from Turner 2001, "Depthmap"
                    factorial *= double(k + 1);
                    double q = (pow(mean_depth, double(k)) / double(factorial)) * exp(-mean_depth);
                    rel_entropy += (float)prob * log2(prob / q);
                }
            }
            dp.entropy = float(entropy);
            dp.rel_entropy = float(rel_entropy);
        } else {
            dp.depth = -1.0f;
            dp.entropy = -1.0f;
            dp.rel_entropy = -1.0f;
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
            ad0.m_point.m_dummy_misc = ad0.m_visitedFromBin;
            ad0.m_point.m_dummy_extent = ad0.m_diagonalExtent;
        }
    }

    // n.b. these must be entered in alphabetical order to preserve col indexing:
    // dX simple version test // TV
    std::string entropy_col_text = getColumnWithRadius(Column::VISUAL_ENTROPY, m_radius);
    std::string integ_dv_col_text = getColumnWithRadius(Column::VISUAL_INTEGRATION_HH, m_radius);
    std::string integ_pv_col_text = getColumnWithRadius(Column::VISUAL_INTEGRATION_PV, m_radius);
    std::string integ_tk_col_text = getColumnWithRadius(Column::VISUAL_INTEGRATION_TK, m_radius);
    std::string depth_col_text = getColumnWithRadius(Column::VISUAL_MEAN_DEPTH, m_radius);
    std::string count_col_text = getColumnWithRadius(Column::VISUAL_NODE_COUNT, m_radius);
    std::string rel_entropy_col_text = getColumnWithRadius(Column::VISUAL_REL_ENTROPY, m_radius);

    AnalysisResult result({entropy_col_text, integ_dv_col_text, integ_pv_col_text,
                           integ_tk_col_text, depth_col_text, count_col_text, rel_entropy_col_text},
                          attributes.getNumRows());

    int entropy_col = attributes.getColumnIndex(entropy_col_text);
    int integ_dv_col = attributes.getColumnIndex(integ_dv_col_text);
    int integ_pv_col = attributes.getColumnIndex(integ_pv_col_text);
    int integ_tk_col = attributes.getColumnIndex(integ_tk_col_text);
    int depth_col = attributes.getColumnIndex(depth_col_text);
    int count_col = attributes.getColumnIndex(count_col_text);
    int rel_entropy_col = attributes.getColumnIndex(rel_entropy_col_text);

    auto dataIter = col_data.begin();
    for (size_t i = 0; i < attributes.getNumRows(); i++) {
        result.setValue(i, integ_dv_col, dataIter->integ_dv);
        result.setValue(i, integ_pv_col, dataIter->integ_pv);
        result.setValue(i, integ_tk_col, dataIter->integ_tk);
        result.setValue(i, count_col, dataIter->count);
        result.setValue(i, depth_col, dataIter->depth);
        result.setValue(i, entropy_col, dataIter->entropy);
        result.setValue(i, rel_entropy_col, dataIter->rel_entropy);
        dataIter++;
    }

    result.completed = true;

    return result;
}

void VGAVisualGlobalOpenMP::extractUnseen(Node &node, PixelRefVector &pixels,
                                          depthmapX::RowMatrix<int> &miscs,
                                          depthmapX::RowMatrix<PixelRef> &extents) {
    for (int i = 0; i < 32; i++) {
        Bin &bin = node.bin(i);
        for (auto pixVec : bin.m_pixel_vecs) {
            for (PixelRef pix = pixVec.start();
                 pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                int &misc = miscs(pix.y, pix.x);
                PixelRef &extent = extents(pix.y, pix.x);
                if (misc == 0) {
                    pixels.push_back(pix);
                    misc |= (1 << i);
                }
                // 10.2.02 revised --- diagonal was breaking this as it was extent in diagonal or
                // horizontal
                if (!(bin.m_dir & PixelRef::DIAGONAL)) {
                    if (extent.col(bin.m_dir) >= pixVec.end().col(bin.m_dir))
                        break;
                    extent.col(bin.m_dir) = pixVec.end().col(bin.m_dir);
                }
                pix.move(bin.m_dir);
            }
        }
    }
}
