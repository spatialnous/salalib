// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmtulipshortestpath.hpp"

// revised to use tulip bins for faster analysis of large spaces

AnalysisResult SegmentTulipShortestPath::run(Communicator *) {

    AnalysisResult result;

    AttributeTable &attributes = m_map.getAttributeTable();

    size_t angleCol = attributes.insertOrResetColumn(Column::ANGULAR_SHORTEST_PATH_ANGLE);
    result.addAttribute(Column::ANGULAR_SHORTEST_PATH_ANGLE);
    size_t pathCol = attributes.insertOrResetColumn(Column::ANGULAR_SHORTEST_PATH_ORDER);
    result.addAttribute(Column::ANGULAR_SHORTEST_PATH_ORDER);

    size_t tulipBins = m_tulipBins;

    tulipBins /= 2; // <- actually use semicircle of tulip bins
    tulipBins += 1;

    std::vector<bool> covered(m_map.getConnections().size());
    for (size_t i = 0; i < m_map.getConnections().size(); i++) {
        covered[i] = false;
    }
    std::vector<std::vector<SegmentData>> bins(tulipBins);

    int opencount = 0;

    int row = static_cast<int>(
        std::distance(m_map.getAllShapes().begin(), m_map.getAllShapes().find(m_refFrom)));
    if (row != -1) {
        bins[0].push_back(SegmentData(0, row, SegmentRef(), 0, 0.0, 0));
        opencount++;
    }

    std::map<unsigned int, unsigned int> parents;

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
            auto curr = pafmath::pafrand() % binIter->size();
            auto currIter = binIter->begin() + static_cast<long>(curr);
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
            Connector &line = m_map.getConnections()[static_cast<size_t>(lineindex.ref)];
            // convert depth from tulip_bins normalised to standard angle
            // (note the -1)
            double depthToLine = depthlevel / (static_cast<double>(tulipBins - 1) * 0.5);
            m_map.getAttributeRowFromShapeIndex(static_cast<size_t>(lineindex.ref))
                .setValue(angleCol, static_cast<float>(depthToLine));
            int extradepth;
            if (lineindex.dir != -1) {
                for (auto &segconn : line.forwardSegconns) {
                    if (!covered[static_cast<size_t>(segconn.first.ref)]) {
                        extradepth = static_cast<int>(
                            floor(segconn.second * static_cast<double>(tulipBins) * 0.5));
                        bins[(static_cast<size_t>(currentbin) + tulipBins +
                              static_cast<size_t>(extradepth)) %
                             tulipBins]
                            .push_back(SegmentData(segconn.first,
                                                   static_cast<int8_t>(lineindex.ref),
                                                   lineindex.segdepth + 1, 0.0, 0));
                        if (parents.find(static_cast<unsigned int>(segconn.first.ref)) ==
                            parents.end()) {
                            parents[static_cast<unsigned int>(segconn.first.ref)] =
                                static_cast<unsigned int>(lineindex.ref);
                        }
                        opencount++;
                    }
                }
            }
            if (lineindex.dir != 1) {
                for (auto &segconn : line.backSegconns) {
                    if (!covered[static_cast<size_t>(segconn.first.ref)]) {
                        extradepth = static_cast<int>(
                            floor(segconn.second * static_cast<double>(tulipBins) * 0.5));
                        bins[(static_cast<size_t>(currentbin) + tulipBins +
                              static_cast<size_t>(extradepth)) %
                             tulipBins]
                            .push_back(SegmentData(segconn.first,
                                                   static_cast<int8_t>(lineindex.ref),
                                                   lineindex.segdepth + 1, 0.0, 0));
                        if (parents.find(static_cast<unsigned int>(segconn.first.ref)) ==
                            parents.end()) {
                            parents[static_cast<unsigned int>(segconn.first.ref)] =
                                static_cast<unsigned int>(lineindex.ref);
                        }
                        opencount++;
                    }
                }
            }
            if (lineindex.ref == m_refTo) {
                break;
            }
        }
    }

    auto refToParent = parents.find(static_cast<unsigned int>(m_refTo));
    int counter = 0;
    while (refToParent != parents.end()) {
        AttributeRow &attrRow = m_map.getAttributeRowFromShapeIndex(refToParent->first);
        attrRow.setValue(pathCol, static_cast<float>(counter));
        counter++;
        refToParent = parents.find(refToParent->second);
    }
    m_map.getAttributeRowFromShapeIndex(static_cast<size_t>(m_refFrom))
        .setValue(pathCol, static_cast<float>(counter));

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        AttributeRow &attrRow = iter->getRow();
        if (attrRow.getValue(pathCol) < 0) {
            attrRow.setValue(angleCol, -1);
        } else {
            attrRow.setValue(pathCol, static_cast<float>(counter) - attrRow.getValue(pathCol));
        }
    }

    result.completed = true;

    return result;
}
