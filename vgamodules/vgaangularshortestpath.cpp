// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaangularshortestpath.h"

AnalysisResult VGAAngularShortestPath::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    AnalysisResult result({Column::ANGULAR_SHORTEST_PATH, Column::ANGULAR_SHORTEST_PATH_LINKED,
                           Column::ANGULAR_SHORTEST_PATH_ORDER,
                           Column::ANGULAR_SHORTEST_PATH_ORDER},
                          attributes.getNumRows());

    int path_col = attributes.getColumnIndex(Column::ANGULAR_SHORTEST_PATH);
    int linked_col = attributes.getColumnIndex(Column::ANGULAR_SHORTEST_PATH_LINKED);
    int order_col = attributes.getColumnIndex(Column::ANGULAR_SHORTEST_PATH_ORDER);
    int zone_col = attributes.getColumnIndex(Column::ANGULAR_SHORTEST_PATH_ORDER);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, false);

    auto [parents] = traverseFind(analysisData, graph, refs, m_pixelFrom, m_pixelTo);

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
                        result.setValue(lpad.m_attributeDataRow, zone_col, 1);

                        std::set<AngularSearchData> newPixels;
                        extractAngular(graph.at(lpad.m_attributeDataRow), newPixels, m_map,
                                       AngularSearchData(lpad, 0.0f, std::nullopt));
                        for (auto &zonePixel : newPixels) {
                            auto *zonePixelRow =
                                attributes.getRowPtr(AttributeKey(zonePixel.pixel.m_ref));
                            if (zonePixelRow != 0) {
                                double zoneLineDist = dist(linePixel, zonePixel.pixel.m_ref);
                                float currZonePixelVal = zonePixelRow->getValue(zone_col);
                                if (currZonePixelVal == -1 ||
                                    1.0f / (zoneLineDist + 1) > currZonePixelVal) {
                                    result.setValue(zonePixel.pixel.m_attributeDataRow, zone_col,
                                                    1.0f / (zoneLineDist + 1));
                                }
                                zonePixel.pixel.m_visitedFromBin = 0;
                                // zonePixel.pixel.m_extent = zonePixel.pixel;
                            }
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
