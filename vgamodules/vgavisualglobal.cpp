// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisualglobal.h"

AnalysisResult VGAVisualGlobal::run(Communicator *comm, PointMap &map, bool simple_version) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }

    AnalysisResult result;

    AttributeTable &attributes = map.getAttributeTable();

    std::optional<size_t> entropy_col = std::nullopt, rel_entropy_col = std::nullopt,
                          integ_dv_col = std::nullopt, integ_pv_col = std::nullopt,
                          integ_tk_col = std::nullopt, depth_col = std::nullopt,
                          count_col = std::nullopt;

    // n.b. these must be entered in alphabetical order to preserve col indexing:
    // dX simple version test // TV
#ifndef _COMPILE_dX_SIMPLE_VERSION
    if (!simple_version) {
        std::string entropy_col_text = getColumnWithRadius(Column::VISUAL_ENTROPY, m_radius);
        entropy_col = attributes.insertOrResetColumn(entropy_col_text.c_str());
        result.addAttribute(entropy_col_text);
    }
#endif

    std::string integ_dv_col_text = getColumnWithRadius(Column::VISUAL_INTEGRATION_HH, m_radius);
    integ_dv_col = attributes.insertOrResetColumn(integ_dv_col_text.c_str());
    result.addAttribute(integ_dv_col_text);

#ifndef _COMPILE_dX_SIMPLE_VERSION
    if (!simple_version) {
        std::string integ_pv_col_text =
            getColumnWithRadius(Column::VISUAL_INTEGRATION_PV, m_radius);
        integ_pv_col = attributes.insertOrResetColumn(integ_pv_col_text.c_str());
        result.addAttribute(integ_pv_col_text);
        std::string integ_tk_col_text =
            getColumnWithRadius(Column::VISUAL_INTEGRATION_TK, m_radius);
        integ_tk_col = attributes.insertOrResetColumn(integ_tk_col_text.c_str());
        result.addAttribute(integ_tk_col_text);
        std::string depth_col_text = getColumnWithRadius(Column::VISUAL_MEAN_DEPTH, m_radius);
        depth_col = attributes.insertOrResetColumn(depth_col_text.c_str());
        result.addAttribute(depth_col_text);
        std::string count_col_text = getColumnWithRadius(Column::VISUAL_NODE_COUNT, m_radius);
        count_col = attributes.insertOrResetColumn(count_col_text.c_str());
        result.addAttribute(count_col_text);
        std::string rel_entropy_col_text =
            getColumnWithRadius(Column::VISUAL_REL_ENTROPY, m_radius);
        rel_entropy_col = attributes.insertOrResetColumn(rel_entropy_col_text.c_str());
        result.addAttribute(rel_entropy_col_text);
    }
#endif

    size_t count = 0;

    depthmapX::RowMatrix<int> miscs(map.getRows(), map.getCols());
    depthmapX::RowMatrix<PixelRef> extents(map.getRows(), map.getCols());

    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (map.getPoint(curs).filled()) {

                if ((map.getPoint(curs).contextfilled() && !curs.iseven()) || (m_gates_only)) {
                    count++;
                    continue;
                }

                for (size_t ii = 0; ii < map.getCols(); ii++) {
                    for (size_t jj = 0; jj < map.getRows(); jj++) {
                        miscs(jj, ii) = 0;
                        extents(jj, ii) = PixelRef(static_cast<short>(ii), static_cast<short>(jj));
                    }
                }

                int total_depth = 0;
                int total_nodes = 0;

                std::vector<int> distribution;
                std::vector<PixelRefVector> search_tree;
                search_tree.push_back(PixelRefVector());
                search_tree.back().push_back(curs);

                size_t level = 0;
                while (search_tree[level].size()) {
                    search_tree.push_back(PixelRefVector());
                    const PixelRefVector &searchTreeAtLevel = search_tree[level];
                    distribution.push_back(0);
                    for (auto currLvlIter = searchTreeAtLevel.rbegin();
                         currLvlIter != searchTreeAtLevel.rend(); currLvlIter++) {
                        int &pmisc = miscs(currLvlIter->y, currLvlIter->x);
                        Point &p = map.getPoint(*currLvlIter);
                        if (p.filled() && pmisc != ~0) {
                            total_depth += level;
                            total_nodes += 1;
                            distribution.back() += 1;
                            if ((int)m_radius == -1 ||
                                (level < static_cast<size_t>(m_radius) &&
                                 (!p.contextfilled() || currLvlIter->iseven()))) {
                                extractUnseen(p.getNode(), search_tree[level + 1], miscs, extents);
                                pmisc = ~0;
                                if (!p.getMergePixel().empty()) {
                                    PixelRef mergePixel = p.getMergePixel();
                                    int &p2misc = miscs(mergePixel.y, mergePixel.x);
                                    Point &p2 = map.getPoint(mergePixel);
                                    if (p2misc != ~0) {
                                        extractUnseen(p2.getNode(), search_tree[level + 1], miscs,
                                                      extents); // did say p.misc
                                        p2misc = ~0;
                                    }
                                }
                            } else {
                                pmisc = ~0;
                            }
                        }
                        search_tree[level].pop_back();
                    }
                    level++;
                }
                AttributeRow &row = attributes.getRow(AttributeKey(curs));
                // only set to single float precision after divide
                // note -- total_nodes includes this one -- mean depth as per p.108 Social Logic of
                // Space
                if (!simple_version) {
                    row.setValue(count_col.value(),
                                 float(total_nodes)); // note: total nodes includes this one
                }
                // ERROR !!!!!!
                if (total_nodes > 1) {
                    double mean_depth = double(total_depth) / double(total_nodes - 1);
                    if (!simple_version) {
                        row.setValue(depth_col.value(), float(mean_depth));
                    }
                    // total nodes > 2 to avoid divide by 0 (was > 3)
                    if (total_nodes > 2 && mean_depth > 1.0) {
                        double ra = 2.0 * (mean_depth - 1.0) / double(total_nodes - 2);
                        // d-value / p-values from Depthmap 4 manual, note: node_count includes this
                        // one
                        double rra_d = ra / pafmath::dvalue(total_nodes);
                        double rra_p = ra / pafmath::pvalue(total_nodes);
                        double integ_tk = pafmath::teklinteg(total_nodes, total_depth);
                        row.setValue(integ_dv_col.value(), float(1.0 / rra_d));
                        if (!simple_version) {
                            row.setValue(integ_pv_col.value(), float(1.0 / rra_p));
                        }
                        if (total_depth - total_nodes + 1 > 1) {
                            if (!simple_version) {
                                row.setValue(integ_tk_col.value(), float(integ_tk));
                            }
                        } else {
                            if (!simple_version) {
                                row.setValue(integ_tk_col.value(), -1.0f);
                            }
                        }
                    } else {
                        row.setValue(integ_dv_col.value(), (float)-1);
                        if (!simple_version) {
                            row.setValue(integ_pv_col.value(), (float)-1);
                            row.setValue(integ_tk_col.value(), (float)-1);
                        }
                    }
                    double entropy = 0.0, rel_entropy = 0.0, factorial = 1.0;
                    // n.b., this distribution contains the root node itself in distribution[0]
                    // -> chopped from entropy to avoid divide by zero if only one node
                    for (size_t k = 1; k < distribution.size(); k++) {
                        if (distribution[k] > 0) {
                            double prob = double(distribution[k]) / double(total_nodes - 1);
                            entropy -= prob * pafmath::log2(prob);
                            // Formula from Turner 2001, "Depthmap"
                            factorial *= double(k + 1);
                            double q =
                                (pow(mean_depth, double(k)) / double(factorial)) * exp(-mean_depth);
                            rel_entropy += (float)prob * pafmath::log2(prob / q);
                        }
                    }
                    if (!simple_version) {
                        row.setValue(entropy_col.value(), float(entropy));
                        row.setValue(rel_entropy_col.value(), float(rel_entropy));
                    }
                } else {
                    if (!simple_version) {
                        row.setValue(depth_col.value(), (float)-1);
                        row.setValue(entropy_col.value(), (float)-1);
                        row.setValue(rel_entropy_col.value(), (float)-1);
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
        }
    }
    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            map.getPoint(curs).m_misc = miscs(j, i);
            map.getPoint(curs).m_extent = extents(j, i);
        }
    }

    result.completed = true;

    return result;
}

void VGAVisualGlobal::extractUnseen(Node &node, PixelRefVector &pixels,
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
