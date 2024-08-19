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
    std::string pathColName = Column::METRIC_SHORTEST_PATH;
    std::string distColName = Column::METRIC_SHORTEST_PATH_DISTANCE;
    std::string linkedColName = Column::METRIC_SHORTEST_PATH_LINKED;
    std::string orderColName = Column::METRIC_SHORTEST_PATH_ORDER;
    std::string zoneColName = Column::METRIC_SHORTEST_PATH_VISUAL_ZONE;
    std::string metricZoneColName = Column::METRIC_SHORTEST_PATH_METRIC_ZONE;
    std::string invMetricZoneColName = Column::METRIC_SHORTEST_PATH_INV_METRIC_ZONE;

    AnalysisResult result({pathColName, distColName, linkedColName, orderColName, zoneColName,
                           metricZoneColName, invMetricZoneColName},
                          attributes.getNumRows());

    int pathCol = result.getColumnIndex(pathColName);
    int distCol = result.getColumnIndex(distColName);
    int linkedCol = result.getColumnIndex(linkedColName);
    int orderCol = result.getColumnIndex(orderColName);
    int visualZoneColIdx = result.getColumnIndex(zoneColName);
    int metricZoneColIdx = result.getColumnIndex(metricZoneColName);
    int invMetricZoneColIdx = result.getColumnIndex(invMetricZoneColName);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes, linkMetricCostColName);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);
    auto [parents] = traverseFind(analysisData, graph, refs, m_pixelsFrom, m_pixelTo);

    int linePixelCounter = 0;
    auto pixelToParent = parents.find(m_pixelTo);
    if (pixelToParent != parents.end()) {

        for (auto &adt : analysisData) {
            adt.visitedFromBin = 0;
            result.setValue(adt.attributeDataRow, distCol, adt.dist);
            adt.dist = -1.0f;
        }

        int counter = 0;

        for (const PixelRef &pixelFrom : m_pixelsFrom) {
            auto adt = analysisData.at(getRefIdx(refs, pixelFrom));
            result.setValue(adt.attributeDataRow, distCol, 0);
        }

        auto *lad = &analysisData.at(getRefIdx(refs, m_pixelTo));
        result.setValue(lad->attributeDataRow, orderCol, counter);

        counter++;
        auto currParent = pixelToParent;
        counter++;
        while (currParent != parents.end()) {
            auto &ad = analysisData.at(getRefIdx(refs, currParent->second));
            auto &p = ad.point;
            result.setValue(ad.attributeDataRow, orderCol, counter);

            if (!p.getMergePixel().empty() && p.getMergePixel() == currParent->first) {
                result.setValue(ad.attributeDataRow, linkedCol, 1);
                result.setValue(lad->attributeDataRow, linkedCol, 1);
            } else {
                // apparently we can't just have 1 number in the whole column
                result.setValue(ad.attributeDataRow, linkedCol, 0);
                auto pixelated = m_map.quickPixelateLine(currParent->first, currParent->second);
                for (auto &linePixel : pixelated) {
                    auto *linePixelRow = attributes.getRowPtr(AttributeKey(linePixel));
                    if (linePixelRow != nullptr) {
                        auto &lpad = analysisData.at(getRefIdx(refs, linePixel));
                        result.setValue(lpad.attributeDataRow, pathCol, linePixelCounter++);
                        result.setValue(lpad.attributeDataRow, visualZoneColIdx, 0);
                        result.setValue(lpad.attributeDataRow, metricZoneColIdx, 0);
                        result.setValue(lpad.attributeDataRow, invMetricZoneColIdx, 1);

                        std::set<MetricSearchData> newPixels;
                        extractMetric(graph.at(lpad.attributeDataRow), newPixels, m_map,
                                      MetricSearchData(lpad, 0.0f, std::nullopt));
                        for (auto &zonePixel : newPixels) {
                            auto &zad = zonePixel.ad;
                            if (result.getValue(zad.attributeDataRow, visualZoneColIdx) == -1) {
                                result.setValue(zad.attributeDataRow, visualZoneColIdx,
                                                linePixelCounter);
                            }

                            double zoneLineDist = dist(linePixel, zad.ref) * m_map.getSpacing();
                            {
                                float currMetricZonePixelVal =
                                    result.getValue(zad.attributeDataRow, metricZoneColIdx);
                                if (currMetricZonePixelVal == -1 ||
                                    zoneLineDist < currMetricZonePixelVal) {
                                    result.setValue(zad.attributeDataRow, metricZoneColIdx,
                                                    zoneLineDist);
                                }
                            }
                            {
                                float currInvMetricZonePixelVal =
                                    result.getValue(zad.attributeDataRow, invMetricZoneColIdx);
                                if (currInvMetricZonePixelVal == -1 ||
                                    1.0f / (zoneLineDist + 1) > currInvMetricZonePixelVal) {
                                    result.setValue(zad.attributeDataRow, invMetricZoneColIdx,
                                                    1.0f / (zoneLineDist + 1));
                                }
                            }
                            zad.visitedFromBin = 0;
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
