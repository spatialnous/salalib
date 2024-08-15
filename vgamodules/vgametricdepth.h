// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgametric.h"
#include "salalib/pointmap.h"

class VGAMetricDepth : public IVGAMetric {

    const std::set<PixelRef> &m_originRefs;

  public:
    struct Column {
        inline static const std::string                                            //
            METRIC_STEP_SHORTEST_PATH_ANGLE = "Metric Step Shortest-Path Angle",   //
            METRIC_STEP_SHORTEST_PATH_LENGTH = "Metric Step Shortest-Path Length", //
            METRIC_STRAIGHT_LINE_DISTANCE = "Metric Straight-Line Distance";       //
    };

  public:
    VGAMetricDepth(const PointMap &map, std::set<PixelRef> &originRefs)
        : IVGAMetric(map), m_originRefs(originRefs) {}
    std::string getAnalysisName() const override { return "Metric Depth"; }
    AnalysisResult run(Communicator *) override;
};
