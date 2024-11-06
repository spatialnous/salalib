// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../isegment.h"

class SegmentTulipDepth : ISegment {
    int m_tulipBins = 1024;

    std::set<int> &m_originRefs;

  public:
    struct Column {
        inline static const std::string                //
            ANGULAR_STEP_DEPTH = "Angular Step Depth"; //
    };

  public:
    SegmentTulipDepth(int tulipBins, std::set<int> &originRefs)
        : m_tulipBins(tulipBins), m_originRefs(originRefs) {}
    std::string getAnalysisName() const override { return "Tulip Analysis"; }
    AnalysisResult run(Communicator *, ShapeGraph &map, bool) override;
};
