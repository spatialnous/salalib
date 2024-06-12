// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/shapegraph.h"

class SegmentMetricShortestPath : public IAnalysis {
  private:
    ShapeGraph &m_map;

  public:
    SegmentMetricShortestPath(ShapeGraph &map) : m_map(map) {}
    std::string getAnalysisName() const override { return "Metric Shortest Path"; }
    AnalysisResult run(Communicator *) override;
};
