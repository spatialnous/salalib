// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgametric.h"
#include "salalib/pixelref.h"
#include "salalib/pointmap.h"

class VGAMetricShortestPathToMany : public IVGAMetric {
  private:
    const std::set<PixelRef> m_pixelsFrom;
    const std::set<PixelRef> m_pixelsTo;

  public:
    struct Column {
        inline static const std::string                                      //
            LINK_METRIC_COST = "Link Metric Cost",                           //
            METRIC_SHORTEST_PATH = "Metric Shortest Path",                   //
            METRIC_SHORTEST_PATH_DISTANCE = "Metric Shortest Path Distance", //
            METRIC_SHORTEST_PATH_LINKED = "Metric Shortest Path Linked",     //
            METRIC_SHORTEST_PATH_ORDER = "Metric Shortest Path Order";       //
    };

    std::string getFormattedColumn(const std::string &column, PixelRef ref) {
        return column + " " + std::to_string(ref);
    }

  public:
    std::string getAnalysisName() const override { return "Metric Shortest Path"; }
    AnalysisResult run(Communicator *) override;
    VGAMetricShortestPathToMany(PointMap &map, std::set<PixelRef> pixelsFrom,
                                std::set<PixelRef> pixelsTo)
        : IVGAMetric(map), m_pixelsFrom(pixelsFrom), m_pixelsTo(pixelsTo) {}
};
