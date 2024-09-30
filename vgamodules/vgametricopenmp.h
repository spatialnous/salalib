// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../genlib/stringutils.h"
#include "../pointmap.h"
#include "ivgametric.h"

class VGAMetricOpenMP : public IVGAMetric {
  private:
    double m_radius;
    bool m_gatesOnly;
    std::optional<int> m_limitToThreads;
    bool m_forceCommUpdatesMasterThread = false;

    // To maintain binary compatibility with older .graph versions
    // write the last "misc" values back to the points
    bool m_legacyWriteMiscs = false;

    struct DataPoint {
        float mspa, mspl, dist, count;
    };

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
    VGAMetricOpenMP(const PointMap &map, double radius, bool gatesOnly,
                    std::optional<int> limitToThreads = std::nullopt,
                    bool forceCommUpdatesMasterThread = false)
        : IVGAMetric(map), m_radius(radius), m_gatesOnly(gatesOnly),
          m_limitToThreads(limitToThreads),
          m_forceCommUpdatesMasterThread(forceCommUpdatesMasterThread) {}
    std::string getAnalysisName() const override { return "Metric Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *comm) override;

  public:
    void setLegacyWriteMiscs(bool legacyWriteMiscs) { m_legacyWriteMiscs = legacyWriteMiscs; }
};
