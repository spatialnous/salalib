// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../isegment.h"

class SegmentTopological : ISegment {
  private:
    double m_radius;
    std::optional<const std::set<int>> m_selSet;

  public:
    struct Column {
        inline static const std::string                                  //
            TOPOLOGICAL_CHOICE = "Topological Choice",                   //
            TOPOLOGICAL_CHOICE_SLW = "Topological Choice [SLW]",         //
            TOPOLOGICAL_MEAN_DEPTH = "Topological Mean Depth",           //
            TOPOLOGICAL_MEAN_DEPTH_SLW = "Topological Mean Depth [SLW]", //
            TOPOLOGICAL_TOTAL_DEPTH = "Topological Total Depth",         //
            TOPOLOGICAL_TOTAL_NODES = "Topological Total Nodes",         //
            TOPOLOGICAL_TOTAL_LENGTH = "Topological Total Length";       //
    };
    static std::string getFormattedColumn(std::string column, double radius) {
        std::string colName = column;
        if (radius != -1.0) {
            // TODO: This should end in "topological" not "metric"
            colName += dXstring::formatString(radius, " R%.f metric");
        }
        return colName;
    }

  public:
    SegmentTopological(double radius, std::optional<const std::set<int>> selSet)
        : m_radius(radius), m_selSet(selSet) {}
    std::string getAnalysisName() const override { return "Topological Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool) override;
};
