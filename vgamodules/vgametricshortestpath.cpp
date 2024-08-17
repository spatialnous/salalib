// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricshortestpath.h"

AnalysisResult VGAMetricShortestPath::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    // custom linking costs from the attribute table
    std::string linkMetricCostColName = Column::LINK_METRIC_COST;
    std::string path_col_name = Column::METRIC_SHORTEST_PATH;
    std::string dist_col_name = Column::METRIC_SHORTEST_PATH_DISTANCE;
    std::string linked_col_name = Column::METRIC_SHORTEST_PATH_LINKED;
    std::string order_col_name = Column::METRIC_SHORTEST_PATH_ORDER;
    std::string zone_col_name = Column::METRIC_SHORTEST_PATH_VISUAL_ZONE;
    std::string metricZone_col_name = Column::METRIC_SHORTEST_PATH_METRIC_ZONE;
    std::string invMetricZone_col_name = Column::METRIC_SHORTEST_PATH_INV_METRIC_ZONE;

    AnalysisResult result({path_col_name, dist_col_name, linked_col_name, order_col_name,
                           zone_col_name, metricZone_col_name, invMetricZone_col_name},
                          attributes.getNumRows());

    int path_col = result.getColumnIndex(path_col_name);
    int dist_col = result.getColumnIndex(dist_col_name);
    int linked_col = result.getColumnIndex(linked_col_name);
    int order_col = result.getColumnIndex(order_col_name);
    int visualZoneColIdx = result.getColumnIndex(zone_col_name);
    int metricZoneColIdx = result.getColumnIndex(metricZone_col_name);
    int invMetricZoneColIdx = result.getColumnIndex(invMetricZone_col_name);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes, linkMetricCostColName);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);
    auto [parents] = traverseFind(analysisData, graph, refs, m_pixelsFrom, m_pixelTo);

    int linePixelCounter = 0;
    auto pixelToParent = parents.find(m_pixelTo);
    if (pixelToParent != parents.end()) {

        for (auto &adt : analysisData) {
            adt.m_visitedFromBin = 0;
            result.setValue(adt.m_attributeDataRow, dist_col, adt.m_dist);
            adt.m_dist = -1.0f;
        }

        int counter = 0;

        for (const PixelRef &pixelFrom : m_pixelsFrom) {
            auto adt = analysisData.at(getRefIdx(refs, pixelFrom));
            result.setValue(adt.m_attributeDataRow, dist_col, 0);
        }

        auto *lad = &analysisData.at(getRefIdx(refs, m_pixelTo));
        result.setValue(lad->m_attributeDataRow, order_col, counter);

        counter++;
        auto currParent = pixelToParent;
        counter++;
        while (currParent != parents.end()) {
            auto &ad = analysisData.at(getRefIdx(refs, currParent->second));
            auto &p = ad.m_point;
            result.setValue(ad.m_attributeDataRow, order_col, counter);

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
                        result.setValue(lpad.m_attributeDataRow, visualZoneColIdx, 0);
                        result.setValue(lpad.m_attributeDataRow, metricZoneColIdx, 0);
                        result.setValue(lpad.m_attributeDataRow, invMetricZoneColIdx, 1);

                        std::set<MetricSearchData> newPixels;
                        extractMetric(graph.at(lpad.m_attributeDataRow), newPixels, m_map,
                                      MetricSearchData(lpad, 0.0f, std::nullopt));
                        for (auto &zonePixel : newPixels) {
                            auto &zad = zonePixel.pixel;
                            if (result.getValue(zad.m_attributeDataRow, visualZoneColIdx) == -1) {
                                result.setValue(zad.m_attributeDataRow, visualZoneColIdx,
                                                linePixelCounter);
                            }

                            double zoneLineDist = dist(linePixel, zad.m_ref) * m_map.getSpacing();
                            {
                                float currMetricZonePixelVal =
                                    result.getValue(zad.m_attributeDataRow, metricZoneColIdx);
                                if (currMetricZonePixelVal == -1 ||
                                    zoneLineDist < currMetricZonePixelVal) {
                                    result.setValue(zad.m_attributeDataRow, metricZoneColIdx,
                                                    zoneLineDist);
                                }
                            }
                            {
                                float currInvMetricZonePixelVal =
                                    result.getValue(zad.m_attributeDataRow, invMetricZoneColIdx);
                                if (currInvMetricZonePixelVal == -1 ||
                                    1.0f / (zoneLineDist + 1) > currInvMetricZonePixelVal) {
                                    result.setValue(zad.m_attributeDataRow, invMetricZoneColIdx,
                                                    1.0f / (zoneLineDist + 1));
                                }
                            }
                            zad.m_visitedFromBin = 0;
                        }
                    }
                }
            }

            lad = &ad;
            currParent = parents.find(currParent->second);
            counter++;
        }

        result.completed = true;

        return result;
    }

    return result;
}
