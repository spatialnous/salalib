// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgametric.hpp"

#include "../latticemap.hpp"
#include "../pixelref.hpp"

#include <set>
#include <string>
#include <string_view>

class VGAMetricDepthLinkCost : public IVGAMetric {

  private:
    std::set<PixelRef> &m_pixelsFrom;

  public:
    struct Column {
        static constexpr std::string_view            //
            LINK_METRIC_COST = "Link Metric Cost",   //
            METRIC_STEP_DEPTH = "Metric Step Depth"; //
    };

  public:
    std::string getAnalysisName() const override { return "Metric Depth"; }
    AnalysisResult run(Communicator *) override;
    VGAMetricDepthLinkCost(LatticeMap &map, std::set<PixelRef> &pixelsFrom)
        : IVGAMetric(map), m_pixelsFrom(pixelsFrom) {}
};
