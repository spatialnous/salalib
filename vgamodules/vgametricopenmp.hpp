// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../genlib/stringutils.hpp"
#include "../pointmap.hpp"
#include "ivgametric.hpp"

class VGAMetricOpenMP : public IVGAMetric {
    double m_radius;

    std::optional<int> m_limitToThreads;

    bool m_gatesOnly;
    bool m_forceCommUpdatesMasterThread = false;

    // To maintain binary compatibility with older .graph versions
    // write the last "misc" values back to the points
    bool m_legacyWriteMiscs = false;

    [[maybe_unused]] unsigned _padding0 : 1 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

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
    static std::string getColumnWithRadius(std::string column, double radius, Region4f mapRegion) {
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
        : IVGAMetric(map), m_radius(radius), m_limitToThreads(limitToThreads),
          m_gatesOnly(gatesOnly), m_forceCommUpdatesMasterThread(forceCommUpdatesMasterThread),
          _padding0(0), _padding1(0) {}
    std::string getAnalysisName() const override { return "Metric Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *comm) override;

  public:
    void setLegacyWriteMiscs(bool legacyWriteMiscs) { m_legacyWriteMiscs = legacyWriteMiscs; }
};
