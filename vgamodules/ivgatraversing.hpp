// SPDX-FileCopyrightText: 2018-2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface to handle different kinds of VGA analysis

#include "ivga.hpp"
#include "vgautils.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <set>
#include <vector>

class IVGATraversing : public IVGA {
  public:
    IVGATraversing(const LatticeMap &map) : IVGA(map) {}

  protected:
    std::vector<ADRefVector> getGraph(const VGAUtils::RefIndex &refIdx, bool diagonalFix) const {
        std::vector<ADRefVector> graph;

        const std::vector<PixelRef> &refs = refIdx.getRefs();
        std::vector<PixelRef> diagonalExtents;
        if (diagonalFix) {
            diagonalExtents.resize(refs.size());
        }
        for (auto &ref : refs) {
            if (diagonalFix) {
                std::copy(refs.begin(), refs.end(), diagonalExtents.begin());
            }
            auto &point = m_map.getPoint(ref);
            graph.push_back(ADRefVector());
            auto &conns = graph.back();
            for (int i = 0; i < 32; i++) {
                Bin &bin = point.getNode().bin(i);
                for (auto pixVec : bin.pixelVecs) {
                    for (PixelRef pix = pixVec.start();
                         pix.col(bin.dir) <= pixVec.end().col(bin.dir);) {
                        auto idx = refIdx.idx(pix);
                        conns.push_back({static_cast<uint32_t>(idx), i});

                        // 10.2.02 revised --- diagonal was breaking this as it was extent in
                        // diagonal or horizontal
                        if (diagonalFix && !(bin.dir & PixelRef::DIAGONAL)) {
                            auto &dx3 = diagonalExtents.at(idx);
                            if (dx3.col(bin.dir) >= pixVec.end().col(bin.dir))
                                break;
                            dx3.col(bin.dir) = pixVec.end().col(bin.dir);
                        }
                        pix.move(bin.dir);
                    }
                }
            }
        }
        return graph;
    }
    virtual std::vector<AnalysisColumn>
    traverse(std::vector<AnalysisData> &analysisData, const std::vector<ADRefVector> &graph,
             const VGAUtils::RefIndex &refIdx, const double radius,
             const std::set<PixelRef> &originRefs, const bool keepStats = false) const = 0;
};
