// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pointdata.h"

#include "genlib/simplematrix.h"

class VGAMetricOpenMP : public IAnalysis {
  private:
    PointMap &m_map;
    double m_radius;
    bool m_gates_only;

    struct DataPoint {
        float mspa, mspl, dist, count;
    };

    void extractMetric(Node &node, std::set<MetricTriple> &pixels, PointMap *pointdata,
                       const MetricTriple &curs, depthmapX::RowMatrix<int> &miscs,
                       depthmapX::RowMatrix<float> &dists, depthmapX::RowMatrix<float> &cumangles);

  public:
    VGAMetricOpenMP(PointMap &map, double radius, bool gates_only)
        : m_map(map), m_radius(radius), m_gates_only(gates_only) {}
    std::string getAnalysisName() const override { return "Metric Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *comm) override;
};
