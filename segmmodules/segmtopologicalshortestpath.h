// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/shapegraph.h"

class SegmentTopologicalShortestPath : public IAnalysis {
  private:
    ShapeGraph &m_map;
    int m_refFrom, m_refTo;

  public:
    struct Column {
        inline static const std::string                                          //
            TOPOLOGICAL_SHORTEST_PATH_DEPTH = "Topological Shortest Path Depth", //
            TOPOLOGICAL_SHORTEST_PATH_ORDER = "Topological Shortest Path Order"; //
    };

  public:
    SegmentTopologicalShortestPath(ShapeGraph &map, int refFrom, int refTo)
        : m_map(map), m_refFrom(refFrom), m_refTo(refTo) {}
    std::string getAnalysisName() const override { return "Topological Shortest Path"; }
    AnalysisResult run(Communicator *) override;
};
