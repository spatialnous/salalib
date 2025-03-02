// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmtopologicalshortestpath.hpp"

#include "segmhelpers.hpp"

AnalysisResult SegmentTopologicalShortestPath::run(Communicator *) {

    AnalysisResult result;

    AttributeTable &attributes = m_map.getAttributeTable();
    size_t shapeCount = m_map.getShapeCount();

    size_t depthCol = attributes.insertOrResetColumn(Column::TOPOLOGICAL_SHORTEST_PATH_DEPTH);
    result.addAttribute(Column::TOPOLOGICAL_SHORTEST_PATH_DEPTH);
    size_t pathCol = attributes.insertOrResetColumn(Column::TOPOLOGICAL_SHORTEST_PATH_ORDER);
    result.addAttribute(Column::TOPOLOGICAL_SHORTEST_PATH_ORDER);

    // record axial line refs for topological analysis
    std::vector<int> axialrefs;
    // quick through to find the longest seg length
    std::vector<float> seglengths;
    float maxseglength = 0.0f;
    for (size_t cursor = 0; cursor < shapeCount; cursor++) {
        AttributeRow &row = m_map.getAttributeRowFromShapeIndex(cursor);
        axialrefs.push_back(static_cast<int>(row.getValue("Axial Line Ref")));
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

    seen[static_cast<size_t>(m_refFrom)] = 0;
    open++;
    double length = seglengths[static_cast<size_t>(m_refFrom)];
    audittrail[static_cast<size_t>(m_refFrom)] =
        TopoMetSegmentRef(m_refFrom, Connector::SEG_CONN_ALL, length * 0.5, -1);
    list[0].push_back(m_refFrom);
    m_map.getAttributeRowFromShapeIndex(static_cast<size_t>(m_refFrom)).setValue(depthCol, 0);

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
        TopoMetSegmentRef &here = audittrail[static_cast<size_t>(list[bin].back())];
        list[bin].pop_back();
        open--;
        // this is necessary using unsigned ints for "seen", as it is possible to add a node twice
        if (here.done) {
            continue;
        } else {
            here.done = true;
        }

        Connector &axline = m_map.getConnections().at(static_cast<size_t>(here.ref));
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
            AttributeRow &row =
                m_map.getAttributeRowFromShapeIndex(static_cast<size_t>(connectedCursor));
            if (seen[static_cast<size_t>(connectedCursor)] > segdepth) {
                float seglength = seglengths[static_cast<size_t>(connectedCursor)];
                int axialref = axialrefs[static_cast<size_t>(connectedCursor)];
                seen[static_cast<size_t>(connectedCursor)] = segdepth;
                audittrail[static_cast<size_t>(connectedCursor)] =
                    TopoMetSegmentRef(connectedCursor, here.dir, here.dist + seglength, here.ref);
                // puts in a suitable bin ahead of us...
                open++;
                //
                if (axialrefs[static_cast<size_t>(here.ref)] == axialref) {
                    list[bin].push_back(connectedCursor);
                    row.setValue(depthCol, static_cast<float>(segdepth));
                } else {
                    list[(bin + 1) % 2].push_back(connectedCursor);
                    seen[static_cast<size_t>(connectedCursor)] =
                        segdepth + 1; // this is so if another node is connected directly to this
                                      // one but is found later it is still handled -- note it can
                                      // result in the connected cursor being added twice
                    row.setValue(depthCol, static_cast<float>(segdepth + 1));
                }
                if (parents.find(static_cast<unsigned int>(connectedCursor)) == parents.end()) {
                    parents[static_cast<unsigned int>(connectedCursor)] =
                        static_cast<unsigned int>(here.ref);
                }
            }
            if (connectedCursor == m_refTo) {
                refFound = true;
                break;
            }
            iter++;
        }
        if (refFound)
            break;
    }

    auto refToParent = parents.find(static_cast<unsigned int>(m_refTo));
    int counter = 0;
    while (refToParent != parents.end()) {
        AttributeRow &row = m_map.getAttributeRowFromShapeIndex(refToParent->first);
        row.setValue(pathCol, static_cast<float>(counter));
        counter++;
        refToParent = parents.find(refToParent->second);
    }
    m_map.getAttributeRowFromShapeIndex(static_cast<size_t>(m_refFrom))
        .setValue(pathCol, static_cast<float>(counter));

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        AttributeRow &row = iter->getRow();
        if (row.getValue(pathCol) < 0) {
            row.setValue(depthCol, -1);
        } else {
            row.setValue(pathCol, static_cast<float>(counter) - row.getValue(pathCol));
        }
    }

    result.completed = true;

    return result;
}
