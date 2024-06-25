// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pointdata.h"

#include "genlib/simplematrix.h"

class VGAAngularOpenMP : public IAnalysis {
  private:
    PointMap &m_map;
    double m_radius;
    bool m_gates_only;

    struct DataPoint {
        float total_depth, mean_depth, count;
    };

    void extractAngular(Node &node, std::set<AngularTriple> &pixels, PointMap *pointdata,
                        const AngularTriple &curs, depthmapX::RowMatrix<int> &miscs,
                        depthmapX::RowMatrix<float> &cumangles);

  public:
    VGAAngularOpenMP(PointMap &map, double radius, bool gates_only)
        : m_map(map), m_radius(radius), m_gates_only(gates_only) {}
    std::string getAnalysisName() const override { return "Angular Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *comm) override;
};
