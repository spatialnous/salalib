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
            if (ad.visitedFromBin == 0) {
                pixels.push_back(conn);
                ad.visitedFromBin |= (1 << binI);
            }
        }
    }

    std::vector<AnalysisColumn> traverse(std::vector<AnalysisData> &analysisData,
                                         const std::vector<ADRefVector<AnalysisData>> &graph,
                                         const std::vector<PixelRef> &refs, const double,
                                         const std::set<PixelRef> &originRefs,
                                         const bool keepStats = false) const override {

        AnalysisColumn sd(analysisData.size());

        std::vector<ADRefVector<AnalysisData>> searchTree;
        searchTree.push_back(ADRefVector<AnalysisData>());
        for (auto &sel : originRefs) {
            auto &ad = analysisData.at(getRefIdx(refs, sel));
            searchTree.back().push_back({ad, 0});
        }

        size_t level = 0;
        while (searchTree[level].size()) {
            searchTree.push_back(ADRefVector<AnalysisData>());
            const auto &searchTreeAtLevel = searchTree[level];
            for (auto currLvlIter = searchTreeAtLevel.rbegin();
                 currLvlIter != searchTreeAtLevel.rend(); currLvlIter++) {
                auto &ad = std::get<0>(*currLvlIter).get();
                auto &p = ad.point;
                if (p.filled() && ad.visitedFromBin != ~0) {
                    sd.setValue(ad.attributeDataRow, float(level), keepStats);
                    if (!p.contextfilled() || ad.ref.iseven() || level == 0) {
                        extractUnseen(graph.at(ad.attributeDataRow), searchTree[level + 1]);
                        ad.visitedFromBin = ~0;
                        if (!p.getMergePixel().empty()) {
                            auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                            int &p2misc = ad2.visitedFromBin;
                            if (p2misc != ~0) {
                                sd.setValue(ad2.attributeDataRow, float(level), keepStats);
                                extractUnseen(graph.at(ad2.attributeDataRow),
                                              searchTree[level + 1]);
                                p2misc = ~0;
                            }
                        }
                    } else {
                        ad.visitedFromBin = ~0;
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

        std::vector<ADRefVector<AnalysisData>> searchTree;
        searchTree.push_back(ADRefVector<AnalysisData>());
        searchTree.back().push_back({ad0, 0});

        std::vector<int> distribution;
        size_t level = 0;
        while (searchTree[level].size()) {
            searchTree.push_back(ADRefVector<AnalysisData>());
            const auto &searchTreeAtLevel = searchTree[level];
            distribution.push_back(0);
            for (auto currLvlIter = searchTreeAtLevel.rbegin();
                 currLvlIter != searchTreeAtLevel.rend(); currLvlIter++) {
                auto &ad3 = std::get<0>(*currLvlIter).get();
                auto &p = ad3.point;
                if (p.filled() && ad3.visitedFromBin != ~0) {

                    totalDepth += static_cast<int>(level);
                    totalNodes += 1;
                    distribution.back() += 1;
                    if ((int)radius == -1 || (level < static_cast<size_t>(radius) &&
                                              (!p.contextfilled() || ad3.ref.iseven()))) {
                        extractUnseen(graph.at(ad3.attributeDataRow), searchTree[level + 1]);
                        ad3.visitedFromBin = ~0;
                        if (!p.getMergePixel().empty()) {
                            auto &ad4 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                            if (ad4.visitedFromBin != ~0) {
                                extractUnseen(graph.at(ad4.attributeDataRow),
                                              searchTree[level + 1]);
                                ad4.visitedFromBin = ~0;
                            }
                        }
                    } else {
                        ad3.visitedFromBin = ~0;
                    }
                }
                searchTree[level].pop_back();
            }
            level++;
        }
        return std::make_tuple(totalDepth, totalNodes, distribution);
    }

    std::tuple<std::map<PixelRef, PixelRef>>
    traverseFind(std::vector<AnalysisData> &analysisData,
                 const std::vector<ADRefVector<AnalysisData>> &graph,
                 const std::vector<PixelRef> &refs, PixelRef sourceRef, PixelRef targetRef) {

        std::vector<ADRefVector<AnalysisData>> searchTree;
        searchTree.push_back(ADRefVector<AnalysisData>());

        searchTree.back().push_back({analysisData.at(getRefIdx(refs, sourceRef)), 0});

        size_t level = 0;
        std::map<PixelRef, PixelRef> parents;
        bool pixelFound = false;
        while (searchTree[level].size()) {
            searchTree.push_back(ADRefVector<AnalysisData>());
            auto &currLevelPix = searchTree[level];
            auto &nextLevelPix = searchTree[level + 1];
            for (auto iter = currLevelPix.rbegin(); iter != currLevelPix.rend(); ++iter) {
                auto &ad = std::get<0>(*iter).get();
                ADRefVector<AnalysisData> newPixels;
                ADRefVector<AnalysisData> mergePixels;
                auto &p = ad.point;
                if (p.filled() && ad.visitedFromBin != ~0) {
                    if (!p.contextfilled() || ad.ref.iseven() || level == 0) {
                        extractUnseen(graph.at(ad.attributeDataRow), newPixels);
                        ad.visitedFromBin = ~0;
                        if (!p.getMergePixel().empty()) {
                            auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                            if (ad2.visitedFromBin != ~0) {
                                newPixels.push_back({ad2, 0});
                                extractUnseen(graph.at(ad2.attributeDataRow), mergePixels);
                                for (auto &pixel : mergePixels) {
                                    parents[std::get<0>(pixel).get().ref] = p.getMergePixel();
                                }
                                ad2.visitedFromBin = ~0;
                            }
                        }
                    } else {
                        ad.visitedFromBin = ~0;
                    }
                }

                for (auto &pixel : newPixels) {
                    parents[std::get<0>(pixel).get().ref] = ad.ref;
                }
                nextLevelPix.insert(nextLevelPix.end(), newPixels.begin(), newPixels.end());
                nextLevelPix.insert(nextLevelPix.end(), mergePixels.begin(), mergePixels.end());
            }
            for (auto iter = nextLevelPix.rbegin(); iter != nextLevelPix.rend(); ++iter) {
                if (std::get<0>(*iter).get().ref == targetRef) {
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
