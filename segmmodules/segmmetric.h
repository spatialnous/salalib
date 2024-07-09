// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/isegment.h"

class SegmentMetric : ISegment {
  private:
    double m_radius;
    std::optional<const std::set<int>> m_selSet;

  public:
    struct Column {
        inline static const std::string                        //
            METRIC_CHOICE = "Metric Choice",                   //
            METRIC_CHOICE_SLW = "Metric Choice [SLW]",         //
            METRIC_MEAN_DEPTH = "Metric Mean Depth",           //
            METRIC_MEAN_DEPTH_SLW = "Metric Mean Depth [SLW]", //
            METRIC_TOTAL_DEPTH = "Metric Total Depth",         //
            METRIC_TOTAL_NODES = "Metric Total Nodes",         //
            METRIC_TOTAL_LENGTH = "Metric Total Length";       //
    };
    static std::string getFormattedColumn(std::string column, double radius) {
        std::string colName = column;
        if (radius != -1.0) {
            colName += dXstring::formatString(radius, " R%.f metric");
        }
        return colName;
    }

  public:
    SegmentMetric(double radius, std::optional<const std::set<int>> selSet)
        : m_radius(radius), m_selSet(selSet) {}
    std::string getAnalysisName() const override { return "Metric Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool) override;
};
