// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

class VGAVisualLocalAdjMatrix : public IAnalysis {
  private:
    PointMap &m_map;
    bool m_gates_only;

    struct DataPoint {
        float cluster, control, controllability;
    };
    void dumpNeighbourhood(Node &node, std::set<PixelRef> &hood) const;

  public:
    VGAVisualLocalAdjMatrix(PointMap &map, bool gates_only)
        : m_map(map), m_gates_only(gates_only) {}
    std::string getAnalysisName() const override { return "Local Visibility Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *comm) override;
};
