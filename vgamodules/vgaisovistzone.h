// SPDX-FileCopyrightText: 2019-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointmap.h"
#include <iomanip>

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
                                   std::optional<const std::set<int>> selectionSet);

  public:
    struct Column {
        inline static const std::string                                            //
            ISOVIST_ZONE_DISTANCE = "Isovist Zone Distance",                       //
            ISOVIST_ZONE_INV_SQ_DISTANCE = "Isovist Zone Inverse Square Distance"; //
    };

    static std::string getFormattedColumn(const std::string &column,
                                          std::optional<std::string> originPointSetName,
                                          float restrictDistance) {
        std::string colName = column;

        if (originPointSetName.has_value()) {
            colName += " [" + originPointSetName.value() + "]";
        }

        if (restrictDistance > 0) {
            std::stringstream restrictionText;
            restrictionText << std::fixed << std::setprecision(2) << " (" << restrictDistance << ")"
                            << std::flush;
            colName += restrictionText.str();
        }
        return colName;
    }

  public:
    std::string getAnalysisName() const override { return "Path Zone"; }
    AnalysisResult run(Communicator *) override;
    VGAIsovistZone(PointMap &map, std::map<std::string, std::set<PixelRef>> originPointSets,
                   float restrictDistance = -1)
        : m_map(map), m_originPointSets(originPointSets), m_restrictDistance(restrictDistance) {}
};
