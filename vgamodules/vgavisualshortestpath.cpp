// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisualshortestpath.h"

AnalysisResult VGAVisualShortestPath::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    AnalysisResult result({Column::VISUAL_SHORTEST_PATH, Column::VISUAL_SHORTEST_PATH_LINKED,
                           Column::VISUAL_SHORTEST_PATH_ORDER, Column::VISUAL_SHORTEST_PATH_ZONE},
                          attributes.getNumRows());

    int path_col = result.getColumnIndex(Column::VISUAL_SHORTEST_PATH);
    int linked_col = result.getColumnIndex(Column::VISUAL_SHORTEST_PATH_LINKED);
    int order_col = result.getColumnIndex(Column::VISUAL_SHORTEST_PATH_ORDER);
    int zone_col = result.getColumnIndex(Column::VISUAL_SHORTEST_PATH_ZONE);

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);

    auto [parents] = traverseFind(analysisData, graph, refs, m_pixelFrom, m_pixelTo);

    int linePixelCounter = 0;
    auto pixelToParent = parents.find(m_pixelTo);
    if (pixelToParent != parents.end()) {
        for (auto &ad : analysisData) {
            ad.m_visitedFromBin = 0;
            ad.m_diagonalExtent = ad.m_ref;
        }

        int counter = 0;
        auto *lad = &analysisData.at(getRefIdx(refs, m_pixelTo));

        result.setValue(lad->m_attributeDataRow, order_col, counter);
        result.setValue(lad->m_attributeDataRow, linked_col, 0);
        counter++;
        auto currParent = pixelToParent;

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
                        result.setValue(lad->m_attributeDataRow, path_col, linePixelCounter++);

                        ADRefVector<AnalysisData> newPixels;
                        extractUnseen(graph.at(lpad.m_attributeDataRow), newPixels);
                        for (auto &zonePixel : newPixels) {
                            auto &zad = std::get<0>(zonePixel).get();
                            auto *zonePixelRow = attributes.getRowPtr(AttributeKey(zad.m_ref));
                            if (zonePixelRow != 0) {
                                if (zonePixelRow->getValue(zone_col) == -1) {
                                    result.setValue(zad.m_attributeDataRow, zone_col,
                                                    linePixelCounter);
                                }
                                zad.m_visitedFromBin = 0;
                                //                                zad.m_extent = zonePixel;
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
    }

    return result;
}

// void VGAVisualShortestPath::extractUnseen(Node &node, PixelRefVector &pixels, const PointMap
// &map,
//                                           std::vector<AnalysisData> &analysisData) {
//     for (int i = 0; i < 32; i++) {
//         Bin &bin = node.bin(i);
//         for (auto pixVec : bin.m_pixel_vecs) {
//             for (PixelRef pix = pixVec.start();
//                  pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
//                 auto &ad = analysisData.at(map.getPixelRefIdx(pix));
//                 int &misc = ad.m_misc;
//                 PixelRef &extent = ad.m_extent;
//                 if (misc == 0) {
//                     pixels.push_back(pix);
//                     misc |= (1 << i);
//                 }
//                 // 10.2.02 revised --- diagonal was breaking this as it was extent in diagonal or
//                 // horizontal
//                 if (!(bin.m_dir & PixelRef::DIAGONAL)) {
//                     if (extent.col(bin.m_dir) >= pixVec.end().col(bin.m_dir))
//                         break;
//                     extent.col(bin.m_dir) = pixVec.end().col(bin.m_dir);
//                 }
//                 pix.move(bin.m_dir);
//             }
//         }
//     }
// }
