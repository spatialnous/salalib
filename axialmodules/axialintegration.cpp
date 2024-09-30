// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "axialintegration.h"

#include "../genlib/pflipper.h"

std::vector<std::string> AxialIntegration::getRequiredColumns(std::vector<int> radii,
                                                              std::string weightingColName,
                                                              bool simpleVersion) {
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

            if (m_weightedMeasureCol != -1) {
                newColumns.push_back(getFormattedColumn( //
                    Column::MEAN_DEPTH, radius, weightingColName));
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL, radius, weightingColName));
            }

            if (!simpleVersion) {
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
                if (m_weightedMeasureCol != -1) {
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

                if (!simpleVersion) {
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
                if (m_weightedMeasureCol != -1) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, radius, weightingColName));
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, radius, weightingColName, Normalisation::NORM));
                }
            }

            if (!simpleVersion) {
                // columns only when simple-version is not selected
                auto formattedCol = getFormattedColumn( //
                    Column::ENTROPY, radius);
                auto fomCol = formattedCol.c_str();
                newColumns.push_back(fomCol);
            }

            // Columns that are always created
            newColumns.push_back(getFormattedColumn( //
                Column::INTEGRATION, radius, std::nullopt, Normalisation::HH));

            if (!simpleVersion) {
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

            if (!simpleVersion) {
                // columns only when simple-version is not selected
                newColumns.push_back(getFormattedColumn( //
                    Column::RELATIVISED_ENTROPY, radius));
            }

            if (m_weightedMeasureCol != -1) {
                newColumns.push_back(getFormattedColumn( //
                    Column::MEAN_DEPTH, radius, weightingColName));
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL, radius, weightingColName));
            }

            if (m_fulloutput) {
                if (!simpleVersion) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::RA, radius, std::nullopt, Normalisation::PENN));
                }
                newColumns.push_back(getFormattedColumn( //
                    Column::RA, radius));

                if (!simpleVersion) {
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
    bool radiusN = false;
    std::vector<int> radii;
    for (double radius : radiusSet) {
        if (radius < 0) {
            radiusN = true;
        } else {
            radii.push_back(static_cast<int>(radius));
        }
    }
    if (radiusN) {
        radii.push_back(-1);
    }
    return radii;
}

AnalysisResult AxialIntegration::run(Communicator *comm, ShapeGraph &map, bool simpleVersion) {
    // note, from 10.0, Depthmap no longer includes *self* connections on axial lines
    // self connections are stripped out on loading graph files, as well as no longer made

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getShapeCount());
    }

    AnalysisResult result;

    AttributeTable &attributes = map.getAttributeTable();

    std::vector<int> radii = getFormattedRadii(m_radiusSet);

    // retrieve weighted col data, as this may well be overwritten in the new analysis:
    std::vector<double> weights;
    std::string weightingColText;
    if (m_weightedMeasureCol != -1) {
        weightingColText = attributes.getColumnName(m_weightedMeasureCol);
        for (size_t i = 0; i < map.getShapeCount(); i++) {
            weights.push_back(map.getAttributeRowFromShapeIndex(i).getValue(m_weightedMeasureCol));
        }
    }

    // first enter the required attribute columns:
    auto newColumns = getRequiredColumns(radii, weightingColText, simpleVersion);
    for (auto &col : newColumns) {
        attributes.insertOrResetColumn(col);
        result.addAttribute(col);
    }

    // then look up all the columns... eek:
    std::vector<size_t> choiceCol, nChoiceCol, wChoiceCol, nwChoiceCol, entropyCol, integDvCol,
        integPvCol, integTkCol, intensityCol, depthCol, countCol, relEntropyCol, pennNormCol,
        wDepthCol, totalWeightCol, raCol, rraCol, tdCol, harmonicCol;
    for (int radius : radii) {
        std::string radiusText;

        if (m_choice) {
            choiceCol.push_back(getFormattedColumnIdx( //
                attributes, Column::CHOICE, radius));
            nChoiceCol.push_back(getFormattedColumnIdx( //
                attributes, Column::CHOICE, radius, std::nullopt, Normalisation::NORM));
            if (m_weightedMeasureCol != -1) {
                wChoiceCol.push_back(getFormattedColumnIdx( //
                    attributes, Column::CHOICE, radius, weightingColText));
                nwChoiceCol.push_back(getFormattedColumnIdx( //
                    attributes, Column::CHOICE, radius, weightingColText, Normalisation::NORM));
            }
        }
        if (!simpleVersion) {
            entropyCol.push_back(getFormattedColumnIdx( //
                attributes, Column::ENTROPY, radius));
        }

        integDvCol.push_back(getFormattedColumnIdx( //
            attributes, Column::INTEGRATION, radius, std::nullopt, Normalisation::HH));

        if (!simpleVersion) {
            integPvCol.push_back(getFormattedColumnIdx( //
                attributes, Column::INTEGRATION, radius, std::nullopt, Normalisation::PV));
            integTkCol.push_back(getFormattedColumnIdx( //
                attributes, Column::INTEGRATION, radius, std::nullopt, Normalisation::TK));
            intensityCol.push_back(getFormattedColumnIdx( //
                attributes, Column::INTENSITY, radius));
            harmonicCol.push_back(getFormattedColumnIdx( //
                attributes, Column::HARMONIC_MEAN_DEPTH, radius));
        }

        depthCol.push_back(getFormattedColumnIdx( //
            attributes, Column::MEAN_DEPTH, radius));
        countCol.push_back(getFormattedColumnIdx( //
            attributes, Column::NODE_COUNT, radius));

        if (!simpleVersion) {
            relEntropyCol.push_back(getFormattedColumnIdx( //
                attributes, Column::RELATIVISED_ENTROPY, radius));
        }

        if (m_weightedMeasureCol != -1) {
            wDepthCol.push_back(getFormattedColumnIdx( //
                attributes, Column::MEAN_DEPTH, radius, weightingColText, std::nullopt));
            totalWeightCol.push_back(getFormattedColumnIdx( //
                attributes, Column::TOTAL, radius, weightingColText, std::nullopt));
        }
        if (m_fulloutput) {
            raCol.push_back(getFormattedColumnIdx( //
                attributes, Column::RA, radius, std::nullopt, std::nullopt));

            if (!simpleVersion) {
                pennNormCol.push_back(getFormattedColumnIdx( //
                    attributes, Column::RA, radius, std::nullopt, Normalisation::PENN));
                rraCol.push_back(getFormattedColumnIdx( //
                    attributes, Column::RRA, radius, std::nullopt, std::nullopt));
            }
            tdCol.push_back(getFormattedColumnIdx( //
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
        int totalDepth = 0, depth = 1, nodeCount = 1, pos = -1,
            previous = -1; // node_count includes this 1
        double weight = 0.0, rootweight = 0.0, totalWeight = 0.0, wTotalDepth = 0.0;
        if (m_weightedMeasureCol != -1) {
            rootweight = weights[i];
            // include this line in total weights (as per nodecount)
            totalWeight += rootweight;
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
                for (size_t k = 0; k < line.connections.size(); k++) {
                    if (!covered[line.connections[k]]) {
                        covered[line.connections[k]] = true;
                        foundlist.b().push_back(std::pair<int, int>(line.connections[k], index));
                        if (m_weightedMeasureCol != -1) {
                            // the weight is taken from the discovered node:
                            weight = weights[line.connections[k]];
                            totalWeight += weight;
                            wTotalDepth += depth * weight;
                        }
                        if (m_choice && previous != -1) {
                            // both directional paths are now recorded for choice
                            // (coincidentally fixes choice problem which was completely wrong)
                            size_t here = index; // note: start counting from index as actually
                                                 // looking ahead here
                            while (here != i) {  // not i means not the current root for the path
                                audittrail[here][r].choice += 1;
                                audittrail[here][r].weightedChoice += weight * rootweight;
                                here = audittrail[here][0]
                                           .previous
                                           .ref; // <- note, just using 0th position: radius for
                                                 // the previous doesn't matter in this analysis
                            }
                            if (m_weightedMeasureCol != -1) {
                                // in weighted choice, root node and current node receive values:
                                audittrail[i][r].weightedChoice += (weight * rootweight) * 0.5;
                                audittrail[line.connections[k]][r].weightedChoice +=
                                    (weight * rootweight) * 0.5;
                            }
                        }
                        totalDepth += depth;
                        nodeCount++;
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
            row.setValue(countCol[r], float(nodeCount));
            if (m_weightedMeasureCol != -1) {
                row.setValue(totalWeightCol[r], float(totalWeight));
            }
            // node count > 1 to avoid divide by zero (was > 2)
            if (nodeCount > 1) {
                // note -- node_count includes this one -- mean depth as per p.108 Social Logic of
                // Space
                double meanDepth = double(totalDepth) / double(nodeCount - 1);
                row.setValue(depthCol[r], float(meanDepth));
                if (m_weightedMeasureCol != -1) {
                    // weighted mean depth:
                    row.setValue(wDepthCol[r], float(wTotalDepth / totalWeight));
                }
                // total nodes > 2 to avoid divide by 0 (was > 3)
                if (nodeCount > 2 && meanDepth > 1.0) {
                    double ra = 2.0 * (meanDepth - 1.0) / double(nodeCount - 2);
                    // d-value / p-value from Depthmap 4 manual, note: node_count includes this one
                    double rraD = ra / pafmath::dvalue(nodeCount);
                    double rraP = ra / pafmath::dvalue(nodeCount);
                    double integTk = pafmath::teklinteg(nodeCount, totalDepth);
                    row.setValue(integDvCol[r], float(1.0 / rraD));

                    if (!simpleVersion) {
                        row.setValue(integPvCol[r], float(1.0 / rraP));
                        if (totalDepth - nodeCount + 1 > 1) {
                            row.setValue(integTkCol[r], float(integTk));
                        } else {
                            row.setValue(integTkCol[r], -1.0f);
                        }
                    }

                    if (m_fulloutput) {
                        row.setValue(raCol[r], float(ra));

                        if (!simpleVersion) {
                            row.setValue(rraCol[r], float(rraD));
                        }
                        row.setValue(tdCol[r], float(totalDepth));

                        if (!simpleVersion) {
                            // alan's palm-tree normalisation: palmtree
                            double dmin = nodeCount - 1;
                            double dmax = pafmath::palmtree(nodeCount, depth - 1);
                            if (dmax != dmin) {
                                row.setValue(pennNormCol[r],
                                             float((dmax - totalDepth) / (dmax - dmin)));
                            }
                        }
                    }
                } else {
                    row.setValue(integDvCol[r], -1.0f);

                    if (!simpleVersion) {
                        row.setValue(integPvCol[r], -1.0f);
                        row.setValue(integTkCol[r], -1.0f);
                    }
                    if (m_fulloutput) {
                        row.setValue(raCol[r], -1.0f);

                        if (!simpleVersion) {
                            row.setValue(rraCol[r], -1.0f);
                        }

                        row.setValue(tdCol[r], -1.0f);

                        if (!simpleVersion) {
                            row.setValue(pennNormCol[r], -1.0f);
                        }
                    }
                }

                if (!simpleVersion) {
                    double entropy = 0.0, intensity = 0.0, relEntropy = 0.0, factorial = 1.0,
                           harmonic = 0.0;
                    for (size_t k = 0; k < depthcounts.size(); k++) {
                        if (depthcounts[k] != 0) {
                            // some debate over whether or not this should be node count - 1
                            // (i.e., including or not including the node itself)
                            double prob = double(depthcounts[k]) / double(nodeCount);
                            entropy -= prob * pafmath::log2(prob);
                            // Formula from Turner 2001, "Depthmap"
                            factorial *= double(k + 1);
                            double q =
                                (pow(meanDepth, double(k)) / double(factorial)) * exp(-meanDepth);
                            relEntropy += (double)prob * pafmath::log2(prob / q);
                            //
                            harmonic += 1.0 / double(depthcounts[k]);
                        }
                    }
                    harmonic = double(depthcounts.size()) / harmonic;
                    if (totalDepth > nodeCount) {
                        intensity = nodeCount * entropy / (totalDepth - nodeCount);
                    } else {
                        intensity = -1;
                    }
                    row.setValue(entropyCol[r], float(entropy));
                    row.setValue(relEntropyCol[r], float(relEntropy));
                    row.setValue(intensityCol[r], float(intensity));
                    row.setValue(harmonicCol[r], float(harmonic));
                }
            } else {
                row.setValue(depthCol[r], -1.0f);
                row.setValue(integDvCol[r], -1.0f);

                if (!simpleVersion) {
                    row.setValue(integPvCol[r], -1.0f);
                    row.setValue(integTkCol[r], -1.0f);
                    row.setValue(entropyCol[r], -1.0f);
                    row.setValue(relEntropyCol[r], -1.0f);
                    row.setValue(harmonicCol[r], -1.0f);
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
            double totalChoice = 0.0, wTotalChoice = 0.0;
            for (size_t r = 0; r < radii.size(); r++) {
                totalChoice += audittrail[i][r].choice;
                wTotalChoice += audittrail[i][r].weightedChoice;
                // n.b., normalise choice according to (n-1)(n-2)/2 (maximum possible through
                // routes)
                double nodeCount = row.getValue(countCol[r]);
                double totalWeight = 0;
                if (m_weightedMeasureCol != -1) {
                    totalWeight = row.getValue(totalWeightCol[r]);
                }
                if (nodeCount > 2) {
                    row.setValue(choiceCol[r], float(totalChoice));
                    row.setValue(nChoiceCol[r],
                                 float(2.0 * totalChoice / ((nodeCount - 1) * (nodeCount - 2))));
                    if (m_weightedMeasureCol != -1) {
                        row.setValue(wChoiceCol[r], float(wTotalChoice));
                        row.setValue(nwChoiceCol[r],
                                     float(2.0 * wTotalChoice / (totalWeight * totalWeight)));
                    }
                } else {
                    row.setValue(choiceCol[r], -1);
                    row.setValue(nChoiceCol[r], -1);
                    if (m_weightedMeasureCol != -1) {
                        row.setValue(wChoiceCol[r], -1);
                        row.setValue(nwChoiceCol[r], -1);
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
