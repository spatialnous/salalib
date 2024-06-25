// SPDX-FileCopyrightText: 2019-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

class VGAIsovistZone : public IAnalysis {
  private:
    PointMap &m_map;
    std::map<std::string, std::set<PixelRef>> m_originPointSets;
    float m_restrictDistance;

    struct MetricPoint {
        Point *m_point = nullptr;
    };
    MetricPoint &getMetricPoint(depthmapX::ColumnMatrix<MetricPoint> &metricPoints, PixelRef ref) {
        return (metricPoints(static_cast<size_t>(ref.y), static_cast<size_t>(ref.x)));
    }
    void extractMetric(Node n, std::set<MetricTriple> &pixels, PointMap &map,
                       const MetricTriple &curs);
    void setColumnFormulaAndUpdate(PointMap &pointmap, int columnIndex, std::string formula,
                                   bool selectionOnly);

  public:
    std::string getAnalysisName() const override { return "Path Zone"; }
    AnalysisResult run(Communicator *) override;
    VGAIsovistZone(PointMap &map, std::map<std::string, std::set<PixelRef>> originPointSets,
                   float restrictDistance = -1)
        : m_map(map), m_originPointSets(originPointSets), m_restrictDistance(restrictDistance) {}
};
