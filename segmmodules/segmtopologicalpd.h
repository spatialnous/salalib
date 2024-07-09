// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/isegment.h"

class SegmentTopologicalPD : ISegment {

    std::set<int> &m_originRefs;

  public:
    struct Column {
        inline static const std::string                        //
            TOPOLOGICAL_STEP_DEPTH = "Topological Step Depth"; //
    };

  public:
    SegmentTopologicalPD(std::set<int> &originRefs) : m_originRefs(originRefs) {}
    std::string getAnalysisName() const override { return "Topological Analysis"; }
    AnalysisResult run(Communicator *, ShapeGraph &map, bool) override;
};
