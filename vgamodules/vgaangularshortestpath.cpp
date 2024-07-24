// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaangularshortestpath.h"

AnalysisResult VGAAngularShortestPath::run(Communicator *) {

    AnalysisResult result;

    auto &attributes = m_map.getAttributeTable();

    std::string path_col_name = Column::ANGULAR_SHORTEST_PATH;
    attributes.insertOrResetColumn(path_col_name);
    result.addAttribute(path_col_name);
    std::string linked_col_name = Column::ANGULAR_SHORTEST_PATH_LINKED;
    attributes.insertOrResetColumn(linked_col_name);
    result.addAttribute(linked_col_name);
    std::string order_col_name = Column::ANGULAR_SHORTEST_PATH_ORDER;
    attributes.insertOrResetColumn(order_col_name);
    result.addAttribute(order_col_name);
    std::string zone_col_name = Column::ANGULAR_SHORTEST_PATH_ZONE;
    attributes.insertOrResetColumn(zone_col_name);
    result.addAttribute(zone_col_name);

    int path_col = attributes.getColumnIndex(path_col_name);
    int linked_col = attributes.getColumnIndex(linked_col_name);
    int order_col = attributes.getColumnIndex(order_col_name);
    int zone_col = attributes.getColumnIndex(zone_col_name);

    for (auto &row : attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        Point &p = m_map.getPoint(pix);
        p.m_misc = 0;
        p.m_dist = 0.0f;
        p.m_cumangle = -1.0f;
    }

    // in order to calculate Penn angle, the MetricPair becomes a metric triple...
    std::set<AngularTriple> search_list; // contains root point

    search_list.insert(AngularTriple(0.0f, m_pixelFrom, NoPixel));
    m_map.getPoint(m_pixelFrom).m_cumangle = 0.0f;

    // note that m_misc is used in a different manner to analyseGraph / PointDepth
    // here it marks the node as used in calculation only
    std::map<PixelRef, PixelRef> parents;
    bool pixelFound = false;
    while (search_list.size()) {
        std::set<AngularTriple>::iterator it = search_list.begin();
        AngularTriple here = *it;
        search_list.erase(it);
        Point &p = m_map.getPoint(here.pixel);
        std::set<AngularTriple> newPixels;
        std::set<AngularTriple> mergePixels;
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (p.filled() && p.m_misc != ~0) {
            p.getNode().extractAngular(newPixels, &m_map, here);
            p.m_misc = ~0;
            if (!p.getMergePixel().empty()) {
                Point &p2 = m_map.getPoint(p.getMergePixel());
                if (p2.m_misc != ~0) {
                    auto newTripleIter =
                        newPixels.insert(AngularTriple(here.angle, p.getMergePixel(), NoPixel));
                    p2.m_cumangle = p.m_cumangle;
                    p2.getNode().extractAngular(mergePixels, &m_map, *newTripleIter.first);
                    for (auto &pixel : mergePixels) {
                        parents[pixel.pixel] = p.getMergePixel();
                    }
                    p2.m_misc = ~0;
                }
            }
        }
        for (auto &pixel : newPixels) {
            parents[pixel.pixel] = here.pixel;
        }
        newPixels.insert(mergePixels.begin(), mergePixels.end());
        for (auto &pixel : newPixels) {
            if (pixel.pixel == m_pixelTo) {
                pixelFound = true;
            }
        }
        if (!pixelFound)
            search_list.insert(newPixels.begin(), newPixels.end());
    }

    int linePixelCounter = 0;
    auto pixelToParent = parents.find(m_pixelTo);
    if (pixelToParent != parents.end()) {

        for (auto &row : attributes) {
            PixelRef pix = PixelRef(row.getKey().value);
            Point &p = m_map.getPoint(pix);
            p.m_misc = 0;
            p.m_dist = 0.0f;
            p.m_cumangle = -1.0f;
        }

        int counter = 0;

        AttributeRow &lastPixelRow = attributes.getRow(AttributeKey(m_pixelTo));
        lastPixelRow.setValue(order_col, counter);
        counter++;
        auto currParent = pixelToParent;
        counter++;
        while (currParent != parents.end()) {
            Point &p = m_map.getPoint(currParent->second);
            AttributeRow &row = attributes.getRow(AttributeKey(currParent->second));
            row.setValue(order_col, counter);

            if (!p.getMergePixel().empty() && p.getMergePixel() == currParent->first) {
                row.setValue(linked_col, 1);
                lastPixelRow.setValue(linked_col, 1);
            } else {
                // apparently we can't just have 1 number in the whole column
                row.setValue(linked_col, 0);
                auto pixelated = m_map.quickPixelateLine(currParent->first, currParent->second);
                for (auto &linePixel : pixelated) {
                    auto *linePixelRow = attributes.getRowPtr(AttributeKey(linePixel));
                    if (linePixelRow != 0) {
                        linePixelRow->setValue(path_col, linePixelCounter++);
                        linePixelRow->setValue(zone_col, 1);

                        std::set<AngularTriple> newPixels;
                        Point &p = m_map.getPoint(linePixel);
                        p.getNode().extractAngular(newPixels, &m_map,
                                                   AngularTriple(0.0f, linePixel, NoPixel));
                        for (auto &zonePixel : newPixels) {
                            auto *zonePixelRow =
                                attributes.getRowPtr(AttributeKey(zonePixel.pixel));
                            if (zonePixelRow != 0) {
                                double zoneLineDist = dist(linePixel, zonePixel.pixel);
                                float currZonePixelVal = zonePixelRow->getValue(zone_col);
                                if (currZonePixelVal == -1 ||
                                    1.0f / (zoneLineDist + 1) > currZonePixelVal) {
                                    zonePixelRow->setValue(zone_col, 1.0f / (zoneLineDist + 1));
                                }
                                m_map.getPoint(zonePixel.pixel).m_misc = 0;
                                m_map.getPoint(zonePixel.pixel).m_extent = zonePixel.pixel;
                            }
                        }
                    }
                }
            }

            lastPixelRow = row;
            currParent = parents.find(currParent->second);
            counter++;
        }

        result.completed = true;

        return result;
    }

    return result;
}
