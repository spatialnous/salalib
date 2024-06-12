// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/isegment.h"

class SegmentTopological : ISegment {
  private:
    double m_radius;
    bool m_sel_only;

  public:
    std::string getAnalysisName() const override { return "Topological Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool) override;
    SegmentTopological(double radius, bool sel_only) : m_radius(radius), m_sel_only(sel_only) {}
};
