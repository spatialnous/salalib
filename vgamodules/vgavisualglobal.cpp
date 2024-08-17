// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisualglobal.h"

std::vector<std::string> VGAVisualGlobal::getColumns(bool simple_version) const {

    std::vector<std::string> columns;
    // n.b. these must be entered in alphabetical order to preserve col indexing:
    // dX simple version test // TV
    if (!simple_version) {
        columns.push_back(getColumnWithRadius(Column::VISUAL_ENTROPY, m_radius));
    }

    columns.push_back(getColumnWithRadius(Column::VISUAL_INTEGRATION_HH, m_radius));

    if (!simple_version) {
        columns.push_back(getColumnWithRadius(Column::VISUAL_INTEGRATION_PV, m_radius));
        columns.push_back(getColumnWithRadius(Column::VISUAL_INTEGRATION_TK, m_radius));
        columns.push_back(getColumnWithRadius(Column::VISUAL_MEAN_DEPTH, m_radius));
        columns.push_back(getColumnWithRadius(Column::VISUAL_NODE_COUNT, m_radius));
        columns.push_back(getColumnWithRadius(Column::VISUAL_REL_ENTROPY, m_radius));
    }
    return columns;
}

AnalysisResult VGAVisualGlobal::run(Communicator *comm) {
    auto &attributes = m_map.getAttributeTable();

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    std::optional<size_t> entropy_col = std::nullopt, rel_entropy_col = std::nullopt,
                          integ_dv_col = std::nullopt, integ_pv_col = std::nullopt,
                          integ_tk_col = std::nullopt, depth_col = std::nullopt,
                          count_col = std::nullopt;

    AnalysisResult result(getColumns(m_simpleVersion), attributes.getNumRows());

    integ_dv_col = result.getColumnIndex(getColumnWithRadius(        //
        Column::VISUAL_INTEGRATION_HH, m_radius));                   //
    if (!m_simpleVersion) {                                          //
        entropy_col = result.getColumnIndex(getColumnWithRadius(     //
            Column::VISUAL_ENTROPY, m_radius));                      //
        integ_pv_col = result.getColumnIndex(getColumnWithRadius(    //
            Column::VISUAL_INTEGRATION_PV, m_radius));               //
        integ_tk_col = result.getColumnIndex(getColumnWithRadius(    //
            Column::VISUAL_INTEGRATION_TK, m_radius));               //
        depth_col = result.getColumnIndex(getColumnWithRadius(       //
            Column::VISUAL_MEAN_DEPTH, m_radius));                   //
        count_col = result.getColumnIndex(getColumnWithRadius(       //
            Column::VISUAL_NODE_COUNT, m_radius));                   //
        rel_entropy_col = result.getColumnIndex(getColumnWithRadius( //
            Column::VISUAL_REL_ENTROPY, m_radius));                  //
    }

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);

    size_t count = 0;

    for (auto &ad0 : analysisData) {
        if ((ad0.m_point.contextfilled() && !ad0.m_ref.iseven()) || (m_gates_only)) {
            count++;
            continue;
        }
        for (auto &ad2 : analysisData) {
            ad2.m_visitedFromBin = 0;
            ad2.m_diagonalExtent = ad2.m_ref;
        }

        auto [totalDepth, totalNodes, distribution] =
            traverseSum(analysisData, graph, refs, m_radius, ad0);
        // only set to single float precision after divide
        // note -- total_nodes includes this one -- mean depth as per p.108 Social Logic of
        // Space
        if (!m_simpleVersion) {
            result.setValue(ad0.m_attributeDataRow, count_col.value(),
                            float(totalNodes)); // note: total nodes includes this one
        }
        // ERROR !!!!!!
        if (totalNodes > 1) {
            double mean_depth = double(totalDepth) / double(totalNodes - 1);
            if (!m_simpleVersion) {
                result.setValue(ad0.m_attributeDataRow, depth_col.value(), float(mean_depth));
            }
            // total nodes > 2 to avoid divide by 0 (was > 3)
            if (totalNodes > 2 && mean_depth > 1.0) {
                double ra = 2.0 * (mean_depth - 1.0) / double(totalNodes - 2);
                // d-value / p-values from Depthmap 4 manual, note: node_count includes this
                // one
                double rra_d = ra / pafmath::dvalue(totalNodes);
                double rra_p = ra / pafmath::pvalue(totalNodes);
                double integ_tk = pafmath::teklinteg(totalNodes, totalDepth);
                result.setValue(ad0.m_attributeDataRow, integ_dv_col.value(), float(1.0 / rra_d));
                if (!m_simpleVersion) {
                    result.setValue(ad0.m_attributeDataRow, integ_pv_col.value(),
                                    float(1.0 / rra_p));
                }
                if (totalDepth - totalNodes + 1 > 1) {
                    if (!m_simpleVersion) {
                        result.setValue(ad0.m_attributeDataRow, integ_tk_col.value(),
                                        float(integ_tk));
                    }
                } else {
                    if (!m_simpleVersion) {
                        result.setValue(ad0.m_attributeDataRow, integ_tk_col.value(), -1.0f);
                    }
                }
            } else {
                result.setValue(ad0.m_attributeDataRow, integ_dv_col.value(), (float)-1);
                if (!m_simpleVersion) {
                    result.setValue(ad0.m_attributeDataRow, integ_pv_col.value(), (float)-1);
                    result.setValue(ad0.m_attributeDataRow, integ_tk_col.value(), (float)-1);
                }
            }
            double entropy = 0.0, rel_entropy = 0.0, factorial = 1.0;
            // n.b., this distribution contains the root node itself in distribution[0]
            // -> chopped from entropy to avoid divide by zero if only one node
            for (size_t k = 1; k < distribution.size(); k++) {
                if (distribution[k] > 0) {
                    double prob = double(distribution[k]) / double(totalNodes - 1);
                    entropy -= prob * pafmath::log2(prob);
                    // Formula from Turner 2001, "Depthmap"
                    factorial *= double(k + 1);
                    double q = (pow(mean_depth, double(k)) / double(factorial)) * exp(-mean_depth);
                    rel_entropy += (float)prob * pafmath::log2(prob / q);
                }
            }
            if (!m_simpleVersion) {
                result.setValue(ad0.m_attributeDataRow, entropy_col.value(), float(entropy));
                result.setValue(ad0.m_attributeDataRow, rel_entropy_col.value(),
                                float(rel_entropy));
            }
        } else {
            if (!m_simpleVersion) {
                result.setValue(ad0.m_attributeDataRow, depth_col.value(), (float)-1);
                result.setValue(ad0.m_attributeDataRow, entropy_col.value(), (float)-1);
                result.setValue(ad0.m_attributeDataRow, rel_entropy_col.value(), (float)-1);
            }
        }
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

    if (m_legacyWriteMiscs) {
        // kept to achieve parity in binary comparison with old versions
        for (auto &ad2 : analysisData) {
            ad2.m_point.m_dummy_misc = ad2.m_visitedFromBin;
            ad2.m_point.m_dummy_extent = ad2.m_diagonalExtent;
        }
    }

    result.completed = true;

    return result;
}
