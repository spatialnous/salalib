// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../ianalysis.hpp"
#include "../shapegraph.hpp"

#include <cstddef>
#include <string>
#include <string_view>

class SegmentTulipShortestPath : public IAnalysis {
  private:
    ShapeGraph &m_map;
    size_t m_tulipBins;
    int m_refFrom, m_refTo;

    pafmath::Pafrand m_rng;

  public:
    struct Column {
        static constexpr std::string_view                                //
            ANGULAR_SHORTEST_PATH_ANGLE = "Angular Shortest Path Angle", //
            ANGULAR_SHORTEST_PATH_ORDER = "Angular Shortest Path Order"; //
    };

  public:
    SegmentTulipShortestPath(ShapeGraph &map, size_t tulipBins, int refFrom, int refTo,
                             unsigned int seed)
        : m_map(map), m_tulipBins(tulipBins), m_refFrom(refFrom), m_refTo(refTo), m_rng(seed) {}
    std::string getAnalysisName() const override { return "Tulip Shortest Path"; }
    AnalysisResult run(Communicator *) override;
};
