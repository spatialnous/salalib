// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ivga.h"
#include "salalib/pointmap.h"

#include "genlib/stringutils.h"

class VGAMetric : IVGA {
  private:
    double m_radius;
    bool m_gates_only;

  public:
    struct Column {
        inline static const std::string                                                //
            METRIC_MEAN_SHORTEST_PATH_ANGLE = "Metric Mean Shortest-Path Angle",       //
            METRIC_MEAN_SHORTEST_PATH_DISTANCE = "Metric Mean Shortest-Path Distance", //
            METRIC_MEAN_STRAIGHT_LINE_DISTANCE = "Metric Mean Straight-Line Distance", //
            METRIC_NODE_COUNT = "Metric Node Count";                                   //
    };
    static std::string getColumnWithRadius(std::string column, double radius, QtRegion mapRegion) {
        if (radius != -1.0) {
            if (radius > 100.0) {
                return column + " R" + dXstring::formatString(radius, "%.f");
            } else if (mapRegion.width() < 1.0) {
                return column + " R" + dXstring::formatString(radius, "%.4f");
            } else {
                return column + " R" + dXstring::formatString(radius, "%.2f");
            }
        }
        return column;
    }

  public:
    std::string getAnalysisName() const override { return "Metric Analysis"; }
    AnalysisResult run(Communicator *comm, PointMap &map, bool) override;
    VGAMetric(double radius, bool gates_only) : m_radius(radius), m_gates_only(gates_only) {}
};
