// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

#include "genlib/simplematrix.h"

class VGAVisualGlobalOpenMP : public IAnalysis {
  private:
    PointMap &m_map;
    double m_radius;
    bool m_gatesOnly;

    struct DataPoint {
        float count, depth, integ_dv, integ_pv;
        float integ_tk, entropy, rel_entropy;
    };
    void extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
                       depthmapX::RowMatrix<PixelRef> &extents);

  public:
    VGAVisualGlobalOpenMP(PointMap &map, double radius, bool gatesOnly)
        : m_map(map), m_radius(radius), m_gatesOnly(gatesOnly) {}
    std::string getAnalysisName() const override { return "Global Visibility Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *) override;
};
