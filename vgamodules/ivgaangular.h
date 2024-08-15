// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgatraversing.h"

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
        AnalysisData &pixel;
        float angle;
        std::optional<const AnalysisData> lastpixel = std::nullopt;
        AngularSearchData(AnalysisData &p, float a = 0.0f,
                          std::optional<const AnalysisData> lp = std::nullopt)
            : pixel(p), angle(a), lastpixel(lp) {}
        bool operator==(const AngularSearchData &mp2) const {
            return (angle == mp2.angle && pixel.m_ref == mp2.pixel.m_ref);
        }
        bool operator<(const AngularSearchData &mp2) const {
            return (angle < mp2.angle) || (angle == mp2.angle && pixel.m_ref < mp2.pixel.m_ref);
        }
        bool operator>(const AngularSearchData &mp2) const {
            return (angle > mp2.angle) || (angle == mp2.angle && pixel.m_ref > mp2.pixel.m_ref);
        }
        bool operator!=(const AngularSearchData &mp2) const {
            return (angle != mp2.angle) || (pixel.m_ref != mp2.pixel.m_ref);
        }
    };

    void extractAngular(const ADRefVector<AnalysisData> &conns, std::set<AngularSearchData> &pixels,
                        const PointMap &map, const AngularSearchData &curs) const {
        if (curs.angle == 0.0f || curs.pixel.m_point.blocked() ||
            map.blockedAdjacent(curs.pixel.m_ref)) {
            for (auto &conn : conns) {
                auto &ad = std::get<0>(conn).get();
                if (ad.m_visitedFromBin == 0) {
                    // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                    float ang =
                        (!curs.lastpixel.has_value())
                            ? 0.0f
                            : (float)(angle(ad.m_ref, curs.pixel.m_ref, curs.lastpixel->m_ref) /
                                      (M_PI * 0.5));
                    if (ad.m_cumAngle == -1.0 || curs.angle + ang < ad.m_cumAngle) {
                        ad.m_cumAngle = curs.pixel.m_cumAngle + ang;
                        pixels.insert(AngularSearchData(ad, ad.m_cumAngle, curs.pixel));
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

        std::set<AngularSearchData> search_list; // contains root point

        for (auto &sel : originRefs) {
            auto &ad = analysisData.at(getRefIdx(refs, sel));
            search_list.insert(AngularSearchData(ad, 0.0f, std::nullopt));
            ad.m_cumAngle = 0.0f;
        }

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only
        while (search_list.size()) {
            auto internalNode = search_list.extract(search_list.begin());
            AngularSearchData here = std::move(internalNode.value());
            if (radius != -1.0 && here.angle > radius) {
                break;
            }

            auto &ad = here.pixel;
            auto &p = ad.m_point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad.m_visitedFromBin != ~0) {
                extractAngular(graph.at(ad.m_attributeDataRow), search_list, m_map, here);
                ad.m_visitedFromBin = ~0;
                angularDepthCol.setValue(ad.m_attributeDataRow, float(ad.m_cumAngle), keepStats);
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.m_visitedFromBin != ~0) {
                        ad2.m_cumAngle = ad.m_cumAngle;
                        angularDepthCol.setValue(ad2.m_attributeDataRow, float(ad2.m_cumAngle),
                                                 keepStats);
                        extractAngular(graph.at(ad2.m_attributeDataRow), search_list, m_map,
                                       AngularSearchData(ad2, here.angle, std::nullopt));
                        ad2.m_visitedFromBin = ~0;
                    }
                }
            }
        }
        return {angularDepthCol};
    }

    std::tuple<float, int> traverseSum(std::vector<AnalysisData> &analysisData,
                                       const std::vector<ADRefVector<AnalysisData>> &graph,
                                       const std::vector<PixelRef> &refs, const double radius,
                                       AnalysisData &ad0) {

        float totalAngle = 0.0f;
        int totalNodes = 0;

        std::set<AngularSearchData> search_list;
        search_list.insert(AngularSearchData(ad0, 0.0f, std::nullopt));
        ad0.m_cumAngle = 0.0f;
        while (search_list.size()) {
            auto internalNode = search_list.extract(search_list.begin());
            AngularSearchData here = std::move(internalNode.value());

            if (radius != -1.0 && here.angle > radius) {
                break;
            }
            auto &ad1 = here.pixel;
            auto &p = ad1.m_point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps'
            // left in
            if (p.filled() && ad1.m_visitedFromBin != ~0) {
                extractAngular(graph.at(ad1.m_attributeDataRow), search_list, m_map, here);
                ad1.m_visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.m_visitedFromBin != ~0) {
                        ad2.m_cumAngle = ad1.m_cumAngle;
                        extractAngular(graph.at(ad2.m_attributeDataRow), search_list, m_map,
                                       AngularSearchData(ad2, here.angle, std::nullopt));
                        ad2.m_visitedFromBin = ~0;
                    }
                }
                totalAngle += ad1.m_cumAngle;
                totalNodes += 1;
            }
        }

        return std::make_tuple(totalAngle, totalNodes);
    }

    std::tuple<std::map<PixelRef, PixelRef>>
    traverseFind(std::vector<AnalysisData> &analysisData,
                 const std::vector<ADRefVector<AnalysisData>> &graph,
                 const std::vector<PixelRef> &refs, const PixelRef sourceRef,
                 const PixelRef targetRef) {

        // in order to calculate Penn angle, the MetricPair becomes a metric triple...
        std::set<AngularSearchData> search_list; // contains root point
        {
            auto &ad = analysisData.at(getRefIdx(refs, sourceRef));
            search_list.insert(AngularSearchData(ad, 0.0f, std::nullopt));
            ad.m_cumAngle = 0.0f;
        }
        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only
        std::map<PixelRef, PixelRef> parents;
        bool pixelFound = false;
        while (search_list.size()) {
            auto internalNode = search_list.extract(search_list.begin());
            AngularSearchData here = std::move(internalNode.value());

            auto &ad = here.pixel;
            auto &p = ad.m_point;
            std::set<AngularSearchData> newPixels;
            std::set<AngularSearchData> mergePixels;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad.m_visitedFromBin != ~0) {
                extractAngular(graph.at(ad.m_attributeDataRow), newPixels, m_map, here);
                ad.m_visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.m_visitedFromBin != ~0) {
                        auto newTripleIter =
                            newPixels.insert(AngularSearchData(ad2, here.angle, std::nullopt));
                        ad2.m_cumAngle = ad.m_cumAngle;
                        extractAngular(graph.at(ad2.m_attributeDataRow), mergePixels, m_map,
                                       *newTripleIter.first);
                        for (auto &pixel : mergePixels) {
                            parents[pixel.pixel.m_ref] = p.getMergePixel();
                        }
                        ad2.m_visitedFromBin = ~0;
                    }
                }
            }
            for (auto &pixel : newPixels) {
                parents[pixel.pixel.m_ref] = here.pixel.m_ref;
            }
            newPixels.insert(mergePixels.begin(), mergePixels.end());
            for (auto &pixel : newPixels) {
                if (pixel.pixel.m_ref == targetRef) {
                    pixelFound = true;
                }
            }
            if (!pixelFound)
                search_list.insert(newPixels.begin(), newPixels.end());
        }
        return std::make_tuple(parents);
    }
};
