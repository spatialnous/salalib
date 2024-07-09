// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointmap.h"

class VGAVisualLocalOpenMP : public IAnalysis {
  private:
    PointMap &m_map;
    struct DataPoint {
        float cluster, control, controllability;
    };

    void dumpNeighbourhood(Node &node, std::set<PixelRef> &hood) const;

  public:
    struct Column {
        inline static const std::string                                      //
            VISUAL_CLUSTERING_COEFFICIENT = "Visual Clustering Coefficient", //
            VISUAL_CONTROL = "Visual Control",                               //
            VISUAL_CONTROLLABILITY = "Visual Controllability";               //
    };

  public:
    VGAVisualLocalOpenMP(PointMap &map) : m_map(map) {}
    std::string getAnalysisName() const override { return "Local Visibility Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *comm) override;
};
