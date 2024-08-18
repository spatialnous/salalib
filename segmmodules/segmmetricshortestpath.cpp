// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmmetricshortestpath.h"

#include "segmhelpers.h"

AnalysisResult SegmentMetricShortestPath::run(Communicator *) {

    AnalysisResult result;

    AttributeTable &attributes = m_map.getAttributeTable();
    size_t shapeCount = m_map.getShapeCount();

    std::string colText = Column::METRIC_SHORTEST_PATH_DISTANCE;
    size_t distCol = attributes.insertOrResetColumn(colText);
    result.addAttribute(colText);
    colText = Column::METRIC_SHORTEST_PATH_ORDER;
    size_t pathCol = attributes.insertOrResetColumn(colText);
    result.addAttribute(colText);

    // record axial line refs for topological analysis
    std::vector<int> axialrefs;
    // quick through to find the longest seg length
    std::vector<float> seglengths;
    float maxseglength = 0.0f;
    for (size_t cursor = 0; cursor < shapeCount; cursor++) {
        AttributeRow &row = m_map.getAttributeRowFromShapeIndex(cursor);
        axialrefs.push_back(row.getValue("Axial Line Ref"));
        seglengths.push_back(row.getValue("Segment Length"));
        if (seglengths.back() > maxseglength) {
            maxseglength = seglengths.back();
        }
    }

    int maxbin = 512;

    std::vector<unsigned int> seen(shapeCount, 0xffffffff);
    std::vector<TopoMetSegmentRef> audittrail(shapeCount);
    std::vector<int> list[512]; // 512 bins!
    int open = 0;

    seen[m_refFrom] = 0;
    open++;
    double length = seglengths[m_refFrom];
    audittrail[m_refFrom] = TopoMetSegmentRef(m_refFrom, Connector::SEG_CONN_ALL, length * 0.5, -1);
    // better to divide by 511 but have 512 bins...
    list[(int(floor(0.5 + 511 * length / maxseglength))) % 512].push_back(m_refFrom);
    m_map.getAttributeRowFromShapeIndex(m_refFrom).setValue(distCol, 0);

    unsigned int segdepth = 0;
    int bin = 0;

    std::map<unsigned int, unsigned int> parents;
    bool refFound = false;

    while (open != 0 && !refFound) {
        while (list[bin].empty()) {
            bin++;
            segdepth += 1;
            if (bin == maxbin) {
                bin = 0;
            }
        }
        //
        TopoMetSegmentRef &here = audittrail[list[bin].back()];
        list[bin].pop_back();
        open--;
        // this is necessary using unsigned ints for "seen", as it is possible to add a node twice
        if (here.done) {
            continue;
        } else {
            here.done = true;
        }

        Connector &axline = m_map.getConnections().at(here.ref);
        int connectedCursor = -2;

        auto iter = axline.backSegconns.begin();
        bool backsegs = true;

        while (connectedCursor != -1) {
            if (backsegs && iter == axline.backSegconns.end()) {
                iter = axline.forwardSegconns.begin();
                backsegs = false;
            }
            if (!backsegs && iter == axline.forwardSegconns.end()) {
                break;
            }

            connectedCursor = iter->first.ref;
            if (seen[connectedCursor] > segdepth) {
                float length = seglengths[connectedCursor];
                seen[connectedCursor] = segdepth;
                audittrail[connectedCursor] =
                    TopoMetSegmentRef(connectedCursor, here.dir, here.dist + length, here.ref);
                parents[connectedCursor] = here.ref;
                // puts in a suitable bin ahead of us...
                open++;
                //
                // better to divide by 511 but have 512 bins...
                list[(bin + int(floor(0.5 + 511 * length / maxseglength))) % 512].push_back(
                    connectedCursor);
                AttributeRow &row = m_map.getAttributeRowFromShapeIndex(connectedCursor);
                row.setValue(distCol, here.dist + length * 0.5);
            }
            if (connectedCursor == m_refTo) {
                refFound = true;
                break;
            }
            iter++;
        }
    }

    auto refToParent = parents.find(m_refTo);
    int counter = 0;
    while (refToParent != parents.end()) {
        AttributeRow &row = m_map.getAttributeRowFromShapeIndex(refToParent->first);
        row.setValue(pathCol, counter);
        counter++;
        refToParent = parents.find(refToParent->second);
    }
    m_map.getAttributeRowFromShapeIndex(m_refFrom).setValue(pathCol, counter);

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        AttributeRow &row = iter->getRow();
        if (row.getValue(pathCol) < 0) {
            row.setValue(distCol, -1);
        } else {
            row.setValue(pathCol, counter - row.getValue(pathCol));
        }
    }

    result.completed = true;

    return result;
}
