// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface to handle different kinds of VGA analysis

#include "ivga.hpp"

#include <numeric>
#include <string>

class IVGATraversing : public IVGA {
  public:
    IVGATraversing(const PointMap &map) : IVGA(map) {}

  protected:
    template <class T>
    std::vector<ADRefVector<T>> getGraph(std::vector<T> &analysisData,
                                         const std::vector<PixelRef> &refs,
                                         bool diagonalFix) const {
        std::vector<ADRefVector<T>> graph;
        for (auto &ad : analysisData) {
            for (auto &ad2 : analysisData) {
                ad2.diagonalExtent = ad2.ref;
            }
            auto &point = ad.point;
            graph.push_back(ADRefVector<T>());
            auto &conns = graph.back();
            for (int i = 0; i < 32; i++) {
                Bin &bin = point.getNode().bin(i);
                for (auto pixVec : bin.pixelVecs) {
                    for (PixelRef pix = pixVec.start();
                         pix.col(bin.dir) <= pixVec.end().col(bin.dir);) {
                        auto &ad3 = analysisData.at(getRefIdx(refs, pix));
                        conns.push_back({ad3, i});

                        // 10.2.02 revised --- diagonal was breaking this as it was extent in
                        // diagonal or horizontal
                        if (diagonalFix && !(bin.dir & PixelRef::DIAGONAL)) {
                            if (ad3.diagonalExtent.col(bin.dir) >= pixVec.end().col(bin.dir))
                                break;
                            ad3.diagonalExtent.col(bin.dir) = pixVec.end().col(bin.dir);
                        }
                        pix.move(bin.dir);
                    }
                }
            }
        }
        return graph;
    }
    virtual std::vector<AnalysisColumn>
    traverse(std::vector<AnalysisData> &analysisData,
             const std::vector<ADRefVector<AnalysisData>> &graph, const std::vector<PixelRef> &refs,
             const double radius, const std::set<PixelRef> &originRefs,
             const bool keepStats = false) const = 0;
};
