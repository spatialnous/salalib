// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgatraversing.h"

class IVGAVisual : public IVGATraversing {
  protected:
    IVGAVisual(const PointMap &map) : IVGATraversing(map) {}
    std::vector<AnalysisData> getAnalysisData(const AttributeTable &attributes) {
        std::vector<AnalysisData> analysisData;
        analysisData.reserve(attributes.getNumRows());

        size_t rowCounter = 0;
        for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
            PixelRef pix = iter->getKey().value;
            auto &point = m_map.getPoint(pix);
            analysisData.push_back(AnalysisData(point, pix, rowCounter, 0, pix, -1.0f, -1.0f));
            rowCounter++;
        }
        return analysisData;
    }

    void extractUnseen(const ADRefVector<AnalysisData> &conns,
                       ADRefVector<AnalysisData> &pixels) const {
        for (auto &conn : conns) {
            auto &ad = std::get<0>(conn).get();
            int binI = std::get<1>(conn);
            if (ad.m_visitedFromBin == 0) {
                pixels.push_back(conn);
                ad.m_visitedFromBin |= (1 << binI);
            }
        }
    }

    virtual std::vector<AnalysisColumn>
    traverse(std::vector<AnalysisData> &analysisData,
             const std::vector<ADRefVector<AnalysisData>> &graph, const std::vector<PixelRef> &refs,
             const double, const std::set<PixelRef> &originRefs,
             const bool keepStats = false) const override {

        AnalysisColumn sd(analysisData.size());

        std::vector<ADRefVector<AnalysisData>> search_tree;
        search_tree.push_back(ADRefVector<AnalysisData>());
        for (auto &sel : originRefs) {
            auto &ad = analysisData.at(getRefIdx(refs, sel));
            search_tree.back().push_back({ad, 0});
        }

        size_t level = 0;
        while (search_tree[level].size()) {
            search_tree.push_back(ADRefVector<AnalysisData>());
            const auto &searchTreeAtLevel = search_tree[level];
            for (auto currLvlIter = searchTreeAtLevel.rbegin();
                 currLvlIter != searchTreeAtLevel.rend(); currLvlIter++) {
                auto &ad = std::get<0>(*currLvlIter).get();
                auto &p = ad.m_point;
                if (p.filled() && ad.m_visitedFromBin != ~0) {
                    sd.setValue(ad.m_attributeDataRow, float(level), keepStats);
                    if (!p.contextfilled() || ad.m_ref.iseven() || level == 0) {
                        extractUnseen(graph.at(ad.m_attributeDataRow), search_tree[level + 1]);
                        ad.m_visitedFromBin = ~0;
                        if (!p.getMergePixel().empty()) {
                            auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                            int &p2misc = ad2.m_visitedFromBin;
                            if (p2misc != ~0) {
                                sd.setValue(ad2.m_attributeDataRow, float(level), keepStats);
                                extractUnseen(graph.at(ad2.m_attributeDataRow),
                                              search_tree[level + 1]);
                                p2misc = ~0;
                            }
                        }
                    } else {
                        ad.m_visitedFromBin = ~0;
                    }
                }
            }
            level++;
        }
        return {sd};
    }

    std::tuple<int, int, std::vector<int>>
    traverseSum(std::vector<AnalysisData> &analysisData,
                const std::vector<ADRefVector<AnalysisData>> &graph,
                const std::vector<PixelRef> &refs, const double radius, AnalysisData &ad0) {

        int totalDepth = 0;
        int totalNodes = 0;

        std::vector<ADRefVector<AnalysisData>> search_tree;
        search_tree.push_back(ADRefVector<AnalysisData>());
        search_tree.back().push_back({ad0, 0});

        std::vector<int> distribution;
        size_t level = 0;
        while (search_tree[level].size()) {
            search_tree.push_back(ADRefVector<AnalysisData>());
            const auto &searchTreeAtLevel = search_tree[level];
            distribution.push_back(0);
            for (auto currLvlIter = searchTreeAtLevel.rbegin();
                 currLvlIter != searchTreeAtLevel.rend(); currLvlIter++) {
                auto &ad3 = std::get<0>(*currLvlIter).get();
                auto &p = ad3.m_point;
                if (p.filled() && ad3.m_visitedFromBin != ~0) {

                    totalDepth += static_cast<int>(level);
                    totalNodes += 1;
                    distribution.back() += 1;
                    if ((int)radius == -1 || (level < static_cast<size_t>(radius) &&
                                              (!p.contextfilled() || ad3.m_ref.iseven()))) {
                        extractUnseen(graph.at(ad3.m_attributeDataRow), search_tree[level + 1]);
                        ad3.m_visitedFromBin = ~0;
                        if (!p.getMergePixel().empty()) {
                            auto &ad4 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                            if (ad4.m_visitedFromBin != ~0) {
                                extractUnseen(graph.at(ad4.m_attributeDataRow),
                                              search_tree[level + 1]);
                                ad4.m_visitedFromBin = ~0;
                            }
                        }
                    } else {
                        ad3.m_visitedFromBin = ~0;
                    }
                }
                search_tree[level].pop_back();
            }
            level++;
        }
        return std::make_tuple(totalDepth, totalNodes, distribution);
    }

    std::tuple<std::map<PixelRef, PixelRef>>
    traverseFind(std::vector<AnalysisData> &analysisData,
                 const std::vector<ADRefVector<AnalysisData>> &graph,
                 const std::vector<PixelRef> &refs, PixelRef sourceRef, PixelRef targetRef) {

        std::vector<ADRefVector<AnalysisData>> search_tree;
        search_tree.push_back(ADRefVector<AnalysisData>());

        search_tree.back().push_back({analysisData.at(getRefIdx(refs, sourceRef)), 0});

        size_t level = 0;
        std::map<PixelRef, PixelRef> parents;
        bool pixelFound = false;
        while (search_tree[level].size()) {
            search_tree.push_back(ADRefVector<AnalysisData>());
            auto &currLevelPix = search_tree[level];
            auto &nextLevelPix = search_tree[level + 1];
            for (auto iter = currLevelPix.rbegin(); iter != currLevelPix.rend(); ++iter) {
                auto &ad = std::get<0>(*iter).get();
                ADRefVector<AnalysisData> newPixels;
                ADRefVector<AnalysisData> mergePixels;
                auto &p = ad.m_point;
                if (p.filled() && ad.m_visitedFromBin != ~0) {
                    if (!p.contextfilled() || ad.m_ref.iseven() || level == 0) {
                        extractUnseen(graph.at(ad.m_attributeDataRow), newPixels);
                        ad.m_visitedFromBin = ~0;
                        if (!p.getMergePixel().empty()) {
                            auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                            if (ad2.m_visitedFromBin != ~0) {
                                newPixels.push_back({ad2, 0});
                                extractUnseen(graph.at(ad2.m_attributeDataRow), mergePixels);
                                for (auto &pixel : mergePixels) {
                                    parents[std::get<0>(pixel).get().m_ref] = p.getMergePixel();
                                }
                                ad2.m_visitedFromBin = ~0;
                            }
                        }
                    } else {
                        ad.m_visitedFromBin = ~0;
                    }
                }

                for (auto &pixel : newPixels) {
                    parents[std::get<0>(pixel).get().m_ref] = ad.m_ref;
                }
                nextLevelPix.insert(nextLevelPix.end(), newPixels.begin(), newPixels.end());
                nextLevelPix.insert(nextLevelPix.end(), mergePixels.begin(), mergePixels.end());
            }
            for (auto iter = nextLevelPix.rbegin(); iter != nextLevelPix.rend(); ++iter) {
                if (std::get<0>(*iter).get().m_ref == targetRef) {
                    pixelFound = true;
                }
            }
            if (pixelFound)
                break;
            level++;
        }
        return std::make_tuple(parents);
    }
};
