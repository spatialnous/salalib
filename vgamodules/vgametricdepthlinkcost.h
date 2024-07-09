// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointmap.h"

class VGAMetricDepthLinkCost : public IAnalysis {
  private:
    PointMap &m_map;
    std::set<int> m_pixelsFrom;

    struct MetricPoint {
        Point *m_point = nullptr;
        float m_linkCost = 0;
        float m_dist = -1.0f;
        bool m_unseen = true;
    };
    MetricPoint &getMetricPoint(depthmapX::ColumnMatrix<MetricPoint> &metricPoints, PixelRef ref) {
        return (metricPoints(static_cast<size_t>(ref.y), static_cast<size_t>(ref.x)));
    }
    void extractMetric(Node n, depthmapX::ColumnMatrix<MetricPoint> &metricPoints,
                       std::set<MetricTriple> &pixels, PointMap *pointdata,
                       const MetricTriple &curs);

  public:
    struct Column {
        inline static const std::string              //
            LINK_METRIC_COST = "Link Metric Cost",   //
            METRIC_STEP_DEPTH = "Metric Step Depth"; //
    };

  public:
    std::string getAnalysisName() const override { return "Metric Depth"; }
    AnalysisResult run(Communicator *) override;
    VGAMetricDepthLinkCost(PointMap &map, std::set<int> pixelsFrom)
        : m_map(map), m_pixelsFrom(pixelsFrom) {}
};
