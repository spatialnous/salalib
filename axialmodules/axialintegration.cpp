// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "axialintegration.h"

#include "genlib/pflipper.h"

std::vector<std::string> AxialIntegration::getRequiredColumns(std::vector<int> radii,
                                                              std::string weightingColName,
                                                              bool simple_version) {
    std::vector<std::string> newColumns;
    for (int radius : radii) {
        if (!m_forceLegacyColumnOrder) {
            // Columns that are always created
            newColumns.push_back(getFormattedColumn( //
                Column::MEAN_DEPTH, radius));
            newColumns.push_back(getFormattedColumn( //
                Column::NODE_COUNT, radius));
            newColumns.push_back(getFormattedColumn( //
                Column::INTEGRATION, radius, std::nullopt, Normalisation::HH));

            if (m_weighted_measure_col != -1) {
                newColumns.push_back(getFormattedColumn( //
                    Column::MEAN_DEPTH, radius, weightingColName));
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL, radius, weightingColName));
            }

            if (!simple_version) {
                // columns only when simple-version is not selected
                newColumns.push_back(getFormattedColumn( //
                    Column::ENTROPY, radius));
                newColumns.push_back(getFormattedColumn( //
                    Column::INTEGRATION, radius, std::nullopt, Normalisation::PV));
                newColumns.push_back(getFormattedColumn( //
                    Column::INTEGRATION, radius, std::nullopt, Normalisation::TK));
                newColumns.push_back(getFormattedColumn( //
                    Column::INTENSITY, radius));
                newColumns.push_back(getFormattedColumn( //
                    Column::HARMONIC_MEAN_DEPTH, radius));
                newColumns.push_back(getFormattedColumn( //
                    Column::RELATIVISED_ENTROPY, radius));
            }

            if (m_choice) {
                // Columns that are only created when choice is selected
                newColumns.push_back(getFormattedColumn( //
                    Column::CHOICE, radius));
                newColumns.push_back(getFormattedColumn( //
                    Column::CHOICE, radius, std::nullopt, Normalisation::NORM));
                if (m_weighted_measure_col != -1) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, radius, weightingColName));
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, radius, weightingColName, Normalisation::NORM));
                }
            }

            if (m_fulloutput) {
                newColumns.push_back(getFormattedColumn( //
                    Column::RA, radius));
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL_DEPTH, radius));

                if (!simple_version) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::RA, radius, std::nullopt, Normalisation::PENN));
                    newColumns.push_back(getFormattedColumn( //
                        Column::RRA, radius));
                }
            }
        } else {
            // This is the legacy order of columns, required for binary
            // compatibility with older versions of sala.

            if (m_choice) {
                // Columns that are only created when choice is selected
                newColumns.push_back(getFormattedColumn( //
                    Column::CHOICE, radius));
                newColumns.push_back(getFormattedColumn( //
                    Column::CHOICE, radius, std::nullopt, Normalisation::NORM));
                if (m_weighted_measure_col != -1) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, radius, weightingColName));
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, radius, weightingColName, Normalisation::NORM));
                }
            }

            if (!simple_version) {
                // columns only when simple-version is not selected
                auto formattedCol = getFormattedColumn( //
                    Column::ENTROPY, radius);
                auto fomCol = formattedCol.c_str();
                newColumns.push_back(fomCol);
            }

            // Columns that are always created
            newColumns.push_back(getFormattedColumn( //
                Column::INTEGRATION, radius, std::nullopt, Normalisation::HH));

            if (!simple_version) {
                // columns only when simple-version is not selected
                newColumns.push_back(getFormattedColumn( //
                    Column::INTEGRATION, radius, std::nullopt, Normalisation::PV));
                newColumns.push_back(getFormattedColumn( //
                    Column::INTEGRATION, radius, std::nullopt, Normalisation::TK));
                newColumns.push_back(getFormattedColumn( //
                    Column::INTENSITY, radius));
                newColumns.push_back(getFormattedColumn( //
                    Column::HARMONIC_MEAN_DEPTH, radius));
            }

            // Columns that are always created
            newColumns.push_back(getFormattedColumn( //
                Column::MEAN_DEPTH, radius));
            newColumns.push_back(getFormattedColumn( //
                Column::NODE_COUNT, radius));

            if (!simple_version) {
                // columns only when simple-version is not selected
                newColumns.push_back(getFormattedColumn( //
                    Column::RELATIVISED_ENTROPY, radius));
            }

            if (m_weighted_measure_col != -1) {
                newColumns.push_back(getFormattedColumn( //
                    Column::MEAN_DEPTH, radius, weightingColName));
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL, radius, weightingColName));
            }

            if (m_fulloutput) {
                if (!simple_version) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::RA, radius, std::nullopt, Normalisation::PENN));
                }
                newColumns.push_back(getFormattedColumn( //
                    Column::RA, radius));

                if (!simple_version) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::RRA, radius));
                }

                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL_DEPTH, radius));
            }
        }
    }
    return newColumns;
}

std::vector<int> AxialIntegration::getFormattedRadii(std::set<double> radiusSet) {
    // note: radius must be sorted lowest to highest, but if -1 occurs ("radius n") it needs to be
    // last...
    // ...to ensure no mess ups, we'll re-sort here:
    bool radius_n = false;
    std::vector<int> radii;
    for (double radius : radiusSet) {
        if (radius < 0) {
            radius_n = true;
        } else {
            radii.push_back(static_cast<int>(radius));
        }
    }
    if (radius_n) {
        radii.push_back(-1);
    }
    return radii;
}

AnalysisResult AxialIntegration::run(Communicator *comm, ShapeGraph &map, bool simple_version) {
    // note, from 10.0, Depthmap no longer includes *self* connections on axial lines
    // self connections are stripped out on loading graph files, as well as no longer made

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getShapeCount());
    }

    AnalysisResult result;

    AttributeTable &attributes = map.getAttributeTable();

    std::vector<int> radii = getFormattedRadii(m_radius_set);

    // retrieve weighted col data, as this may well be overwritten in the new analysis:
    std::vector<double> weights;
    std::string weighting_col_text;
    if (m_weighted_measure_col != -1) {
        weighting_col_text = attributes.getColumnName(m_weighted_measure_col);
        for (size_t i = 0; i < map.getShapeCount(); i++) {
            weights.push_back(
                map.getAttributeRowFromShapeIndex(i).getValue(m_weighted_measure_col));
        }
    }

    // first enter the required attribute columns:
    auto newColumns = getRequiredColumns(radii, weighting_col_text, simple_version);
    for (auto &col : newColumns) {
        attributes.insertOrResetColumn(col);
        result.addAttribute(col);
    }

    // then look up all the columns... eek:
    std::vector<size_t> choice_col, n_choice_col, w_choice_col, nw_choice_col, entropy_col,
        integ_dv_col, integ_pv_col, integ_tk_col, intensity_col, depth_col, count_col,
        rel_entropy_col, penn_norm_col, w_depth_col, total_weight_col, ra_col, rra_col, td_col,
        harmonic_col;
    for (int radius : radii) {
        std::string radius_text;

        if (m_choice) {
            choice_col.push_back(getFormattedColumnIdx( //
                attributes, Column::CHOICE, radius));
            n_choice_col.push_back(getFormattedColumnIdx( //
                attributes, Column::CHOICE, radius, std::nullopt, Normalisation::NORM));
            if (m_weighted_measure_col != -1) {
                w_choice_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::CHOICE, radius, weighting_col_text));
                nw_choice_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::CHOICE, radius, weighting_col_text, Normalisation::NORM));
            }
        }
        if (!simple_version) {
            entropy_col.push_back(getFormattedColumnIdx( //
                attributes, Column::ENTROPY, radius));
        }

        integ_dv_col.push_back(getFormattedColumnIdx( //
            attributes, Column::INTEGRATION, radius, std::nullopt, Normalisation::HH));

        if (!simple_version) {
            integ_pv_col.push_back(getFormattedColumnIdx( //
                attributes, Column::INTEGRATION, radius, std::nullopt, Normalisation::PV));
            integ_tk_col.push_back(getFormattedColumnIdx( //
                attributes, Column::INTEGRATION, radius, std::nullopt, Normalisation::TK));
            intensity_col.push_back(getFormattedColumnIdx( //
                attributes, Column::INTENSITY, radius));
            harmonic_col.push_back(getFormattedColumnIdx( //
                attributes, Column::HARMONIC_MEAN_DEPTH, radius));
        }

        depth_col.push_back(getFormattedColumnIdx( //
            attributes, Column::MEAN_DEPTH, radius));
        count_col.push_back(getFormattedColumnIdx( //
            attributes, Column::NODE_COUNT, radius));

        if (!simple_version) {
            rel_entropy_col.push_back(getFormattedColumnIdx( //
                attributes, Column::RELATIVISED_ENTROPY, radius));
        }

        if (m_weighted_measure_col != -1) {
            w_depth_col.push_back(getFormattedColumnIdx( //
                attributes, Column::MEAN_DEPTH, radius, weighting_col_text, std::nullopt));
            total_weight_col.push_back(getFormattedColumnIdx( //
                attributes, Column::TOTAL, radius, weighting_col_text, std::nullopt));
        }
        if (m_fulloutput) {
            ra_col.push_back(getFormattedColumnIdx( //
                attributes, Column::RA, radius, std::nullopt, std::nullopt));

            if (!simple_version) {
                penn_norm_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::RA, radius, std::nullopt, Normalisation::PENN));
                rra_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::RRA, radius, std::nullopt, std::nullopt));
            }
            td_col.push_back(getFormattedColumnIdx( //
                attributes, Column::TOTAL_DEPTH, radius, std::nullopt, std::nullopt));
        }
    }

    // for choice
    AnalysisInfo **audittrail = nullptr;
    if (m_choice) {
        audittrail = new AnalysisInfo *[map.getShapeCount()];
        for (size_t i = 0; i < map.getShapeCount(); i++) {
            audittrail[i] = new AnalysisInfo[radii.size()];
        }
    }

    // n.b., for this operation we assume continuous line referencing from zero (this is silly?)
    // has already failed due to this!  when intro hand drawn fewest line (where user may have
    // deleted) it's going to get worse...

    bool *covered = new bool[map.getShapeCount()];

    size_t i = 0;
    for (auto &iter : attributes) {
        AttributeRow &row = iter.getRow();
        for (size_t j = 0; j < map.getShapeCount(); j++) {
            covered[j] = false;
        }
        if (m_choice) {
            for (size_t k = 0; k < map.getShapeCount(); k++) {
                audittrail[k][0].previous.ref =
                    -1; // note, 0th member used as radius doesn't matter
                // note, choice columns are not cleared, but cummulative over all shortest path
                // pairs
            }
        }

        std::vector<int> depthcounts;
        depthcounts.push_back(0);

        pflipper<std::vector<std::pair<int, int>>> foundlist;
        foundlist.a().push_back(std::pair<int, int>(i, -1));
        covered[i] = true;
        int total_depth = 0, depth = 1, node_count = 1, pos = -1,
            previous = -1; // node_count includes this 1
        double weight = 0.0, rootweight = 0.0, total_weight = 0.0, w_total_depth = 0.0;
        if (m_weighted_measure_col != -1) {
            rootweight = weights[i];
            // include this line in total weights (as per nodecount)
            total_weight += rootweight;
        }
        int index = -1;
        int r = 0;
        for (int radius : radii) {
            while (foundlist.a().size()) {
                if (!m_choice) {
                    index = foundlist.a().back().first;
                } else {
                    pos = pafmath::pafrand() % foundlist.a().size();
                    index = foundlist.a().at(pos).first;
                    previous = foundlist.a().at(pos).second;
                    audittrail[index][0].previous.ref =
                        previous; // note 0th member used here: can be used individually different
                                  // radius previous
                }
                Connector &line = map.getConnections()[index];
                for (size_t k = 0; k < line.m_connections.size(); k++) {
                    if (!covered[line.m_connections[k]]) {
                        covered[line.m_connections[k]] = true;
                        foundlist.b().push_back(std::pair<int, int>(line.m_connections[k], index));
                        if (m_weighted_measure_col != -1) {
                            // the weight is taken from the discovered node:
                            weight = weights[line.m_connections[k]];
                            total_weight += weight;
                            w_total_depth += depth * weight;
                        }
                        if (m_choice && previous != -1) {
                            // both directional paths are now recorded for choice
                            // (coincidentally fixes choice problem which was completely wrong)
                            size_t here = index; // note: start counting from index as actually
                                                 // looking ahead here
                            while (here != i) {  // not i means not the current root for the path
                                audittrail[here][r].choice += 1;
                                audittrail[here][r].weighted_choice += weight * rootweight;
                                here = audittrail[here][0]
                                           .previous
                                           .ref; // <- note, just using 0th position: radius for
                                                 // the previous doesn't matter in this analysis
                            }
                            if (m_weighted_measure_col != -1) {
                                // in weighted choice, root node and current node receive values:
                                audittrail[i][r].weighted_choice += (weight * rootweight) * 0.5;
                                audittrail[line.m_connections[k]][r].weighted_choice +=
                                    (weight * rootweight) * 0.5;
                            }
                        }
                        total_depth += depth;
                        node_count++;
                        depthcounts.back() += 1;
                    }
                }
                if (!m_choice)
                    foundlist.a().pop_back();
                else
                    foundlist.a().erase(foundlist.a().begin() + pos);
                if (!foundlist.a().size()) {
                    foundlist.flip();
                    depth++;
                    depthcounts.push_back(0);
                    if (radius != -1 && depth > radius) {
                        break;
                    }
                }
            }
            // set the attributes for this node:
            row.setValue(count_col[r], float(node_count));
            if (m_weighted_measure_col != -1) {
                row.setValue(total_weight_col[r], float(total_weight));
            }
            // node count > 1 to avoid divide by zero (was > 2)
            if (node_count > 1) {
                // note -- node_count includes this one -- mean depth as per p.108 Social Logic of
                // Space
                double mean_depth = double(total_depth) / double(node_count - 1);
                row.setValue(depth_col[r], float(mean_depth));
                if (m_weighted_measure_col != -1) {
                    // weighted mean depth:
                    row.setValue(w_depth_col[r], float(w_total_depth / total_weight));
                }
                // total nodes > 2 to avoid divide by 0 (was > 3)
                if (node_count > 2 && mean_depth > 1.0) {
                    double ra = 2.0 * (mean_depth - 1.0) / double(node_count - 2);
                    // d-value / p-value from Depthmap 4 manual, note: node_count includes this one
                    double rra_d = ra / pafmath::dvalue(node_count);
                    double rra_p = ra / pafmath::dvalue(node_count);
                    double integ_tk = pafmath::teklinteg(node_count, total_depth);
                    row.setValue(integ_dv_col[r], float(1.0 / rra_d));

                    if (!simple_version) {
                        row.setValue(integ_pv_col[r], float(1.0 / rra_p));
                        if (total_depth - node_count + 1 > 1) {
                            row.setValue(integ_tk_col[r], float(integ_tk));
                        } else {
                            row.setValue(integ_tk_col[r], -1.0f);
                        }
                    }

                    if (m_fulloutput) {
                        row.setValue(ra_col[r], float(ra));

                        if (!simple_version) {
                            row.setValue(rra_col[r], float(rra_d));
                        }
                        row.setValue(td_col[r], float(total_depth));

                        if (!simple_version) {
                            // alan's palm-tree normalisation: palmtree
                            double dmin = node_count - 1;
                            double dmax = pafmath::palmtree(node_count, depth - 1);
                            if (dmax != dmin) {
                                row.setValue(penn_norm_col[r],
                                             float((dmax - total_depth) / (dmax - dmin)));
                            }
                        }
                    }
                } else {
                    row.setValue(integ_dv_col[r], -1.0f);

                    if (!simple_version) {
                        row.setValue(integ_pv_col[r], -1.0f);
                        row.setValue(integ_tk_col[r], -1.0f);
                    }
                    if (m_fulloutput) {
                        row.setValue(ra_col[r], -1.0f);

                        if (!simple_version) {
                            row.setValue(rra_col[r], -1.0f);
                        }

                        row.setValue(td_col[r], -1.0f);

                        if (!simple_version) {
                            row.setValue(penn_norm_col[r], -1.0f);
                        }
                    }
                }

                if (!simple_version) {
                    double entropy = 0.0, intensity = 0.0, rel_entropy = 0.0, factorial = 1.0,
                           harmonic = 0.0;
                    for (size_t k = 0; k < depthcounts.size(); k++) {
                        if (depthcounts[k] != 0) {
                            // some debate over whether or not this should be node count - 1
                            // (i.e., including or not including the node itself)
                            double prob = double(depthcounts[k]) / double(node_count);
                            entropy -= prob * pafmath::log2(prob);
                            // Formula from Turner 2001, "Depthmap"
                            factorial *= double(k + 1);
                            double q =
                                (pow(mean_depth, double(k)) / double(factorial)) * exp(-mean_depth);
                            rel_entropy += (double)prob * pafmath::log2(prob / q);
                            //
                            harmonic += 1.0 / double(depthcounts[k]);
                        }
                    }
                    harmonic = double(depthcounts.size()) / harmonic;
                    if (total_depth > node_count) {
                        intensity = node_count * entropy / (total_depth - node_count);
                    } else {
                        intensity = -1;
                    }
                    row.setValue(entropy_col[r], float(entropy));
                    row.setValue(rel_entropy_col[r], float(rel_entropy));
                    row.setValue(intensity_col[r], float(intensity));
                    row.setValue(harmonic_col[r], float(harmonic));
                }
            } else {
                row.setValue(depth_col[r], -1.0f);
                row.setValue(integ_dv_col[r], -1.0f);

                if (!simple_version) {
                    row.setValue(integ_pv_col[r], -1.0f);
                    row.setValue(integ_tk_col[r], -1.0f);
                    row.setValue(entropy_col[r], -1.0f);
                    row.setValue(rel_entropy_col[r], -1.0f);
                    row.setValue(harmonic_col[r], -1.0f);
                }
            }
            ++r;
        }
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    delete[] covered;
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, i);
            }
        }
        i++;
    }
    delete[] covered;
    if (m_choice) {
        i = 0;
        for (auto &iter : attributes) {
            AttributeRow &row = iter.getRow();
            double total_choice = 0.0, w_total_choice = 0.0;
            for (size_t r = 0; r < radii.size(); r++) {
                total_choice += audittrail[i][r].choice;
                w_total_choice += audittrail[i][r].weighted_choice;
                // n.b., normalise choice according to (n-1)(n-2)/2 (maximum possible through
                // routes)
                double node_count = row.getValue(count_col[r]);
                double total_weight = 0;
                if (m_weighted_measure_col != -1) {
                    total_weight = row.getValue(total_weight_col[r]);
                }
                if (node_count > 2) {
                    row.setValue(choice_col[r], float(total_choice));
                    row.setValue(n_choice_col[r],
                                 float(2.0 * total_choice / ((node_count - 1) * (node_count - 2))));
                    if (m_weighted_measure_col != -1) {
                        row.setValue(w_choice_col[r], float(w_total_choice));
                        row.setValue(nw_choice_col[r],
                                     float(2.0 * w_total_choice / (total_weight * total_weight)));
                    }
                } else {
                    row.setValue(choice_col[r], -1);
                    row.setValue(n_choice_col[r], -1);
                    if (m_weighted_measure_col != -1) {
                        row.setValue(w_choice_col[r], -1);
                        row.setValue(nw_choice_col[r], -1);
                    }
                }
            }
            i++;
        }
        for (size_t i = 0; i < map.getShapeCount(); i++) {
            // TODO: At this moment, GCC triggers a warning here. Find
            // a better solution rather than disabling the warnind
            delete[] audittrail[i];
        }
        delete[] audittrail;
    }

    result.completed = true;

    return result;
}
