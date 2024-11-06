// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgametric.h"

#include "../pixelref.h"
#include "../pointmap.h"

class VGAMetricDepthLinkCost : public IVGAMetric {

  private:
    std::set<PixelRef> m_pixelsFrom;

  public:
    struct Column {
        inline static const std::string              //
            LINK_METRIC_COST = "Link Metric Cost",   //
            METRIC_STEP_DEPTH = "Metric Step Depth"; //
    };

  public:
    std::string getAnalysisName() const override { return "Metric Depth"; }
    AnalysisResult run(Communicator *) override;
    VGAMetricDepthLinkCost(PointMap &map, std::set<PixelRef> pixelsFrom)
        : IVGAMetric(map), m_pixelsFrom(pixelsFrom) {}
};
