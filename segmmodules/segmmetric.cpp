// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmmetric.hpp"

#include "segmhelpers.hpp"

AnalysisResult SegmentMetric::run(Communicator *comm, ShapeGraph &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    AnalysisResult result;

    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(
            Communicator::NUM_RECORDS,
            (m_selSet.has_value() ? m_selSet.value().size() : map.getConnections().size()));
    }
    size_t reccount = 0;

    // record axial line refs for topological analysis
    std::vector<int> axialrefs;
    // quick through to find the longest seg length
    std::vector<float> seglengths;
    float maxseglength = 0.0f;
    for (size_t cursor = 0; cursor < map.getShapeCount(); cursor++) {
        AttributeRow &row = map.getAttributeRowFromShapeIndex(cursor);
        axialrefs.push_back(
            static_cast<int>(row.getValue(attributes.getColumnIndex("Axial Line Ref"))));
        seglengths.push_back(row.getValue(attributes.getColumnIndex("Segment Length")));
        if (seglengths.back() > maxseglength) {
            maxseglength = seglengths.back();
        }
    }

    int maxbin = 512;

    std::string choicecol = getFormattedColumn(Column::METRIC_CHOICE, m_radius);
    std::string wchoicecol = getFormattedColumn(Column::METRIC_CHOICE_SLW, m_radius);
    std::string meandepthcol = getFormattedColumn(Column::METRIC_MEAN_DEPTH, m_radius);
    std::string wmeandepthcol = getFormattedColumn(Column::METRIC_MEAN_DEPTH_SLW, m_radius);
    std::string totaldcol = getFormattedColumn(Column::METRIC_TOTAL_DEPTH, m_radius);
    std::string totalcol = getFormattedColumn(Column::METRIC_TOTAL_NODES, m_radius);
    std::string wtotalcol = getFormattedColumn(Column::METRIC_TOTAL_LENGTH, m_radius);

    if (!m_selSet.has_value()) {
        attributes.insertOrResetColumn(choicecol.c_str());
        result.addAttribute(choicecol);
        attributes.insertOrResetColumn(wchoicecol.c_str());
        result.addAttribute(wchoicecol);
    }
    attributes.insertOrResetColumn(meandepthcol.c_str());
    result.addAttribute(meandepthcol);
    attributes.insertOrResetColumn(wmeandepthcol.c_str());
    result.addAttribute(wmeandepthcol);
    attributes.insertOrResetColumn(totaldcol.c_str());
    result.addAttribute(totaldcol);
    attributes.insertOrResetColumn(totalcol.c_str());
    result.addAttribute(totalcol);
    attributes.insertOrResetColumn(wtotalcol.c_str());
    result.addAttribute(wtotalcol);
    //
    std::vector<unsigned int> seen(map.getShapeCount());
    std::vector<TopoMetSegmentRef> audittrail(map.getShapeCount());
    std::vector<TopoMetSegmentChoice> choicevals(map.getShapeCount());
    for (size_t cursor = 0; cursor < map.getShapeCount(); cursor++) {
        AttributeRow &row = map.getAttributeRowFromShapeIndex(cursor);
        auto &shapeRef = map.getShapeRefFromIndex(cursor);
        if (m_selSet.has_value() &&
            m_selSet.value().find(shapeRef->first) == m_selSet.value().end()) {
            continue;
        }
        for (size_t i = 0; i < map.getShapeCount(); i++) {
            seen[i] = 0xffffffff;
        }
        std::vector<int> list[512]; // 512 bins!
        int bin = 0;
        list[bin].push_back(static_cast<int>(cursor));
        double rootseglength = seglengths[cursor];
        audittrail[cursor] = TopoMetSegmentRef(static_cast<int>(cursor), Connector::SEG_CONN_ALL,
                                               rootseglength * 0.5, -1);
        int open = 1;
        unsigned int segdepth = 0;
        double total = 0.0, wtotal = 0.0, wtotaldepth = 0.0, totalmetdepth = 0.0;
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
            //
            if (here.done) {
                continue;
            } else {
                here.done = true;
            }
            //
            double len = seglengths[static_cast<size_t>(here.ref)];
            totalmetdepth += here.dist - len * 0.5; // preloaded with length ahead
            wtotal += len;
            wtotaldepth += len * (here.dist - len * 0.5);
            total += 1;
            //
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

                if (seen[static_cast<size_t>(connectedCursor)] > segdepth &&
                    static_cast<size_t>(connectedCursor) != cursor) {
                    bool seenalready =
                        (seen[static_cast<size_t>(connectedCursor)] == 0xffffffff) ? false : true;
                    float length = seglengths[static_cast<size_t>(connectedCursor)];
                    audittrail[static_cast<size_t>(connectedCursor)] =
                        TopoMetSegmentRef(connectedCursor, here.dir, here.dist + length, here.ref);
                    seen[static_cast<size_t>(connectedCursor)] = segdepth;
                    if (m_radius == -1 || here.dist + length < m_radius) {
                        // puts in a suitable bin ahead of us...
                        open++;
                        //
                        // better to divide by 511 but have 512 bins...
                        list[(bin + static_cast<int>(floor(0.5 + 511 * length / maxseglength))) %
                             512]
                            .push_back(connectedCursor);
                    }
                    // not sure why this is outside the radius restriction
                    // (sel_only: with restricted selection set, not all lines will be labelled)
                    // (seenalready: need to check that we're not doing this twice, given the seen
                    // can go twice)

                    // Quick mod - TV
                    if (!m_selSet.has_value() && connectedCursor > static_cast<int>(cursor) &&
                        !seenalready) { // only one way paths, saves doing this twice
                        int subcur = connectedCursor;
                        while (subcur != -1) {
                            // in this method of choice, start and end lines are included
                            choicevals[static_cast<size_t>(subcur)].choice += 1;
                            choicevals[static_cast<size_t>(subcur)].wchoice +=
                                (rootseglength * length);
                            subcur = audittrail[static_cast<size_t>(subcur)].previous;
                        }
                    }
                }
                iter++;
            }
        }
        // also put in mean depth:
        //
        row.setValue(meandepthcol.c_str(), static_cast<float>(totalmetdepth / (total - 1)));
        row.setValue(totaldcol.c_str(), static_cast<float>(totalmetdepth));
        row.setValue(wmeandepthcol.c_str(),
                     static_cast<float>(wtotaldepth / (wtotal - rootseglength)));
        row.setValue(totalcol.c_str(), static_cast<float>(total));
        row.setValue(wtotalcol.c_str(), static_cast<float>(wtotal));
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
            }
            comm->CommPostMessage(Communicator::CURRENT_RECORD, reccount);
        }
        reccount++;
    }
    if (!m_selSet.has_value()) {
        // note, I've stopped sel only from calculating choice values:
        for (size_t cursor = 0; cursor < map.getShapeCount(); cursor++) {
            AttributeRow &row = map.getAttributeRowFromShapeIndex(cursor);
            row.setValue(choicecol.c_str(), static_cast<float>(choicevals[cursor].choice));
            row.setValue(wchoicecol.c_str(), static_cast<float>(choicevals[cursor].wchoice));
        }
    }

    result.completed = true;

    return result;
}
