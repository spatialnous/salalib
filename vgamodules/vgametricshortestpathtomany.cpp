// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricshortestpathtomany.h"

AnalysisResult VGAMetricShortestPathToMany::run(Communicator *) {

    AnalysisResult result;

    auto &attributes = m_map.getAttributeTable();

    // custom linking costs from the attribute table
    std::string link_metric_cost_col_name = "Link Metric Cost";
    int link_metric_cost_col = attributes.getOrInsertColumn(link_metric_cost_col_name);
    result.addAttribute(link_metric_cost_col_name);

    depthmapX::ColumnMatrix<MetricPoint> metricPoints(m_map.getRows(), m_map.getCols());

    for (auto &row : attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        MetricPoint &pnt = getMetricPoint(metricPoints, pix);
        pnt.m_point = &(m_map.getPoint(pix));
        if (link_metric_cost_col != -1) {
            float linkCost = row.getRow().getValue(link_metric_cost_col);
            if (linkCost > 0)
                pnt.m_linkCost += linkCost;
        }
    }

    // in order to calculate Penn angle, the MetricPair becomes a metric triple...
    std::set<MetricTriple> search_list; // contains root point

    for (const PixelRef &pixelFrom : m_pixelsFrom) {
        search_list.insert(MetricTriple(0.0f, pixelFrom, NoPixel));
    }

    // note that m_misc is used in a different manner to analyseGraph / PointDepth
    // here it marks the node as used in calculation only
    std::map<PixelRef, PixelRef> parents;
    std::set<PixelRef> pixelsTo = m_pixelsTo; // consumable set
    while (search_list.size()) {
        std::set<MetricTriple>::iterator it = search_list.begin();
        MetricTriple here = *it;
        search_list.erase(it);
        MetricPoint &mp = getMetricPoint(metricPoints, here.pixel);
        std::set<MetricTriple> newPixels;
        std::set<MetricTriple> mergePixels;
        if (mp.m_unseen || (here.dist < mp.m_dist)) {
            extractMetric(mp.m_point->getNode(), metricPoints, newPixels, &m_map, here);
            mp.m_dist = here.dist;
            mp.m_unseen = false;
            if (!mp.m_point->getMergePixel().empty()) {
                MetricPoint &mp2 = getMetricPoint(metricPoints, mp.m_point->getMergePixel());
                if (mp2.m_unseen || (here.dist + mp2.m_linkCost < mp2.m_dist)) {
                    mp2.m_dist = here.dist + mp2.m_linkCost;
                    mp2.m_unseen = false;

                    auto newTripleIter = newPixels.insert(
                        MetricTriple(mp2.m_dist, mp.m_point->getMergePixel(), NoPixel));
                    extractMetric(mp2.m_point->getNode(), metricPoints, mergePixels, &m_map,
                                  *newTripleIter.first);
                    for (auto &pixel : mergePixels) {
                        parents[pixel.pixel] = mp.m_point->getMergePixel();
                    }
                }
            }
            for (auto &pixel : newPixels) {
                parents[pixel.pixel] = here.pixel;
            }
            newPixels.insert(mergePixels.begin(), mergePixels.end());
            for (auto &pixel : newPixels) {
                auto it = pixelsTo.find(pixel.pixel);
                if (it != pixelsTo.end()) {
                    pixelsTo.erase(it);
                }
            }
            if (pixelsTo.size() != 0)
                search_list.insert(newPixels.begin(), newPixels.end());
        }
    }

    for (const PixelRef &pixelFrom : m_pixelsFrom) {
        getMetricPoint(metricPoints, pixelFrom).m_dist = 0;
    }

    std::map<PixelRef, std::vector<int>> columns;
    for (PixelRef ref : m_pixelsTo) {
        columns[ref].push_back(
            attributes.insertOrResetColumn(getFormattedColumn(Column::METRIC_SHORTEST_PATH, ref)));
    }
    for (PixelRef ref : m_pixelsTo) {
        columns[ref].push_back(attributes.insertOrResetColumn(
            getFormattedColumn(Column::METRIC_SHORTEST_PATH_DISTANCE, ref)));
    }
    for (PixelRef ref : m_pixelsTo) {
        columns[ref].push_back(attributes.insertOrResetColumn(
            getFormattedColumn(Column::METRIC_SHORTEST_PATH_LINKED, ref)));
    }
    for (PixelRef ref : m_pixelsTo) {
        columns[ref].push_back(attributes.insertOrResetColumn(
            getFormattedColumn(Column::METRIC_SHORTEST_PATH_ORDER, ref)));
    }

    for (PixelRef pixelTo : m_pixelsTo) {
        const std::vector<int> &pixelToCols = columns[pixelTo];
        int path_col = pixelToCols[0];
        int dist_col = pixelToCols[1];
        int linked_col = pixelToCols[2];
        int order_col = pixelToCols[3];
        auto pixelToParent = parents.find(pixelTo);
        if (pixelToParent != parents.end()) {

            int counter = 0;
            int linePixelCounter = 0;
            AttributeRow &lastPixelRow = attributes.getRow(AttributeKey(pixelTo));
            MetricPoint &mp = getMetricPoint(metricPoints, pixelTo);
            lastPixelRow.setValue(order_col, counter);
            lastPixelRow.setValue(dist_col, mp.m_dist);
            counter++;
            auto currParent = pixelToParent;
            while (currParent != parents.end()) {
                MetricPoint &mp = getMetricPoint(metricPoints, currParent->second);
                AttributeRow &row = attributes.getRow(AttributeKey(currParent->second));
                row.setValue(order_col, counter);
                row.setValue(dist_col, mp.m_dist);

                if (!mp.m_point->getMergePixel().empty() &&
                    mp.m_point->getMergePixel() == currParent->first) {
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
                        }
                    }
                }

                lastPixelRow = row;
                currParent = parents.find(currParent->second);
                counter++;
            }
        }
    }
    if (m_pixelsTo.size() > 0) {
        result.completed = true;
    }

    return result;
}

void VGAMetricShortestPathToMany::extractMetric(Node n,
                                                depthmapX::ColumnMatrix<MetricPoint> &metricPoints,
                                                std::set<MetricTriple> &pixels, PointMap *pointdata,
                                                const MetricTriple &curs) {
    MetricPoint &cursMP = getMetricPoint(metricPoints, curs.pixel);
    if (curs.dist == 0.0f || cursMP.m_point->blocked() || pointdata->blockedAdjacent(curs.pixel)) {
        for (int i = 0; i < 32; i++) {
            Bin &bin = n.bin(i);
            for (auto pixVec : bin.m_pixel_vecs) {
                for (PixelRef pix = pixVec.start();
                     pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                    MetricPoint &mpt = getMetricPoint(metricPoints, pix);
                    // the nullptr check is unfortunately required because somehow depthmap stores
                    // neighbour pixels that are not really filled..
                    if ((mpt.m_point != nullptr && mpt.m_unseen &&
                         (mpt.m_dist == -1 ||
                          (curs.dist + pointdata->getSpacing() * dist(pix, curs.pixel) <
                           mpt.m_dist)))) {
                        mpt.m_dist =
                            curs.dist + pointdata->getSpacing() * (float)dist(pix, curs.pixel);
                        pixels.insert(MetricTriple(mpt.m_dist, pix, curs.pixel));
                    }
                    pix.move(bin.m_dir);
                }
            }
        }
    }
}
