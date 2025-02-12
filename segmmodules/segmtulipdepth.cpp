// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmtulipdepth.hpp"

// revised to use tulip bins for faster analysis of large spaces

AnalysisResult SegmentTulipDepth::run(Communicator *, ShapeGraph &map, bool) {

    AttributeTable &attributes = map.getAttributeTable();

    AnalysisResult result;

    auto stepdepthCol = attributes.insertOrResetColumn(Column::ANGULAR_STEP_DEPTH);
    result.addAttribute(Column::ANGULAR_STEP_DEPTH);

    // The original code set tulip_bins to 1024, divided by two and added one
    // in order to duplicate previous code (using a semicircle of tulip bins)
    size_t tulipBins = static_cast<size_t>(m_tulipBins / 2) + 1;

    std::vector<bool> covered(map.getConnections().size());
    for (size_t i = 0; i < map.getConnections().size(); i++) {
        covered[i] = false;
    }
    std::vector<std::vector<SegmentData>> bins(tulipBins);

    int opencount = 0;
    for (auto &sel : m_originRefs) {
        int row = depthmapX::getMapAtIndex(map.getAllShapes(), static_cast<size_t>(sel))->first;
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
            auto curr = static_cast<int>(pafmath::pafrand() % binIter->size());
            auto currIter = binIter->begin() + curr;
            lineindex = *currIter;
            binIter->erase(currIter);
            // note: do not clear choice values here!
        } else {
            lineindex = binIter->front();
            binIter->pop_back();
        }
        opencount--;
        if (!covered[static_cast<size_t>(lineindex.ref)]) {
            covered[static_cast<size_t>(lineindex.ref)] = true;
            Connector &line = map.getConnections()[static_cast<size_t>(lineindex.ref)];
            // convert depth from tulip_bins normalised to standard angle
            // (note the -1)
            double depthToLine = depthlevel / (static_cast<float>(tulipBins - 1) * 0.5);
            map.getAttributeRowFromShapeIndex(static_cast<size_t>(lineindex.ref))
                .setValue(stepdepthCol, static_cast<float>(depthToLine));
            int extradepth;
            if (lineindex.dir != -1) {
                for (auto &segconn : line.forwardSegconns) {
                    if (!covered[static_cast<size_t>(segconn.first.ref)]) {
                        extradepth = static_cast<int>(
                            floor(segconn.second * static_cast<float>(tulipBins) * 0.5));
                        bins[(static_cast<size_t>(currentbin) + tulipBins +
                              static_cast<size_t>(extradepth)) %
                             tulipBins]
                            .push_back(SegmentData(segconn.first,
                                                   static_cast<int8_t>(lineindex.ref),
                                                   lineindex.segdepth + 1, 0.0, 0));
                        opencount++;
                    }
                }
            }
            if (lineindex.dir != 1) {
                for (auto &segconn : line.backSegconns) {
                    if (!covered[static_cast<size_t>(segconn.first.ref)]) {
                        extradepth = static_cast<int>(
                            floor(segconn.second * static_cast<float>(tulipBins) * 0.5));
                        bins[(static_cast<size_t>(currentbin) + tulipBins +
                              static_cast<size_t>(extradepth)) %
                             tulipBins]
                            .push_back(SegmentData(segconn.first,
                                                   static_cast<int8_t>(lineindex.ref),
                                                   lineindex.segdepth + 1, 0.0, 0));
                        opencount++;
                    }
                }
            }
        }
    }

    result.completed = true;

    return result;
}
