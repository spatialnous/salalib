// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../isegment.hpp"

class SegmentMetric : ISegment {
    double m_radius;
    std::optional<std::set<int>> m_selSet;

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
    static std::string getFormattedColumn(const std::string &column, double radius) {
        std::string colName = column;
        if (radius != -1.0) {
            colName += dXstring::formatString(radius, " R%.f metric");
        }
        return colName;
    }

  public:
    SegmentMetric(double radius, std::optional<std::set<int>> selSet)
        : m_radius(radius), m_selSet(std::move(selSet)) {}
    std::string getAnalysisName() const override { return "Metric Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool) override;
};
