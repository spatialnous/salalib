// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmtopologicalshortestpath.h"

#include "segmhelpers.h"

AnalysisResult SegmentTopologicalShortestPath::run(Communicator *) {

    AnalysisResult result;
    auto &selected = m_map.getSelSet();
    if (selected.size() != 2) {
        return result;
    }

    AttributeTable &attributes = m_map.getAttributeTable();
    size_t shapeCount = m_map.getShapeCount();

    std::string colText = "Topological Shortest Path Depth";
    size_t depth_col = attributes.insertOrResetColumn(colText);
    result.addAttribute(colText);
    colText = "Topological Shortest Path Order";
    size_t path_col = attributes.insertOrResetColumn(colText);
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

    int maxbin = 2;

    std::vector<unsigned int> seen(shapeCount, 0xffffffff);
    std::vector<TopoMetSegmentRef> audittrail(shapeCount);
    std::vector<int> list[512]; // 512 bins!
    int open = 0;

    int refFrom = *selected.begin();
    int refTo = *selected.rbegin();

    seen[refFrom] = 0;
    open++;
    double length = seglengths[refFrom];
    audittrail[refFrom] = TopoMetSegmentRef(refFrom, Connector::SEG_CONN_ALL, length * 0.5, -1);
    list[0].push_back(refFrom);
    m_map.getAttributeRowFromShapeIndex(refFrom).setValue(depth_col, 0);

    unsigned int segdepth = 0;
    int bin = 0;

    std::map<unsigned int, unsigned int> parents;
    bool refFound = false;

    while (open != 0) {
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
        int connected_cursor = -2;

        auto iter = axline.m_back_segconns.begin();
        bool backsegs = true;

        while (connected_cursor != -1) {
            if (backsegs && iter == axline.m_back_segconns.end()) {
                iter = axline.m_forward_segconns.begin();
                backsegs = false;
            }
            if (!backsegs && iter == axline.m_forward_segconns.end()) {
                break;
            }

            connected_cursor = iter->first.ref;
            AttributeRow &row = m_map.getAttributeRowFromShapeIndex(connected_cursor);
            if (seen[connected_cursor] > segdepth) {
                float length = seglengths[connected_cursor];
                int axialref = axialrefs[connected_cursor];
                seen[connected_cursor] = segdepth;
                audittrail[connected_cursor] =
                    TopoMetSegmentRef(connected_cursor, here.dir, here.dist + length, here.ref);
                // puts in a suitable bin ahead of us...
                open++;
                //
                if (axialrefs[here.ref] == axialref) {
                    list[bin].push_back(connected_cursor);
                    row.setValue(depth_col, segdepth);
                } else {
                    list[(bin + 1) % 2].push_back(connected_cursor);
                    seen[connected_cursor] =
                        segdepth + 1; // this is so if another node is connected directly to this
                                      // one but is found later it is still handled -- note it can
                                      // result in the connected cursor being added twice
                    row.setValue(depth_col, segdepth + 1);
                }
                if (parents.find(connected_cursor) == parents.end()) {
                    parents[connected_cursor] = here.ref;
                }
            }
            if (connected_cursor == refTo) {
                refFound = true;
                break;
            }
            iter++;
        }
        if (refFound)
            break;
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
            row.setValue(depth_col, -1);
        } else {
            row.setValue(path_col, counter - row.getValue(path_col));
        }
    }

    m_map.overrideDisplayedAttribute(-2); // <- override if it's already showing
    m_map.setDisplayedAttribute(static_cast<int>(depth_col));

    result.completed = true;

    return result;
}
