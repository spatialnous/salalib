// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgatraversing.hpp"

class IVGAAngular : public IVGATraversing {
  protected:
    IVGAAngular(const PointMap &map) : IVGATraversing(map) {}

    std::vector<AnalysisData> getAnalysisData(const AttributeTable &attributes) {
        std::vector<AnalysisData> analysisData;
        analysisData.reserve(m_map.getAttributeTable().getNumRows());

        size_t rowCounter = 0;
        for (auto &attRow : attributes) {
            auto &point = m_map.getPoint(attRow.getKey().value);
            analysisData.push_back(AnalysisData(point, attRow.getKey().value, rowCounter, 0,
                                                attRow.getKey().value, 0.0f, -1.0f));
            rowCounter++;
        }
        return analysisData;
    }

    struct AngularSearchData {
        AnalysisData &ad;
        float angle;
        std::optional<PixelRef> lastPixel;

      private:
        [[maybe_unused]] unsigned _padding0 : 2 * 8;
        [[maybe_unused]] unsigned _padding1 : 4 * 8;

      public:
        AngularSearchData(AnalysisData &p, float a = 0.0f,
                          std::optional<PixelRef> lp = std::nullopt)
            : ad(p), angle(a), lastPixel(lp), _padding0(0), _padding1(0) {}
        bool operator==(const AngularSearchData &mp2) const {
            return (angle == mp2.angle && ad.ref == mp2.ad.ref);
        }
        bool operator<(const AngularSearchData &mp2) const {
            return (angle < mp2.angle) || (angle == mp2.angle && ad.ref < mp2.ad.ref);
        }
        bool operator>(const AngularSearchData &mp2) const {
            return (angle > mp2.angle) || (angle == mp2.angle && ad.ref > mp2.ad.ref);
        }
        bool operator!=(const AngularSearchData &mp2) const {
            return (angle != mp2.angle) || (ad.ref != mp2.ad.ref);
        }
    };

    void extractAngular(const ADRefVector<AnalysisData> &conns, std::set<AngularSearchData> &pixels,
                        const PointMap &map, const AngularSearchData &curs) const {
        if (curs.angle == 0.0f || curs.ad.point.blocked() || map.blockedAdjacent(curs.ad.ref)) {
            for (auto &conn : conns) {
                auto &ad = std::get<0>(conn).get();
                if (ad.visitedFromBin == 0) {
                    // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                    float ang =
                        (!curs.lastPixel.has_value())
                            ? 0.0f
                            : static_cast<float>(angle(ad.ref, curs.ad.ref, *curs.lastPixel) /
                                                 (M_PI * 0.5));
                    if (ad.cumAngle == -1.0 || curs.angle + ang < ad.cumAngle) {
                        ad.cumAngle = curs.ad.cumAngle + ang;
                        pixels.insert(AngularSearchData(ad, ad.cumAngle, curs.ad.ref));
                    }
                }
            }
        }
    }

    std::vector<AnalysisColumn> traverse(std::vector<AnalysisData> &analysisData,
                                         const std::vector<ADRefVector<AnalysisData>> &graph,
                                         const std::vector<PixelRef> &refs, const double radius,
                                         const std::set<PixelRef> &originRefs,
                                         const bool keepStats = false) const override {

        AnalysisColumn angularDepthCol(analysisData.size());

        std::set<AngularSearchData> searchList; // contains root point

        for (auto &sel : originRefs) {
            auto &ad = analysisData.at(getRefIdx(refs, sel));
            searchList.insert(AngularSearchData(ad, 0.0f, std::nullopt));
            ad.cumAngle = 0.0f;
        }

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only
        while (searchList.size()) {
            auto internalNode = searchList.extract(searchList.begin());
            AngularSearchData here = std::move(internalNode.value());
            if (radius != -1.0 && here.angle > radius) {
                break;
            }

            auto &ad = here.ad;
            auto &p = ad.point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad.visitedFromBin != ~0) {
                extractAngular(graph.at(ad.attributeDataRow), searchList, m_map, here);
                ad.visitedFromBin = ~0;
                angularDepthCol.setValue(ad.attributeDataRow, static_cast<float>(ad.cumAngle),
                                         keepStats);
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.visitedFromBin != ~0) {
                        ad2.cumAngle = ad.cumAngle;
                        angularDepthCol.setValue(ad2.attributeDataRow,
                                                 static_cast<float>(ad2.cumAngle), keepStats);
                        extractAngular(graph.at(ad2.attributeDataRow), searchList, m_map,
                                       AngularSearchData(ad2, here.angle, std::nullopt));
                        ad2.visitedFromBin = ~0;
                    }
                }
            }
        }
        return {std::move(angularDepthCol)};
    }

    std::tuple<float, int> traverseSum(std::vector<AnalysisData> &analysisData,
                                       const std::vector<ADRefVector<AnalysisData>> &graph,
                                       const std::vector<PixelRef> &refs, const double radius,
                                       AnalysisData &ad0) {

        float totalAngle = 0.0f;
        int totalNodes = 0;

        std::set<AngularSearchData> searchList;
        searchList.insert(AngularSearchData(ad0, 0.0f, std::nullopt));
        ad0.cumAngle = 0.0f;
        while (searchList.size()) {
            auto internalNode = searchList.extract(searchList.begin());
            AngularSearchData here = std::move(internalNode.value());

            if (radius != -1.0 && here.angle > radius) {
                break;
            }
            auto &ad1 = here.ad;
            auto &p = ad1.point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps'
            // left in
            if (p.filled() && ad1.visitedFromBin != ~0) {
                extractAngular(graph.at(ad1.attributeDataRow), searchList, m_map, here);
                ad1.visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.visitedFromBin != ~0) {
                        ad2.cumAngle = ad1.cumAngle;
                        extractAngular(graph.at(ad2.attributeDataRow), searchList, m_map,
                                       AngularSearchData(ad2, here.angle, std::nullopt));
                        ad2.visitedFromBin = ~0;
                    }
                }
                totalAngle += ad1.cumAngle;
                totalNodes += 1;
            }
        }

        return std::make_tuple(totalAngle, totalNodes);
    }

    std::tuple<std::map<PixelRef, PixelRef>>
    traverseFind(std::vector<AnalysisData> &analysisData,
                 const std::vector<ADRefVector<AnalysisData>> &graph,
                 const std::vector<PixelRef> &refs, const std::set<PixelRef> sourceRefs,
                 const PixelRef targetRef) {

        // in order to calculate Penn angle, the MetricPair becomes a metric triple...
        std::set<AngularSearchData> searchList; // contains root point
        for (const auto &sourceRef : sourceRefs) {
            auto &ad = analysisData.at(getRefIdx(refs, sourceRef));
            searchList.insert(AngularSearchData(ad, 0.0f, std::nullopt));
            ad.cumAngle = 0.0f;
        }
        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only
        std::map<PixelRef, PixelRef> parents;
        bool pixelFound = false;
        while (searchList.size()) {
            auto internalNode = searchList.extract(searchList.begin());
            auto here = std::move(internalNode.value());

            auto &ad = here.ad;
            auto &p = ad.point;
            std::set<AngularSearchData> newPixels;
            std::set<AngularSearchData> mergePixels;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad.visitedFromBin != ~0) {
                extractAngular(graph.at(ad.attributeDataRow), newPixels, m_map, here);
                ad.visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.visitedFromBin != ~0) {
                        auto newTripleIter =
                            newPixels.insert(AngularSearchData(ad2, here.angle, std::nullopt));
                        ad2.cumAngle = ad.cumAngle;
                        extractAngular(graph.at(ad2.attributeDataRow), mergePixels, m_map,
                                       *newTripleIter.first);
                        for (auto &pixel : mergePixels) {
                            parents[pixel.ad.ref] = p.getMergePixel();
                        }
                        ad2.visitedFromBin = ~0;
                    }
                }
            }
            for (auto &pixel : newPixels) {
                parents[pixel.ad.ref] = here.ad.ref;
            }
            newPixels.insert(mergePixels.begin(), mergePixels.end());
            for (auto &pixel : newPixels) {
                if (pixel.ad.ref == targetRef) {
                    pixelFound = true;
                }
            }
            if (!pixelFound)
                searchList.insert(newPixels.begin(), newPixels.end());
        }
        return std::make_tuple(parents);
    }
};
