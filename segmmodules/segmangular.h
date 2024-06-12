// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/isegment.h"

class SegmentAngular : ISegment {
  private:
    std::set<double> m_radius_set;

  public:
    std::string getAnalysisName() const override { return "Angular Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool) override;
    SegmentAngular(std::set<double> radius_set) : m_radius_set(radius_set) {}
};
