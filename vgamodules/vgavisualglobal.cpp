// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisualglobal.hpp"

std::vector<std::string> VGAVisualGlobal::getColumns(bool simpleVersion) const {

    std::vector<std::string> columns;
    // n.b. these must be entered in alphabetical order to preserve col indexing:
    // dX simple version test // TV
    if (!simpleVersion) {
        columns.push_back(getColumnWithRadius(Column::VISUAL_ENTROPY, m_radius));
    }

    columns.push_back(getColumnWithRadius(Column::VISUAL_INTEGRATION_HH, m_radius));

    if (!simpleVersion) {
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
        comm->CommPostMessage(Communicator::NUM_RECORDS,
                              static_cast<size_t>(m_map.getFilledPointCount()));
    }

    std::optional<size_t> entropyCol = std::nullopt, relEntropyCol = std::nullopt,
                          integDvCol = std::nullopt, integPvCol = std::nullopt,
                          integTkCol = std::nullopt, depthCol = std::nullopt,
                          countCol = std::nullopt;

    AnalysisResult result(getColumns(m_simpleVersion), attributes.getNumRows());

    integDvCol = result.getColumnIndex(getColumnWithRadius(        //
        Column::VISUAL_INTEGRATION_HH, m_radius));                 //
    if (!m_simpleVersion) {                                        //
        entropyCol = result.getColumnIndex(getColumnWithRadius(    //
            Column::VISUAL_ENTROPY, m_radius));                    //
        integPvCol = result.getColumnIndex(getColumnWithRadius(    //
            Column::VISUAL_INTEGRATION_PV, m_radius));             //
        integTkCol = result.getColumnIndex(getColumnWithRadius(    //
            Column::VISUAL_INTEGRATION_TK, m_radius));             //
        depthCol = result.getColumnIndex(getColumnWithRadius(      //
            Column::VISUAL_MEAN_DEPTH, m_radius));                 //
        countCol = result.getColumnIndex(getColumnWithRadius(      //
            Column::VISUAL_NODE_COUNT, m_radius));                 //
        relEntropyCol = result.getColumnIndex(getColumnWithRadius( //
            Column::VISUAL_REL_ENTROPY, m_radius));                //
    }

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);

    size_t count = 0;

    for (auto &ad0 : analysisData) {
        if ((ad0.point.contextfilled() && !ad0.ref.iseven()) || (m_gatesOnly)) {
            count++;
            continue;
        }
        for (auto &ad2 : analysisData) {
            ad2.visitedFromBin = 0;
            ad2.diagonalExtent = ad2.ref;
        }

        auto [totalDepth, totalNodes, distribution] =
            traverseSum(analysisData, graph, refs, m_radius, ad0);
        // only set to single float precision after divide
        // note -- total_nodes includes this one -- mean depth as per p.108 Social Logic of
        // Space
        if (!m_simpleVersion) {
            result.setValue(ad0.attributeDataRow, countCol.value(),
                            static_cast<float>(totalNodes)); // note: total nodes includes this one
        }
        // ERROR !!!!!!
        if (totalNodes > 1) {
            double meanDepth =
                static_cast<double>(totalDepth) / static_cast<double>(totalNodes - 1);
            if (!m_simpleVersion) {
                result.setValue(ad0.attributeDataRow, depthCol.value(),
                                static_cast<float>(meanDepth));
            }
            // total nodes > 2 to avoid divide by 0 (was > 3)
            if (totalNodes > 2 && meanDepth > 1.0) {
                double ra = 2.0 * (meanDepth - 1.0) / static_cast<double>(totalNodes - 2);
                // d-value / p-values from Depthmap 4 manual, note: node_count includes this
                // one
                double rraD = ra / pafmath::dvalue(totalNodes);
                double rraP = ra / pafmath::pvalue(totalNodes);
                double integTk = pafmath::teklinteg(totalNodes, totalDepth);
                result.setValue(ad0.attributeDataRow, integDvCol.value(),
                                static_cast<float>(1.0 / rraD));
                if (!m_simpleVersion) {
                    result.setValue(ad0.attributeDataRow, integPvCol.value(),
                                    static_cast<float>(1.0 / rraP));
                }
                if (totalDepth - totalNodes + 1 > 1) {
                    if (!m_simpleVersion) {
                        result.setValue(ad0.attributeDataRow, integTkCol.value(),
                                        static_cast<float>(integTk));
                    }
                } else {
                    if (!m_simpleVersion) {
                        result.setValue(ad0.attributeDataRow, integTkCol.value(), -1.0f);
                    }
                }
            } else {
                result.setValue(ad0.attributeDataRow, integDvCol.value(), -1.0f);
                if (!m_simpleVersion) {
                    result.setValue(ad0.attributeDataRow, integPvCol.value(), -1.0f);
                    result.setValue(ad0.attributeDataRow, integTkCol.value(), -1.0f);
                }
            }
            double entropy = 0.0, relEntropy = 0.0, factorial = 1.0;
            // n.b., this distribution contains the root node itself in distribution[0]
            // -> chopped from entropy to avoid divide by zero if only one node
            for (size_t k = 1; k < distribution.size(); k++) {
                if (distribution[k] > 0) {
                    double prob =
                        static_cast<double>(distribution[k]) / static_cast<double>(totalNodes - 1);
                    entropy -= prob * pafmath::log2(prob);
                    // Formula from Turner 2001, "Depthmap"
                    factorial *= static_cast<double>(k + 1);
                    double q =
                        (pow(meanDepth, static_cast<double>(k)) / static_cast<double>(factorial)) *
                        exp(-meanDepth);
                    relEntropy += static_cast<float>(prob) * pafmath::log2(prob / q);
                }
            }
            if (!m_simpleVersion) {
                result.setValue(ad0.attributeDataRow, entropyCol.value(),
                                static_cast<float>(entropy));
                result.setValue(ad0.attributeDataRow, relEntropyCol.value(),
                                static_cast<float>(relEntropy));
            }
        } else {
            if (!m_simpleVersion) {
                result.setValue(ad0.attributeDataRow, depthCol.value(), -1.0f);
                result.setValue(ad0.attributeDataRow, entropyCol.value(), -1.0f);
                result.setValue(ad0.attributeDataRow, relEntropyCol.value(), -1.0f);
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
            ad2.point.dummyMisc = ad2.visitedFromBin;
            ad2.point.dummyExtent = ad2.diagonalExtent;
        }
    }

    result.completed = true;

    return result;
}
