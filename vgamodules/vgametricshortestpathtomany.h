// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

class VGAMetricShortestPathToMany : public IAnalysis {
  private:
    PointMap &m_map;
    const std::set<PixelRef> m_pixelsFrom;
    const std::set<PixelRef> m_pixelsTo;

    struct MetricPoint {
        Point *m_point = nullptr;
        float m_linkCost = 0;
        float m_dist = -1.0f;
        float m_cumdist = -1.0f;
        bool m_unseen = true;
    };
    MetricPoint &getMetricPoint(depthmapX::ColumnMatrix<MetricPoint> &metricPoints, PixelRef ref) {
        return (metricPoints(static_cast<size_t>(ref.y), static_cast<size_t>(ref.x)));
    }
    void extractMetric(Node n, depthmapX::ColumnMatrix<MetricPoint> &metricPoints,
                       std::set<MetricTriple> &pixels, PointMap *pointdata,
                       const MetricTriple &curs);

  public:
    std::string getAnalysisName() const override { return "Metric Shortest Path"; }
    AnalysisResult run(Communicator *) override;
    VGAMetricShortestPathToMany(PointMap &map, std::set<PixelRef> pixelsFrom,
                                std::set<PixelRef> pixelsTo)
        : m_map(map), m_pixelsFrom(pixelsFrom), m_pixelsTo(pixelsTo) {}
};
