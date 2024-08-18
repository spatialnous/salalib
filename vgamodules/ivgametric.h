// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgatraversing.h"

class IVGAMetric : public IVGATraversing {
  protected:
    IVGAMetric(const PointMap &map) : IVGATraversing(map) {}

    std::vector<AnalysisData>
    getAnalysisData(const AttributeTable &attributes,
                    std::optional<std::string> linkCostColumn = std::nullopt) {
        std::vector<AnalysisData> analysisData;
        analysisData.reserve(attributes.getNumRows());

        std::optional<size_t> linkCostIdx = std::nullopt;
        if (linkCostColumn.has_value() && attributes.hasColumn(*linkCostColumn)) {
            linkCostIdx = attributes.getColumnIndex(*linkCostColumn);
        }
        size_t rowCounter = 0;
        for (auto &attRow : attributes) {
            auto &point = m_map.getPoint(attRow.getKey().value);
            analysisData.push_back(AnalysisData(point, attRow.getKey().value, rowCounter, 0,
                                                attRow.getKey().value, -1.0f, 0.0f));
            if (linkCostIdx.has_value()) {
                analysisData.back().m_linkCost = attRow.getRow().getValue(*linkCostIdx);
            }
            rowCounter++;
        }
        return analysisData;
    }

    // to allow a dist / PixelRef pair for easy sorting
    // (have to do comparison operation on both dist and PixelRef as
    // otherwise would have a duplicate key for pqmap / pqvector)

    struct MetricSearchData {
        AnalysisData &m_pixel;
        float m_dist;
        std::optional<PixelRef> m_lastpixel;
        MetricSearchData(AnalysisData &p, float d = 0.0f, std::optional<PixelRef> lp = std::nullopt)
            : m_pixel(p), m_dist(d), m_lastpixel(lp) {}
        bool operator==(const MetricSearchData &mp2) const {
            return (m_dist == mp2.m_dist && m_pixel.m_ref == mp2.m_pixel.m_ref);
        }
        bool operator<(const MetricSearchData &mp2) const {
            return (m_dist < mp2.m_dist) ||
                   (m_dist == mp2.m_dist && m_pixel.m_ref < mp2.m_pixel.m_ref);
        }
        bool operator>(const MetricSearchData &mp2) const {
            return (m_dist > mp2.m_dist) ||
                   (m_dist == mp2.m_dist && m_pixel.m_ref > mp2.m_pixel.m_ref);
        }
        bool operator!=(const MetricSearchData &mp2) const {
            return (m_dist != mp2.m_dist) || (m_pixel.m_ref != mp2.m_pixel.m_ref);
        }
    };

    void extractMetric(const ADRefVector<AnalysisData> &conns, std::set<MetricSearchData> &pixels,
                       const PointMap &map, const MetricSearchData &curs) const {
        // if (dist == 0.0f || concaveConnected()) { // increases effiency but is too
        // inaccurate if (dist == 0.0f || !fullyConnected()) { // increases effiency
        // but can miss lines
        if (curs.m_dist == 0.0f || curs.m_pixel.m_point.blocked() ||
            map.blockedAdjacent(curs.m_pixel.m_ref)) {
            for (auto &conn : conns) {
                auto &ad = std::get<0>(conn).get();
                if (ad.m_visitedFromBin == 0 &&
                    (ad.m_dist == -1.0 ||
                     (curs.m_dist + dist(ad.m_ref, curs.m_pixel.m_ref) < ad.m_dist))) {
                    ad.m_dist = curs.m_dist + (float)dist(ad.m_ref, curs.m_pixel.m_ref);
                    // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                    ad.m_cumAngle =
                        curs.m_pixel.m_cumAngle +
                        (!curs.m_lastpixel.has_value()
                             ? 0.0f
                             : (float)(angle(ad.m_ref, curs.m_pixel.m_ref, *curs.m_lastpixel) /
                                       (M_PI * 0.5)));
                    pixels.insert(MetricSearchData(ad, ad.m_dist, curs.m_pixel.m_ref));
                }
            }
        }
    }

    std::vector<AnalysisColumn> traverse(std::vector<AnalysisData> &analysisData,
                                         const std::vector<ADRefVector<AnalysisData>> &graph,
                                         const std::vector<PixelRef> &refs, const double radius,
                                         const std::set<PixelRef> &originRefs,
                                         const bool keepStats = false) const override {

        AnalysisColumn pathAngleCol(analysisData.size(), 0), //
            pathLengthCol(analysisData.size(), 0),           //
            euclidDistCol;                                   //

        if (originRefs.size() == 1) {
            euclidDistCol = AnalysisColumn(analysisData.size(), 0);
        }
        // in order to calculate Penn angle, the MetricPair becomes a metric triple...
        std::set<MetricSearchData> searchList; // contains root point

        for (auto &sel : originRefs) {
            auto &ad = analysisData.at(getRefIdx(refs, sel));
            searchList.insert(MetricSearchData(ad, 0.0f, std::nullopt));
        }

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only
        while (searchList.size()) {
            auto internalNode = searchList.extract(searchList.begin());
            MetricSearchData here = std::move(internalNode.value());

            if (radius != -1.0 && (here.m_dist * m_map.getSpacing()) > radius) {
                break;
            }

            auto &ad1 = here.m_pixel;
            auto &p = ad1.m_point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad1.m_visitedFromBin != ~0) {
                extractMetric(graph.at(ad1.m_attributeDataRow), searchList, m_map, here);
                ad1.m_visitedFromBin = ~0;
                pathAngleCol.setValue(ad1.m_attributeDataRow, float(ad1.m_cumAngle), keepStats);
                pathLengthCol.setValue(ad1.m_attributeDataRow,
                                       float(m_map.getSpacing() * here.m_dist), keepStats);
                if (originRefs.size() == 1) {
                    // Note: Euclidean distance is currently only calculated from a single point
                    euclidDistCol.setValue(
                        ad1.m_attributeDataRow,
                        float(m_map.getSpacing() * dist(ad1.m_ref, *originRefs.begin())),
                        keepStats);
                }
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.m_visitedFromBin != ~0) {
                        ad2.m_cumAngle = ad1.m_cumAngle;
                        pathAngleCol.setValue(ad2.m_attributeDataRow, float(ad2.m_cumAngle),
                                              keepStats);
                        pathLengthCol.setValue(ad2.m_attributeDataRow,
                                               float(m_map.getSpacing() * here.m_dist), keepStats);
                        if (originRefs.size() == 1) {
                            // Note: Euclidean distance is currently only calculated from a single
                            // point
                            euclidDistCol.setValue(
                                ad2.m_attributeDataRow,
                                float(m_map.getSpacing() *
                                      dist(p.getMergePixel(), *originRefs.begin())),
                                keepStats);
                        }
                        extractMetric(
                            graph.at(ad2.m_attributeDataRow), searchList, m_map,
                            MetricSearchData(ad2, here.m_dist + ad2.m_linkCost, std::nullopt));
                        ad2.m_visitedFromBin = ~0;
                    }
                }
            }
        }
        return {pathAngleCol, pathLengthCol, euclidDistCol};
    }

    std::tuple<std::map<PixelRef, PixelRef>>
    traverseFind(std::vector<AnalysisData> &analysisData,
                 const std::vector<ADRefVector<AnalysisData>> &graph,
                 const std::vector<PixelRef> &refs, const std::set<PixelRef> sourceRefs,
                 const PixelRef targetRef) {

        // in order to calculate Penn angle, the MetricPair becomes a metric triple...
        std::set<MetricSearchData> searchList; // contains root point

        for (const auto &sourceRef : sourceRefs) {
            auto &ad = analysisData.at(getRefIdx(refs, sourceRef));
            searchList.insert(MetricSearchData(ad, 0.0f, std::nullopt));
        }

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only
        std::map<PixelRef, PixelRef> parents;
        bool pixelFound = false;
        while (searchList.size()) {
            auto internalNode = searchList.extract(searchList.begin());
            auto here = std::move(internalNode.value());

            auto &ad = here.m_pixel;
            auto &p = ad.m_point;
            std::set<MetricSearchData> newPixels;
            std::set<MetricSearchData> mergePixels;
            if (ad.m_visitedFromBin != ~0 || (here.m_dist < ad.m_dist)) {
                extractMetric(graph.at(ad.m_attributeDataRow), newPixels, m_map, here);
                ad.m_dist = here.m_dist;
                ad.m_visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.m_visitedFromBin != ~0 || (here.m_dist + ad2.m_linkCost < ad2.m_dist)) {
                        ad2.m_dist = here.m_dist + ad2.m_linkCost;

                        auto newTripleIter =
                            newPixels.insert(MetricSearchData(ad2, ad2.m_dist, NoPixel));
                        extractMetric(graph.at(ad2.m_attributeDataRow), mergePixels, m_map,
                                      *newTripleIter.first);
                        for (auto &pixel : mergePixels) {
                            parents[pixel.m_pixel.m_ref] = p.getMergePixel();
                        }
                        ad2.m_visitedFromBin = ~0;
                    }
                }
            }
            for (auto &pixel : newPixels) {
                parents[pixel.m_pixel.m_ref] = here.m_pixel.m_ref;
            }
            newPixels.insert(mergePixels.begin(), mergePixels.end());
            for (auto &pixel : newPixels) {
                if (pixel.m_pixel.m_ref == targetRef) {
                    pixelFound = true;
                }
            }
            if (!pixelFound)
                searchList.insert(newPixels.begin(), newPixels.end());
        }
        return std::make_tuple(parents);
    }

    std::tuple<std::map<PixelRef, PixelRef>>
    traverseFindMany(std::vector<AnalysisData> &analysisData,
                     const std::vector<ADRefVector<AnalysisData>> &graph,
                     const std::vector<PixelRef> &refs, const std::set<PixelRef> sourceRefs,
                     const std::set<PixelRef> targetRefs) {

        std::set<PixelRef> targetRefsConsumable = targetRefs;
        // in order to calculate Penn angle, the MetricPair becomes a metric triple...
        std::set<MetricSearchData> searchList; // contains root point

        for (const auto &sourceRef : sourceRefs) {
            auto &ad = analysisData.at(getRefIdx(refs, sourceRef));
            searchList.insert(MetricSearchData(ad, 0.0f, std::nullopt));
        }

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only
        std::map<PixelRef, PixelRef> parents;
        while (searchList.size()) {
            auto internalNode = searchList.extract(searchList.begin());
            auto here = std::move(internalNode.value());

            auto &ad = here.m_pixel;
            auto &p = ad.m_point;
            std::set<MetricSearchData> newPixels;
            std::set<MetricSearchData> mergePixels;
            if (ad.m_visitedFromBin != ~0 || (here.m_dist < ad.m_dist)) {
                extractMetric(graph.at(ad.m_attributeDataRow), newPixels, m_map, here);
                ad.m_dist = here.m_dist;
                ad.m_visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.m_visitedFromBin != ~0 || (here.m_dist + ad2.m_linkCost < ad2.m_dist)) {
                        ad2.m_dist = here.m_dist + ad2.m_linkCost;

                        auto newTripleIter =
                            newPixels.insert(MetricSearchData(ad2, ad2.m_dist, NoPixel));
                        extractMetric(graph.at(ad2.m_attributeDataRow), mergePixels, m_map,
                                      *newTripleIter.first);
                        for (auto &pixel : mergePixels) {
                            parents[pixel.m_pixel.m_ref] = p.getMergePixel();
                        }
                        ad2.m_visitedFromBin = ~0;
                    }
                }
            }
            for (auto &pixel : newPixels) {
                parents[pixel.m_pixel.m_ref] = here.m_pixel.m_ref;
            }
            newPixels.insert(mergePixels.begin(), mergePixels.end());
            for (auto &pixel : newPixels) {
                auto it = targetRefsConsumable.find(pixel.m_pixel.m_ref);
                if (it != targetRefsConsumable.end()) {
                    targetRefsConsumable.erase(it);
                }
            }
            if (targetRefsConsumable.size() != 0)
                searchList.insert(newPixels.begin(), newPixels.end());
        }
        return std::make_tuple(parents);
    }

    // This is a slow algorithm, but should give the correct answer
    // for demonstrative purposes

    std::tuple<float, float, float, int>
    traverseSum(std::vector<AnalysisData> &analysisData,
                const std::vector<ADRefVector<AnalysisData>> &graph,
                const std::vector<PixelRef> &refs, const double radius, AnalysisData &ad0) {

        float totalDepth = 0.0f;
        float totalAngle = 0.0f;
        float euclidDepth = 0.0f;
        int totalNodes = 0;

        std::set<MetricSearchData> searchList;
        searchList.insert(MetricSearchData(ad0, 0.0f, std::nullopt));

        while (searchList.size()) {
            auto internalNode = searchList.extract(searchList.begin());
            MetricSearchData here = std::move(internalNode.value());

            if (radius != -1.0 && (here.m_dist * m_map.getSpacing()) > radius) {
                break;
            }
            auto &ad1 = here.m_pixel;
            auto &p = ad1.m_point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad1.m_visitedFromBin != ~0) {
                extractMetric(graph.at(ad1.m_attributeDataRow), searchList, m_map, here);
                ad1.m_visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.m_visitedFromBin != ~0) {
                        ad2.m_cumAngle = ad1.m_cumAngle;
                        extractMetric(graph.at(ad2.m_attributeDataRow), searchList, m_map,
                                      MetricSearchData(ad2, here.m_dist, std::nullopt));
                        ad2.m_visitedFromBin = ~0;
                    }
                }
                totalDepth += float(here.m_dist * m_map.getSpacing());
                totalAngle += ad1.m_cumAngle;
                euclidDepth += float(m_map.getSpacing() * dist(ad1.m_ref, ad0.m_ref));
                totalNodes += 1;
            }
        }
        return std::make_tuple(totalDepth, totalAngle, euclidDepth, totalNodes);
    }
};
