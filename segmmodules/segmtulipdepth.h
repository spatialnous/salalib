// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/isegment.h"

class SegmentTulipDepth : ISegment {
    int m_tulip_bins = 1024;

    std::set<int> &m_originRefs;

  public:
    struct Column {
        inline static const std::string                //
            ANGULAR_STEP_DEPTH = "Angular Step Depth"; //
    };

  public:
    SegmentTulipDepth(int tulip_bins, std::set<int> &originRefs)
        : m_tulip_bins(tulip_bins), m_originRefs(originRefs) {}
    std::string getAnalysisName() const override { return "Tulip Analysis"; }
    AnalysisResult run(Communicator *, ShapeGraph &map, bool) override;
};
