// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmtulipdepth.h"

// revised to use tulip bins for faster analysis of large spaces

AnalysisResult SegmentTulipDepth::run(Communicator *, ShapeGraph &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    AnalysisResult result;

    int stepdepthCol = attributes.insertOrResetColumn(Column::ANGULAR_STEP_DEPTH);
    result.addAttribute(Column::ANGULAR_STEP_DEPTH);

    // The original code set tulip_bins to 1024, divided by two and added one
    // in order to duplicate previous code (using a semicircle of tulip bins)
    size_t tulipBins = (m_tulipBins / 2) + 1;

    std::vector<bool> covered(map.getConnections().size());
    for (size_t i = 0; i < map.getConnections().size(); i++) {
        covered[i] = false;
    }
    std::vector<std::vector<SegmentData>> bins(tulipBins);

    int opencount = 0;
    for (auto &sel : m_originRefs) {
        int row = depthmapX::getMapAtIndex(map.getAllShapes(), sel)->first;
        if (row != -1) {
            bins[0].push_back(SegmentData(0, row, SegmentRef(), 0, 0.0, 0));
            opencount++;
        }
    }
    int depthlevel = 0;
    auto binIter = bins.begin();
    int currentbin = 0;
    while (opencount) {
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
            int curr = pafmath::pafrand() % binIter->size();
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
            Connector &line = map.getConnections()[lineindex.ref];
            // convert depth from tulip_bins normalised to standard angle
            // (note the -1)
            double depthToLine = depthlevel / ((tulipBins - 1) * 0.5);
            map.getAttributeRowFromShapeIndex(lineindex.ref).setValue(stepdepthCol, depthToLine);
            int extradepth;
            if (lineindex.dir != -1) {
                for (auto &segconn : line.forwardSegconns) {
                    if (!covered[segconn.first.ref]) {
                        extradepth = (int)floor(segconn.second * tulipBins * 0.5);
                        bins[(currentbin + tulipBins + extradepth) % tulipBins].push_back(
                            SegmentData(segconn.first, lineindex.ref, lineindex.segdepth + 1, 0.0,
                                        0));
                        opencount++;
                    }
                }
            }
            if (lineindex.dir != 1) {
                for (auto &segconn : line.backSegconns) {
                    if (!covered[segconn.first.ref]) {
                        extradepth = (int)floor(segconn.second * tulipBins * 0.5);
                        bins[(currentbin + tulipBins + extradepth) % tulipBins].push_back(
                            SegmentData(segconn.first, lineindex.ref, lineindex.segdepth + 1, 0.0,
                                        0));
                        opencount++;
                    }
                }
            }
        }
    }

    result.completed = true;

    return result;
}
