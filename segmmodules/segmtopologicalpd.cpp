// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmtopologicalpd.hpp"

#include "segmhelpers.hpp"

AnalysisResult SegmentTopologicalPD::run(Communicator *, ShapeGraph &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    AnalysisResult result;

    // record axial line refs for topological analysis
    std::vector<int> axialrefs;
    // quick through to find the longest seg length
    std::vector<float> seglengths;
    float maxseglength = 0.0f;
    for (size_t cursor = 0; cursor < map.getShapeCount(); cursor++) {
        AttributeRow &row = map.getAttributeRowFromShapeIndex(cursor);
        axialrefs.push_back(static_cast<int>(row.getValue("Axial Line Ref")));
        seglengths.push_back(row.getValue("Segment Length"));
        if (seglengths.back() > maxseglength) {
            maxseglength = seglengths.back();
        }
    }

    int maxbin = 2;

    auto sdColIdx = attributes.insertOrResetColumn(Column::TOPOLOGICAL_STEP_DEPTH);
    result.addAttribute(Column::TOPOLOGICAL_STEP_DEPTH);

    std::vector<unsigned int> seen(map.getShapeCount());
    std::vector<TopoMetSegmentRef> audittrail(map.getShapeCount());
    std::vector<int> list[512]; // 512 bins!
    int open = 0;

    for (size_t i = 0; i < map.getShapeCount(); i++) {
        seen[i] = 0xffffffff;
    }
    for (auto &cursor : m_originRefs) {
        seen[static_cast<size_t>(cursor)] = 0;
        open++;
        double length = seglengths[static_cast<size_t>(cursor)];
        audittrail[static_cast<size_t>(cursor)] =
            TopoMetSegmentRef(cursor, Connector::SEG_CONN_ALL, length * 0.5, -1);
        list[0].push_back(cursor);
        attributes.getRow(AttributeKey(cursor)).setValue(sdColIdx, 0);
    }

    unsigned int segdepth = 0;
    int bin = 0;

    while (open != 0) {
        while (list[bin].size() == 0) {
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

        Connector &axline = map.getConnections().at(static_cast<size_t>(here.ref));
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
                map.getAttributeRowFromShapeIndex(static_cast<size_t>(connectedCursor));
            if (seen[static_cast<size_t>(connectedCursor)] > segdepth) {
                float length = seglengths[static_cast<size_t>(connectedCursor)];
                int axialref = axialrefs[static_cast<size_t>(connectedCursor)];
                seen[static_cast<size_t>(connectedCursor)] = segdepth;
                audittrail[static_cast<size_t>(connectedCursor)] =
                    TopoMetSegmentRef(connectedCursor, here.dir, here.dist + length, here.ref);
                // puts in a suitable bin ahead of us...
                open++;
                //
                if (axialrefs[static_cast<size_t>(here.ref)] == axialref) {
                    list[bin].push_back(connectedCursor);
                    row.setValue(sdColIdx, static_cast<float>(segdepth));
                } else {
                    list[(bin + 1) % 2].push_back(connectedCursor);
                    seen[static_cast<size_t>(connectedCursor)] =
                        segdepth + 1; // this is so if another node is connected directly to this
                                      // one but is found later it is still handled -- note it can
                                      // result in the connected cursor being added twice
                    row.setValue(sdColIdx, static_cast<float>(segdepth + 1));
                }
            }
            iter++;
        }
    }

    result.completed = true;

    return result;
}
