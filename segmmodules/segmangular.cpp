// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmangular.h"

AnalysisResult SegmentAngular::run(Communicator *comm, ShapeGraph &map, bool) {
    AnalysisResult result;

    if (map.getMapType() != ShapeMap::SEGMENTMAP) {
        return result;
    }

    AttributeTable &attributes = map.getAttributeTable();

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getConnections().size());
    }

    // note: radius must be sorted lowest to highest, but if -1 occurs ("radius n") it needs to be
    // last...
    // ...to ensure no mess ups, we'll re-sort here:
    bool radiusN = false;
    std::vector<double> radii;
    for (double radius : m_radiusSet) {
        if (radius < 0) {
            radiusN = true;
        } else {
            radii.push_back(radius);
        }
    }
    if (radiusN) {
        radii.push_back(-1.0);
    }

    std::vector<size_t> depthCol, countCol, totalCol;
    // first enter table values
    for (auto radius : radii) {
        std::string depthColText = getFormattedColumn(Column::ANGULAR_MEAN_DEPTH, radius);
        attributes.insertOrResetColumn(depthColText.c_str());
        result.addAttribute(depthColText);
        std::string countColText = getFormattedColumn(Column::ANGULAR_NODE_COUNT, radius);
        attributes.insertOrResetColumn(countColText.c_str());
        result.addAttribute(countColText);
        std::string totalColText = getFormattedColumn(Column::ANGULAR_TOTAL_DEPTH, radius);
        attributes.insertOrResetColumn(totalColText.c_str());
        result.addAttribute(totalColText);
    }

    for (auto radius : radii) {
        std::string radiusText = makeRadiusText(RadiusType::ANGULAR, radius);
        std::string depthColText = getFormattedColumn(Column::ANGULAR_MEAN_DEPTH, radius);
        depthCol.push_back(attributes.getColumnIndex(depthColText.c_str()));
        std::string countColText = getFormattedColumn(Column::ANGULAR_NODE_COUNT, radius);
        countCol.push_back(attributes.getColumnIndex(countColText.c_str()));
        std::string totalColText = getFormattedColumn(Column::ANGULAR_TOTAL_DEPTH, radius);
        totalCol.push_back(attributes.getColumnIndex(totalColText.c_str()));
    }

    std::vector<bool> covered(map.getShapeCount());
    size_t i = 0;
    for (auto &iter : attributes) {
        for (size_t j = 0; j < map.getShapeCount(); j++) {
            covered[j] = false;
        }
        std::vector<std::pair<float, SegmentData>> anglebins;
        anglebins.push_back(std::make_pair(0.0f, SegmentData(0, i, SegmentRef(), 0, 0.0, 0)));

        std::vector<double> totalDepth;
        std::vector<int> nodeCount;
        for (size_t r = 0; r < radii.size(); r++) {
            totalDepth.push_back(0.0);
            nodeCount.push_back(0);
        }
        // node_count includes this one, but will be added in next algo:
        while (anglebins.size()) {
            auto iter = anglebins.begin();
            SegmentData lineindex = iter->second;
            if (!covered[lineindex.ref]) {
                covered[lineindex.ref] = true;
                double depthToLine = iter->first;
                totalDepth[lineindex.coverage] += depthToLine;
                nodeCount[lineindex.coverage] += 1;
                anglebins.erase(iter);
                Connector &line = map.getConnections()[lineindex.ref];
                if (lineindex.dir != -1) {
                    for (auto &segconn : line.forwardSegconns) {
                        if (!covered[segconn.first.ref]) {
                            double angle = depthToLine + segconn.second;
                            size_t rbin = lineindex.coverage;
                            while (rbin != radii.size() && radii[rbin] != -1 &&
                                   angle > radii[rbin]) {
                                rbin++;
                            }
                            if (rbin != radii.size()) {
                                depthmapX::insert_sorted(
                                    anglebins,
                                    std::make_pair(
                                        float(angle),
                                        SegmentData(segconn.first, SegmentRef(), 0, 0.0, rbin)));
                            }
                        }
                    }
                }
                if (lineindex.dir != 1) {
                    for (auto &segconn : line.backSegconns) {
                        if (!covered[segconn.first.ref]) {
                            double angle = depthToLine + segconn.second;
                            size_t rbin = lineindex.coverage;
                            while (rbin != radii.size() && radii[rbin] != -1 &&
                                   angle > radii[rbin]) {
                                rbin++;
                            }
                            if (rbin != radii.size()) {
                                depthmapX::insert_sorted(
                                    anglebins,
                                    std::make_pair(
                                        float(angle),
                                        SegmentData(segconn.first, SegmentRef(), 0, 0.0, rbin)));
                            }
                        }
                    }
                }
            } else {
                anglebins.erase(iter);
            }
        }
        AttributeRow &row = iter.getRow();
        // set the attributes for this node:
        int cursNodeCount = 0;
        double cursTotalDepth = 0.0;
        for (size_t r = 0; r < radii.size(); r++) {
            cursNodeCount += nodeCount[r];
            cursTotalDepth += totalDepth[r];
            row.setValue(countCol[r], float(cursNodeCount));
            if (cursNodeCount > 1) {
                // note -- node_count includes this one -- mean depth as per p.108 Social Logic of
                // Space
                double meanDepth = cursTotalDepth / double(cursNodeCount - 1);
                row.setValue(depthCol[r], float(meanDepth));
                row.setValue(totalCol[r], float(cursTotalDepth));
            } else {
                row.setValue(depthCol[r], -1);
                row.setValue(totalCol[r], -1);
            }
        }
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, i);
            }
        }
        i++;
    }

    result.completed = true;

    return result;
}
