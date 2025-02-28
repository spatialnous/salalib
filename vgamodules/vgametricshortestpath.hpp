// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../pixelref.hpp"
#include "../pointmap.hpp"

#include "ivgametric.hpp"

class VGAMetricShortestPath : public IVGAMetric {
    std::set<PixelRef> m_pixelsFrom;
    PixelRef m_pixelTo;

    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    struct Column {
        inline static const std::string                                                    //
            LINK_METRIC_COST = "Link Metric Cost",                                         //
            METRIC_SHORTEST_PATH = "Metric Shortest Path",                                 //
            METRIC_SHORTEST_PATH_DISTANCE = "Metric Shortest Path Distance",               //
            METRIC_SHORTEST_PATH_LINKED = "Metric Shortest Path Linked",                   //
            METRIC_SHORTEST_PATH_ORDER = "Metric Shortest Path Order",                     //
            METRIC_SHORTEST_PATH_VISUAL_ZONE = "Metric Shortest Path Visual Zone",         //
            METRIC_SHORTEST_PATH_METRIC_ZONE = "Metric Shortest Path Metric Zone",         //
            METRIC_SHORTEST_PATH_INV_METRIC_ZONE = "Metric Shortest Path Inv Metric Zone"; //
    };

  public:
    std::string getAnalysisName() const override { return "Metric Shortest Path"; }
    AnalysisResult run(Communicator *) override;
    VGAMetricShortestPath(PointMap &map, std::set<PixelRef> pixelsFrom, PixelRef pixelTo)
        : IVGAMetric(map), m_pixelsFrom(pixelsFrom), m_pixelTo(pixelTo), _padding0(0) {}
};
