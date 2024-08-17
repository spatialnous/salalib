// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaangularshortestpath.h"

AnalysisResult VGAAngularShortestPath::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    AnalysisResult result({Column::ANGULAR_SHORTEST_PATH,             //
                           Column::ANGULAR_SHORTEST_PATH_LINKED,      //
                           Column::ANGULAR_SHORTEST_PATH_ORDER,       //
                           Column::ANGULAR_SHORTEST_PATH_VISUAL_ZONE, //
                           Column::ANGULAR_SHORTEST_PATH_METRIC_ZONE, //
                           Column::ANGULAR_SHORTEST_PATH_INV_METRIC_ZONE},
                          attributes.getNumRows());

    int pathColIdx = result.getColumnIndex(Column::ANGULAR_SHORTEST_PATH);
    int linkedColIdx = result.getColumnIndex(Column::ANGULAR_SHORTEST_PATH_LINKED);
    int orderColIdx = result.getColumnIndex(Column::ANGULAR_SHORTEST_PATH_ORDER);
    int visualZoneColIdx = result.getColumnIndex(Column::ANGULAR_SHORTEST_PATH_VISUAL_ZONE);
    int metricZoneColIdx = result.getColumnIndex(Column::ANGULAR_SHORTEST_PATH_METRIC_ZONE);
    int invMetricZoneColIdx = result.getColumnIndex(Column::ANGULAR_SHORTEST_PATH_INV_METRIC_ZONE);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, false);

    auto [parents] = traverseFind(analysisData, graph, refs, {m_pixelFrom}, m_pixelTo);

    int linePixelCounter = 0;
    auto pixelToParent = parents.find(m_pixelTo);
    if (pixelToParent != parents.end()) {

        for (auto &ad : analysisData) {
            ad.m_visitedFromBin = 0;
            ad.m_dist = 0.0f;
            ad.m_cumAngle = -1.0f;
        }

        int counter = 0;

        auto *lad = &analysisData.at(getRefIdx(refs, m_pixelTo));
        result.setValue(lad->m_attributeDataRow, orderColIdx, counter);

        counter++;
        auto currParent = pixelToParent;
        counter++;

        while (currParent != parents.end()) {
            auto &ad = analysisData.at(getRefIdx(refs, currParent->second));
            auto &p = ad.m_point;
            result.setValue(ad.m_attributeDataRow, orderColIdx, counter);

            if (!p.getMergePixel().empty() && p.getMergePixel() == currParent->first) {
                result.setValue(ad.m_attributeDataRow, linkedColIdx, 1);
                result.setValue(lad->m_attributeDataRow, linkedColIdx, 1);
            } else {
                // apparently we can't just have 1 number in the whole column
                result.setValue(ad.m_attributeDataRow, linkedColIdx, 0);
                auto pixelated = m_map.quickPixelateLine(currParent->first, currParent->second);
                for (auto &linePixel : pixelated) {
                    auto linePixelRow = getRefIdxOptional(refs, linePixel);
                    if (linePixelRow.has_value()) {
                        auto &lpad = analysisData.at(getRefIdx(refs, linePixel));
                        result.setValue(lpad.m_attributeDataRow, pathColIdx, linePixelCounter++);
                        result.setValue(lpad.m_attributeDataRow, visualZoneColIdx, 0);
                        result.setValue(lpad.m_attributeDataRow, metricZoneColIdx, 0);
                        result.setValue(lpad.m_attributeDataRow, invMetricZoneColIdx, 1);

                        std::set<AngularSearchData> newPixels;
                        extractAngular(graph.at(lpad.m_attributeDataRow), newPixels, m_map,
                                       AngularSearchData(lpad, 0.0f, std::nullopt));
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
    }

    return result;
}
