// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmtulipshortestpath.h"

// revised to use tulip bins for faster analysis of large spaces

AnalysisResult SegmentTulipShortestPath::run(Communicator *) {

    AnalysisResult result;

    auto &selected = m_map.getSelSet();
    if (selected.size() != 2) {
        return result;
    }

    AttributeTable &attributes = m_map.getAttributeTable();

    std::string colText = "Angular Shortest Path Angle";
    size_t angle_col = attributes.insertOrResetColumn(colText);
    result.addAttribute(colText);
    colText = "Angular Shortest Path Order";
    size_t path_col = attributes.insertOrResetColumn(colText);
    result.addAttribute(colText);

    // The original code set tulip_bins to 1024, divided by two and added one
    // in order to duplicate previous code (using a semicircle of tulip bins)
    size_t tulip_bins = 513;

    std::vector<bool> covered(m_map.getConnections().size());
    for (size_t i = 0; i < m_map.getConnections().size(); i++) {
        covered[i] = false;
    }
    std::vector<std::vector<SegmentData>> bins(tulip_bins);

    int refFrom = *selected.begin();
    int refTo = *selected.rbegin();

    int opencount = 0;

    int row = std::distance(m_map.getAllShapes().begin(), m_map.getAllShapes().find(refFrom));
    if (row != -1) {
        bins[0].push_back(SegmentData(0, row, SegmentRef(), 0, 0.0, 0));
        opencount++;
    }

    std::map<unsigned int, unsigned int> parents;
    bool refFound = false;

    int depthlevel = 0;
    auto binIter = bins.begin();
    int currentbin = 0;
    while (opencount && !refFound) {
        while (binIter->empty()) {
            depthlevel++;
            binIter++;
            currentbin++;
            if (binIter == bins.end()) {
                binIter = bins.begin();
            }
        }
        SegmentData lineindex;
        if (binIter->size() > 1) {
            // it is slightly slower to delete from an arbitrary place in the bin,
            // but it is necessary to use random paths to even out the number of times through equal
            // paths
            int curr = pafrand() % binIter->size();
            auto currIter = binIter->begin() + curr;
            lineindex = *currIter;
            binIter->erase(currIter);
            // note: do not clear choice values here!
        } else {
            lineindex = binIter->front();
            binIter->pop_back();
        }
        opencount--;
        if (!covered[lineindex.ref]) {
            covered[lineindex.ref] = true;
            Connector &line = m_map.getConnections()[lineindex.ref];
            // convert depth from tulip_bins normalised to standard angle
            // (note the -1)
            double depth_to_line = depthlevel / ((tulip_bins - 1) * 0.5);
            m_map.getAttributeRowFromShapeIndex(lineindex.ref).setValue(angle_col, depth_to_line);
            int extradepth;
            if (lineindex.dir != -1) {
                for (auto &segconn : line.m_forward_segconns) {
                    if (!covered[segconn.first.ref]) {
                        extradepth = (int)floor(segconn.second * tulip_bins * 0.5);
                        bins[(currentbin + tulip_bins + extradepth) % tulip_bins].push_back(
                            SegmentData(segconn.first, lineindex.ref, lineindex.segdepth + 1, 0.0,
                                        0));
                        if (parents.find(segconn.first.ref) == parents.end()) {
                            parents[segconn.first.ref] = lineindex.ref;
                        }
                        opencount++;
                    }
                }
            }
            if (lineindex.dir != 1) {
                for (auto &segconn : line.m_back_segconns) {
                    if (!covered[segconn.first.ref]) {
                        extradepth = (int)floor(segconn.second * tulip_bins * 0.5);
                        bins[(currentbin + tulip_bins + extradepth) % tulip_bins].push_back(
                            SegmentData(segconn.first, lineindex.ref, lineindex.segdepth + 1, 0.0,
                                        0));
                        if (parents.find(segconn.first.ref) == parents.end()) {
                            parents[segconn.first.ref] = lineindex.ref;
                        }
                        opencount++;
                    }
                }
            }
            if (lineindex.ref == refTo) {
                refFound = true;
                break;
            }
        }
    }

    auto refToParent = parents.find(refTo);
    int counter = 0;
    while (refToParent != parents.end()) {
        AttributeRow &row = m_map.getAttributeRowFromShapeIndex(refToParent->first);
        row.setValue(path_col, counter);
        counter++;
        refToParent = parents.find(refToParent->second);
    }
    m_map.getAttributeRowFromShapeIndex(refFrom).setValue(path_col, counter);

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        AttributeRow &row = iter->getRow();
        if (row.getValue(path_col) < 0) {
            row.setValue(angle_col, -1);
        } else {
            row.setValue(path_col, counter - row.getValue(path_col));
        }
    }

    m_map.overrideDisplayedAttribute(-2); // <- override if it's already showing
    m_map.setDisplayedAttribute(static_cast<int>(angle_col));

    result.completed = true;

    return result;
}
