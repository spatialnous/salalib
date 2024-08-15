// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgatraversing.h"

class IVGAMetric : public IVGATraversing {
  protected:
    IVGAMetric(const PointMap &map) : IVGATraversing(map) {}

    std::vector<AnalysisData> getAnalysisData(const AttributeTable &attributes) {
        std::vector<AnalysisData> analysisData;
        analysisData.reserve(attributes.getNumRows());

        size_t rowCounter = 0;
        for (auto &attRow : attributes) {
            auto &point = m_map.getPoint(attRow.getKey().value);
            analysisData.push_back(AnalysisData(point, attRow.getKey().value, rowCounter, 0,
                                                attRow.getKey().value, -1.0f, 0.0f));
            rowCounter++;
        }
        return analysisData;
    }

    // to allow a dist / PixelRef pair for easy sorting
    // (have to do comparison operation on both dist and PixelRef as
    // otherwise would have a duplicate key for pqmap / pqvector)

    struct MetricSearchData {
        AnalysisData &pixel;
        float dist;
        std::optional<PixelRef> lastpixel;
        MetricSearchData(AnalysisData &p, float d = 0.0f, std::optional<PixelRef> lp = std::nullopt)
            : pixel(p), dist(d), lastpixel(lp) {}
        bool operator==(const MetricSearchData &mp2) const {
            return (dist == mp2.dist && pixel.m_ref == mp2.pixel.m_ref);
        }
        bool operator<(const MetricSearchData &mp2) const {
            return (dist < mp2.dist) || (dist == mp2.dist && pixel.m_ref < mp2.pixel.m_ref);
        }
        bool operator>(const MetricSearchData &mp2) const {
            return (dist > mp2.dist) || (dist == mp2.dist && pixel.m_ref > mp2.pixel.m_ref);
        }
        bool operator!=(const MetricSearchData &mp2) const {
            return (dist != mp2.dist) || (pixel.m_ref != mp2.pixel.m_ref);
        }
    };

    void extractMetric(const ADRefVector<AnalysisData> &conns, std::set<MetricSearchData> &pixels,
                       const PointMap &map, const MetricSearchData &curs) const {
        // if (dist == 0.0f || concaveConnected()) { // increases effiency but is too
        // inaccurate if (dist == 0.0f || !fullyConnected()) { // increases effiency
        // but can miss lines
        if (curs.dist == 0.0f || curs.pixel.m_point.blocked() ||
            map.blockedAdjacent(curs.pixel.m_ref)) {
            for (auto &conn : conns) {
                auto &ad = std::get<0>(conn).get();
                if (ad.m_visitedFromBin == 0 &&
                    (ad.m_dist == -1.0 ||
                     (curs.dist + dist(ad.m_ref, curs.pixel.m_ref) < ad.m_dist))) {
                    ad.m_dist = curs.dist + (float)dist(ad.m_ref, curs.pixel.m_ref);
                    // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                    ad.m_cumAngle =
                        curs.pixel.m_cumAngle +
                        (!curs.lastpixel.has_value()
                             ? 0.0f
                             : (float)(angle(ad.m_ref, curs.pixel.m_ref, *curs.lastpixel) /
                                       (M_PI * 0.5)));
                    pixels.insert(MetricSearchData(ad, ad.m_dist, curs.pixel.m_ref));
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
            distCol;                                         //

        if (originRefs.size() == 1) {
            distCol = AnalysisColumn(analysisData.size(), 0);
        }
        // in order to calculate Penn angle, the MetricPair becomes a metric triple...
        std::set<MetricSearchData> search_list; // contains root point

        for (auto &sel : originRefs) {
            auto &ad = analysisData.at(getRefIdx(refs, sel));
            search_list.insert(MetricSearchData(ad, 0.0f, std::nullopt));
        }

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only
        while (search_list.size()) {
            auto internalNode = search_list.extract(search_list.begin());
            MetricSearchData here = std::move(internalNode.value());

            if (radius != -1.0 && (here.dist * m_map.getSpacing()) > radius) {
                break;
            }

            auto &ad1 = here.pixel;
            auto &p = ad1.m_point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad1.m_visitedFromBin != ~0) {
                extractMetric(graph.at(ad1.m_attributeDataRow), search_list, m_map, here);
                ad1.m_visitedFromBin = ~0;
                pathAngleCol.setValue(ad1.m_attributeDataRow, float(ad1.m_cumAngle), keepStats);
                pathLengthCol.setValue(ad1.m_attributeDataRow,
                                       float(m_map.getSpacing() * here.dist), keepStats);
                if (originRefs.size() == 1) {
                    // Note: Euclidean distance is currently only calculated from a single point
                    distCol.setValue(
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
                                               float(m_map.getSpacing() * here.dist), keepStats);
                        if (originRefs.size() == 1) {
                            // Note: Euclidean distance is currently only calculated from a single
                            // point
                            distCol.setValue(ad2.m_attributeDataRow,
                                             float(m_map.getSpacing() *
                                                   dist(p.getMergePixel(), *originRefs.begin())),
                                             keepStats);
                        }
                        extractMetric(graph.at(ad2.m_attributeDataRow), search_list, m_map,
                                      MetricSearchData(ad2, here.dist, std::nullopt));
                        ad2.m_visitedFromBin = ~0;
                    }
                }
            }
        }
        return {pathAngleCol, pathLengthCol, distCol};
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

        std::set<MetricSearchData> search_list;
        search_list.insert(MetricSearchData(ad0, 0.0f, std::nullopt));

        while (search_list.size()) {
            auto internalNode = search_list.extract(search_list.begin());
            MetricSearchData here = std::move(internalNode.value());

            if (radius != -1.0 && (here.dist * m_map.getSpacing()) > radius) {
                break;
            }
            auto &ad1 = here.pixel;
            auto &p = ad1.m_point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad1.m_visitedFromBin != ~0) {
                extractMetric(graph.at(ad1.m_attributeDataRow), search_list, m_map, here);
                ad1.m_visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.m_visitedFromBin != ~0) {
                        ad2.m_cumAngle = ad1.m_cumAngle;
                        extractMetric(graph.at(ad2.m_attributeDataRow), search_list, m_map,
                                      MetricSearchData(ad2, here.dist, std::nullopt));
                        ad2.m_visitedFromBin = ~0;
                    }
                }
                totalDepth += float(here.dist * m_map.getSpacing());
                totalAngle += ad1.m_cumAngle;
                euclidDepth += float(m_map.getSpacing() * dist(ad1.m_ref, ad0.m_ref));
                totalNodes += 1;
            }
        }
        return std::make_tuple(totalDepth, totalAngle, euclidDepth, totalNodes);
    }
};
