// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisualshortestpath.h"

AnalysisResult VGAVisualShortestPath::run(Communicator *) {

    AnalysisResult result;

    auto &attributes = m_map.getAttributeTable();

    std::string path_col_name = Column::VISUAL_SHORTEST_PATH;
    attributes.insertOrResetColumn(path_col_name);
    result.addAttribute(path_col_name);
    std::string linked_col_name = Column::VISUAL_SHORTEST_PATH_LINKED;
    attributes.insertOrResetColumn(linked_col_name);
    result.addAttribute(linked_col_name);
    std::string order_col_name = Column::VISUAL_SHORTEST_PATH_ORDER;
    attributes.insertOrResetColumn(order_col_name);
    result.addAttribute(order_col_name);
    std::string zone_col_name = Column::VISUAL_SHORTEST_PATH_ZONE;
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
        p.m_extent = pix;
    }

    std::vector<PixelRefVector> search_tree;
    search_tree.push_back(PixelRefVector());

    search_tree.back().push_back(m_pixelFrom);

    size_t level = 0;
    std::map<PixelRef, PixelRef> parents;
    while (search_tree[level].size()) {
        search_tree.push_back(PixelRefVector());
        auto &currLevelPix = search_tree[level];
        auto &nextLevelPix = search_tree[level + 1];
        for (auto iter = currLevelPix.rbegin(); iter != currLevelPix.rend(); ++iter) {
            PixelRefVector newPixels;
            PixelRefVector mergePixels;
            Point &p = m_map.getPoint(*iter);
            if (p.filled() && p.m_misc != ~0) {
                if (!p.contextfilled() || iter->iseven() || level == 0) {
                    p.getNode().extractUnseen(newPixels, &m_map);
                    p.m_misc = ~0;
                    if (!p.getMergePixel().empty()) {
                        Point &p2 = m_map.getPoint(p.getMergePixel());
                        if (p2.m_misc != ~0) {
                            newPixels.push_back(p.getMergePixel());
                            p2.getNode().extractUnseen(mergePixels, &m_map);
                            for (auto &pixel : mergePixels) {
                                parents[pixel] = p.getMergePixel();
                            }
                            p2.m_misc = ~0;
                        }
                    }
                } else {
                    p.m_misc = ~0;
                }
            }

            for (auto &pixel : newPixels) {
                parents[pixel] = *iter;
            }
            nextLevelPix.insert(nextLevelPix.end(), newPixels.begin(), newPixels.end());
            nextLevelPix.insert(nextLevelPix.end(), mergePixels.begin(), mergePixels.end());
        }
        int linePixelCounter = 0;
        for (auto iter = nextLevelPix.rbegin(); iter != nextLevelPix.rend(); ++iter) {
            if (*iter == m_pixelTo) {

                for (auto &row : attributes) {
                    PixelRef pix = PixelRef(row.getKey().value);
                    Point &p = m_map.getPoint(pix);
                    p.m_misc = 0;
                    p.m_extent = pix;
                }

                int counter = 0;
                AttributeRow &lastPixelRow = attributes.getRow(AttributeKey(*iter));
                lastPixelRow.setValue(order_col, counter);
                lastPixelRow.setValue(linked_col, 0);
                counter++;
                auto currParent = parents.find(*iter);

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
                        auto pixelated =
                            m_map.quickPixelateLine(currParent->first, currParent->second);
                        for (auto &linePixel : pixelated) {
                            auto *linePixelRow = attributes.getRowPtr(AttributeKey(linePixel));
                            if (linePixelRow != 0) {
                                linePixelRow->setValue(path_col, linePixelCounter++);

                                PixelRefVector newPixels;
                                Point &p = m_map.getPoint(linePixel);
                                p.getNode().extractUnseen(newPixels, &m_map);
                                for (auto &zonePixel : newPixels) {
                                    auto *zonePixelRow =
                                        attributes.getRowPtr(AttributeKey(zonePixel));
                                    if (zonePixelRow != 0) {
                                        if (zonePixelRow->getValue(zone_col) == -1) {
                                            zonePixelRow->setValue(zone_col, linePixelCounter);
                                        }
                                        m_map.getPoint(zonePixel).m_misc = 0;
                                        m_map.getPoint(zonePixel).m_extent = zonePixel;
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
        }
        level++;
    }

    return result;
}

void VGAVisualShortestPath::extractUnseen(Node &node, PixelRefVector &pixels,
                                          depthmapX::RowMatrix<int> &miscs,
                                          depthmapX::RowMatrix<PixelRef> &extents) {
    for (int i = 0; i < 32; i++) {
        Bin &bin = node.bin(i);
        for (auto pixVec : bin.m_pixel_vecs) {
            for (PixelRef pix = pixVec.start();
                 pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                int &misc = miscs(pix.y, pix.x);
                PixelRef &extent = extents(pix.y, pix.x);
                if (misc == 0) {
                    pixels.push_back(pix);
                    misc |= (1 << i);
                }
                // 10.2.02 revised --- diagonal was breaking this as it was extent in diagonal or
                // horizontal
                if (!(bin.m_dir & PixelRef::DIAGONAL)) {
                    if (extent.col(bin.m_dir) >= pixVec.end().col(bin.m_dir))
                        break;
                    extent.col(bin.m_dir) = pixVec.end().col(bin.m_dir);
                }
                pix.move(bin.m_dir);
            }
        }
    }
}
