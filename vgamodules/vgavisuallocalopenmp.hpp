// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../ianalysis.hpp"
#include "../pixelref.hpp"
#include "../pointmap.hpp"

class VGAVisualLocalOpenMP : public IAnalysis {
    PointMap &m_map;
    std::optional<int> m_limitToThreads;
    bool m_forceCommUpdatesMasterThread = false;

    [[maybe_unused]] unsigned _padding0 : 3 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

    struct DataPoint {
        float cluster, control, controllability;
    };

    std::set<PixelRef> getNeighbourhood(const Node &node) const;

  public:
    struct Column {
        inline static const std::string                                      //
            VISUAL_CLUSTERING_COEFFICIENT = "Visual Clustering Coefficient", //
            VISUAL_CONTROL = "Visual Control",                               //
            VISUAL_CONTROLLABILITY = "Visual Controllability";               //
    };

  public:
    VGAVisualLocalOpenMP(PointMap &map, std::optional<int> limitToThreads = std::nullopt,
                         bool forceCommUpdatesMasterThread = false)
        : m_map(map), m_limitToThreads(limitToThreads),
          m_forceCommUpdatesMasterThread(forceCommUpdatesMasterThread), _padding0(0), _padding1(0) {
    }
    std::string getAnalysisName() const override { return "Local Visibility Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *comm) override;
};
