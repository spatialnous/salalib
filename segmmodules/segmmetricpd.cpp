// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmmetricpd.h"

#include "segmhelpers.h"

AnalysisResult SegmentMetricPD::run(Communicator *, ShapeGraph &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    AnalysisResult result;

    // record axial line refs for topological analysis
    std::vector<int> axialrefs;
    // quick through to find the longest seg length
    std::vector<float> seglengths;
    float maxseglength = 0.0f;
    for (size_t cursor = 0; cursor < map.getShapeCount(); cursor++) {
        AttributeRow &row = map.getAttributeRowFromShapeIndex(cursor);
        axialrefs.push_back(row.getValue("Axial Line Ref"));
        seglengths.push_back(row.getValue("Segment Length"));
        if (seglengths.back() > maxseglength) {
            maxseglength = seglengths.back();
        }
    }

    int maxbin;
    std::string prefix;
    prefix = "Metric ";
    maxbin = 512;
    std::string depthcol = prefix + "Step Depth";
    attributes.insertOrResetColumn(depthcol.c_str());
    result.addAttribute(depthcol);

    std::vector<unsigned int> seen(map.getShapeCount());
    std::vector<TopoMetSegmentRef> audittrail(map.getShapeCount());
    std::vector<int> list[512]; // 512 bins!
    int open = 0;

    for (size_t i = 0; i < map.getShapeCount(); i++) {
        seen[i] = 0xffffffff;
    }
    for (auto &cursor : map.getSelSet()) {
        seen[cursor] = 0;
        open++;
        double length = seglengths[cursor];
        audittrail[cursor] = TopoMetSegmentRef(cursor, Connector::SEG_CONN_ALL, length * 0.5, -1);
        // better to divide by 511 but have 512 bins...
        list[(int(floor(0.5 + 511 * length / maxseglength))) % 512].push_back(cursor);
        AttributeRow &row = map.getAttributeRowFromShapeIndex(cursor);
        row.setValue(depthcol.c_str(), 0);
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
        TopoMetSegmentRef &here = audittrail[list[bin].back()];
        list[bin].pop_back();
        open--;
        // this is necessary using unsigned ints for "seen", as it is possible to add a node twice
        if (here.done) {
            continue;
        } else {
            here.done = true;
        }

        Connector &axline = map.getConnections().at(here.ref);
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
            if (seen[connected_cursor] > segdepth) {
                float length = seglengths[connected_cursor];
                seen[connected_cursor] = segdepth;
                audittrail[connected_cursor] =
                    TopoMetSegmentRef(connected_cursor, here.dir, here.dist + length, here.ref);
                // puts in a suitable bin ahead of us...
                open++;
                //
                // better to divide by 511 but have 512 bins...
                list[(bin + int(floor(0.5 + 511 * length / maxseglength))) % 512].push_back(
                    connected_cursor);
                AttributeRow &row = map.getAttributeRowFromShapeIndex(connected_cursor);
                row.setValue(depthcol.c_str(), here.dist + length * 0.5);
            }
            iter++;
        }
    }

    map.setDisplayedAttribute(attributes.getColumnIndex(depthcol.c_str()));

    result.completed = true;

    return result;
}
