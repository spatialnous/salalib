// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../ianalysis.hpp"
#include "../pixelref.hpp"
#include "../pointmap.hpp"

class VGAVisualLocalAdjMatrix : public IAnalysis {
  private:
    PointMap &m_map;
    std::optional<int> m_limitToThreads;
    bool m_gatesOnly;
    bool m_forceCommUpdatesMasterThread = false;
#ifdef USE_EXPLICIT_PADDING
    unsigned : 2 * 8; // padding
    unsigned : 4 * 8; // padding
#endif
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
    VGAVisualLocalAdjMatrix(PointMap &map, bool gatesOnly,
                            std::optional<int> limitToThreads = std::nullopt,
                            bool forceCommUpdatesMasterThread = false)
        : m_map(map), m_limitToThreads(limitToThreads), m_gatesOnly(gatesOnly),
          m_forceCommUpdatesMasterThread(forceCommUpdatesMasterThread) {}
    std::string getAnalysisName() const override {
        return "Local Visibility Analysis (Adj. Matrix)";
    }
    AnalysisResult run(Communicator *comm) override;
};
