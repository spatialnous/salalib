// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgatraversing.hpp"

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
                analysisData.back().linkCost = attRow.getRow().getValue(*linkCostIdx);
            }
            rowCounter++;
        }
        return analysisData;
    }

    // to allow a dist / PixelRef pair for easy sorting
    // (have to do comparison operation on both dist and PixelRef as
    // otherwise would have a duplicate key for pqmap / pqvector)

    struct MetricSearchData {
        AnalysisData &ad;
        float dist;
        std::optional<PixelRef> lastPixel;

      private:
        [[maybe_unused]] unsigned _padding0 : 2 * 8;
        [[maybe_unused]] unsigned _padding1 : 4 * 8;

      public:
        MetricSearchData(AnalysisData &p, float d = 0.0f, std::optional<PixelRef> lp = std::nullopt)
            : ad(p), dist(d), lastPixel(lp), _padding0(0), _padding1(0) {}
        bool operator==(const MetricSearchData &mp2) const {
            return (dist == mp2.dist && ad.ref == mp2.ad.ref);
        }
        bool operator<(const MetricSearchData &mp2) const {
            return (dist < mp2.dist) || (dist == mp2.dist && ad.ref < mp2.ad.ref);
        }
        bool operator>(const MetricSearchData &mp2) const {
            return (dist > mp2.dist) || (dist == mp2.dist && ad.ref > mp2.ad.ref);
        }
        bool operator!=(const MetricSearchData &mp2) const {
            return (dist != mp2.dist) || (ad.ref != mp2.ad.ref);
        }
    };

    void extractMetric(const ADRefVector<AnalysisData> &conns, std::set<MetricSearchData> &pixels,
                       const PointMap &map, const MetricSearchData &curs) const {
        // if (dist == 0.0f || concaveConnected()) { // increases effiency but is too
        // inaccurate if (dist == 0.0f || !fullyConnected()) { // increases effiency
        // but can miss lines
        if (curs.dist == 0.0f || curs.ad.point.blocked() || map.blockedAdjacent(curs.ad.ref)) {
            for (auto &conn : conns) {
                auto &ad = std::get<0>(conn).get();
                if (ad.visitedFromBin == 0 &&
                    (ad.dist == -1.0 || (curs.dist + dist(ad.ref, curs.ad.ref) < ad.dist))) {
                    ad.dist = curs.dist + static_cast<float>(dist(ad.ref, curs.ad.ref));
                    // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                    ad.cumAngle =
                        curs.ad.cumAngle +
                        (!curs.lastPixel.has_value()
                             ? 0.0f
                             : static_cast<float>(angle(ad.ref, curs.ad.ref, *curs.lastPixel) /
                                                  (M_PI * 0.5)));
                    pixels.insert(MetricSearchData(ad, ad.dist, curs.ad.ref));
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

            if (radius != -1.0 && (here.dist * m_map.getSpacing()) > radius) {
                break;
            }

            auto &ad1 = here.ad;
            auto &p = ad1.point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad1.visitedFromBin != ~0) {
                extractMetric(graph.at(ad1.attributeDataRow), searchList, m_map, here);
                ad1.visitedFromBin = ~0;
                pathAngleCol.setValue(ad1.attributeDataRow, static_cast<float>(ad1.cumAngle),
                                      keepStats);
                pathLengthCol.setValue(ad1.attributeDataRow,
                                       static_cast<float>(m_map.getSpacing() * here.dist),
                                       keepStats);
                if (originRefs.size() == 1) {
                    // Note: Euclidean distance is currently only calculated from a single point
                    euclidDistCol.setValue(
                        ad1.attributeDataRow,
                        static_cast<float>(m_map.getSpacing() * dist(ad1.ref, *originRefs.begin())),
                        keepStats);
                }
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.visitedFromBin != ~0) {
                        ad2.cumAngle = ad1.cumAngle;
                        pathAngleCol.setValue(ad2.attributeDataRow,
                                              static_cast<float>(ad2.cumAngle), keepStats);
                        pathLengthCol.setValue(ad2.attributeDataRow,
                                               static_cast<float>(m_map.getSpacing() * here.dist),
                                               keepStats);
                        if (originRefs.size() == 1) {
                            // Note: Euclidean distance is currently only calculated from a single
                            // point
                            euclidDistCol.setValue(
                                ad2.attributeDataRow,
                                static_cast<float>(m_map.getSpacing() *
                                                   dist(p.getMergePixel(), *originRefs.begin())),
                                keepStats);
                        }
                        extractMetric(
                            graph.at(ad2.attributeDataRow), searchList, m_map,
                            MetricSearchData(ad2, here.dist + ad2.linkCost, std::nullopt));
                        ad2.visitedFromBin = ~0;
                    }
                }
            }
        }
        return {std::move(pathAngleCol), std::move(pathLengthCol), std::move(euclidDistCol)};
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

            auto &ad = here.ad;
            auto &p = ad.point;
            std::set<MetricSearchData> newPixels;
            std::set<MetricSearchData> mergePixels;
            if (ad.visitedFromBin != ~0 || (here.dist < ad.dist)) {
                extractMetric(graph.at(ad.attributeDataRow), newPixels, m_map, here);
                ad.dist = here.dist;
                ad.visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.visitedFromBin != ~0 || (here.dist + ad2.linkCost < ad2.dist)) {
                        ad2.dist = here.dist + ad2.linkCost;

                        auto newTripleIter =
                            newPixels.insert(MetricSearchData(ad2, ad2.dist, NoPixel));
                        extractMetric(graph.at(ad2.attributeDataRow), mergePixels, m_map,
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

    std::tuple<std::map<PixelRef, PixelRef>>
    traverseFindMany(std::vector<AnalysisData> &analysisData,
                     const std::vector<ADRefVector<AnalysisData>> &graph,
                     const std::vector<PixelRef> &refs, const std::set<PixelRef> sourceRefs,
                     std::set<PixelRef> targetRefs) {

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

            auto &ad = here.ad;
            auto &p = ad.point;
            std::set<MetricSearchData> newPixels;
            std::set<MetricSearchData> mergePixels;
            if (ad.visitedFromBin != ~0 || (here.dist < ad.dist)) {
                extractMetric(graph.at(ad.attributeDataRow), newPixels, m_map, here);
                ad.dist = here.dist;
                ad.visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.visitedFromBin != ~0 || (here.dist + ad2.linkCost < ad2.dist)) {
                        ad2.dist = here.dist + ad2.linkCost;

                        auto newTripleIter =
                            newPixels.insert(MetricSearchData(ad2, ad2.dist, NoPixel));
                        extractMetric(graph.at(ad2.attributeDataRow), mergePixels, m_map,
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
                auto it = targetRefs.find(pixel.ad.ref);
                if (it != targetRefs.end()) {
                    targetRefs.erase(it);
                }
            }
            if (targetRefs.size() != 0)
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

            if (radius != -1.0 && (here.dist * m_map.getSpacing()) > radius) {
                break;
            }
            auto &ad1 = here.ad;
            auto &p = ad1.point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && ad1.visitedFromBin != ~0) {
                extractMetric(graph.at(ad1.attributeDataRow), searchList, m_map, here);
                ad1.visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.visitedFromBin != ~0) {
                        ad2.cumAngle = ad1.cumAngle;
                        extractMetric(graph.at(ad2.attributeDataRow), searchList, m_map,
                                      MetricSearchData(ad2, here.dist, std::nullopt));
                        ad2.visitedFromBin = ~0;
                    }
                }
                totalDepth += static_cast<float>(here.dist * m_map.getSpacing());
                totalAngle += ad1.cumAngle;
                euclidDepth += static_cast<float>(m_map.getSpacing() * dist(ad1.ref, ad0.ref));
                totalNodes += 1;
            }
        }
        return std::make_tuple(totalDepth, totalAngle, euclidDepth, totalNodes);
    }
};
