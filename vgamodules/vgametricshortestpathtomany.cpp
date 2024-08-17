// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricshortestpathtomany.h"

AnalysisResult VGAMetricShortestPathToMany::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    // custom linking costs from the attribute table
    std::string linkMetricCostColName = Column::LINK_METRIC_COST;

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes, linkMetricCostColName);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);

    auto [parents] = traverseFindMany(analysisData, graph, refs, m_pixelsFrom, m_pixelsTo);

    //    // in order to calculate Penn angle, the MetricPair becomes a metric triple...
    //    std::set<MetricTriple> search_list; // contains root point

    //    for (const PixelRef &pixelFrom : m_pixelsFrom) {
    //        search_list.insert(MetricTriple(0.0f, pixelFrom, NoPixel));
    //    }

    //    // note that m_misc is used in a different manner to analyseGraph / PointDepth
    //    // here it marks the node as used in calculation only
    //    std::map<PixelRef, PixelRef> parents;
    //    std::set<PixelRef> pixelsTo = m_pixelsTo; // consumable set
    //    while (search_list.size()) {
    //        std::set<MetricTriple>::iterator it = search_list.begin();
    //        MetricTriple here = *it;
    //        search_list.erase(it);
    //        MetricPoint &mp = getMetricPoint(metricPoints, here.pixel);
    //        std::set<MetricTriple> newPixels;
    //        std::set<MetricTriple> mergePixels;
    //        if (mp.m_unseen || (here.dist < mp.m_dist)) {
    //            extractMetric(mp.m_point->getNode(), metricPoints, newPixels, &m_map, here);
    //            mp.m_dist = here.dist;
    //            mp.m_unseen = false;
    //            if (!mp.m_point->getMergePixel().empty()) {
    //                MetricPoint &mp2 = getMetricPoint(metricPoints, mp.m_point->getMergePixel());
    //                if (mp2.m_unseen || (here.dist + mp2.m_linkCost < mp2.m_dist)) {
    //                    mp2.m_dist = here.dist + mp2.m_linkCost;
    //                    mp2.m_unseen = false;

    //                    auto newTripleIter = newPixels.insert(
    //                        MetricTriple(mp2.m_dist, mp.m_point->getMergePixel(), NoPixel));
    //                    extractMetric(mp2.m_point->getNode(), metricPoints, mergePixels, &m_map,
    //                                  *newTripleIter.first);
    //                    for (auto &pixel : mergePixels) {
    //                        parents[pixel.pixel] = mp.m_point->getMergePixel();
    //                    }
    //                }
    //            }
    //            for (auto &pixel : newPixels) {
    //                parents[pixel.pixel] = here.pixel;
    //            }
    //            newPixels.insert(mergePixels.begin(), mergePixels.end());
    //            for (auto &pixel : newPixels) {
    //                auto it = pixelsTo.find(pixel.pixel);
    //                if (it != pixelsTo.end()) {
    //                    pixelsTo.erase(it);
    //                }
    //            }
    //            if (pixelsTo.size() != 0)
    //                search_list.insert(newPixels.begin(), newPixels.end());
    //        }
    //    }

    for (const PixelRef &pixelFrom : m_pixelsFrom) {
        analysisData.at(getRefIdx(refs, pixelFrom)).m_dist = 0.0f;
    }

    std::vector<std::string> colNames;
    { // create all columns
        for (PixelRef ref : m_pixelsTo) {
            colNames.push_back(getFormattedColumn(Column::METRIC_SHORTEST_PATH, ref));
        }
        for (PixelRef ref : m_pixelsTo) {
            colNames.push_back(getFormattedColumn(Column::METRIC_SHORTEST_PATH_DISTANCE, ref));
        }
        for (PixelRef ref : m_pixelsTo) {
            colNames.push_back(getFormattedColumn(Column::METRIC_SHORTEST_PATH_LINKED, ref));
        }
        for (PixelRef ref : m_pixelsTo) {
            colNames.push_back(getFormattedColumn(Column::METRIC_SHORTEST_PATH_ORDER, ref));
        }
    }

    AnalysisResult result(std::move(colNames), attributes.getNumRows());

    std::map<PixelRef, std::vector<int>> columns;
    {
        for (PixelRef ref : m_pixelsTo) {
            columns[ref].push_back(
                result.getColumnIndex(getFormattedColumn(Column::METRIC_SHORTEST_PATH, ref)));
        }
        for (PixelRef ref : m_pixelsTo) {
            columns[ref].push_back(result.getColumnIndex(
                getFormattedColumn(Column::METRIC_SHORTEST_PATH_DISTANCE, ref)));
        }
        for (PixelRef ref : m_pixelsTo) {
            columns[ref].push_back(result.getColumnIndex(
                getFormattedColumn(Column::METRIC_SHORTEST_PATH_LINKED, ref)));
        }
        for (PixelRef ref : m_pixelsTo) {
            columns[ref].push_back(
                result.getColumnIndex(getFormattedColumn(Column::METRIC_SHORTEST_PATH_ORDER, ref)));
        }
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
            auto *lad = &analysisData.at(getRefIdx(refs, pixelTo));
            result.setValue(lad->m_attributeDataRow, order_col, counter);
            result.setValue(lad->m_attributeDataRow, dist_col, lad->m_dist);

            counter++;
            auto currParent = pixelToParent;
            counter++;
            while (currParent != parents.end()) {
                auto &ad = analysisData.at(getRefIdx(refs, currParent->second));
                auto &p = ad.m_point;
                result.setValue(ad.m_attributeDataRow, order_col, counter);
                result.setValue(ad.m_attributeDataRow, dist_col, ad.m_dist);

                if (!p.getMergePixel().empty() && p.getMergePixel() == currParent->first) {
                    result.setValue(ad.m_attributeDataRow, linked_col, 1);
                    result.setValue(lad->m_attributeDataRow, linked_col, 1);
                } else {
                    // apparently we can't just have 1 number in the whole column
                    result.setValue(ad.m_attributeDataRow, linked_col, 0);
                    auto pixelated = m_map.quickPixelateLine(currParent->first, currParent->second);
                    for (auto &linePixel : pixelated) {
                        auto *linePixelRow = attributes.getRowPtr(AttributeKey(linePixel));
                        if (linePixelRow != 0) {
                            auto &lpad = analysisData.at(getRefIdx(refs, linePixel));
                            result.setValue(lpad.m_attributeDataRow, path_col, linePixelCounter++);
                        }
                    }
                }

                lad = &ad;
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
