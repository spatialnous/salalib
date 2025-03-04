// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../isegment.hpp"

class SegmentTulipDepth : ISegment {
    std::set<int> m_originRefs;
    int m_tulipBins = 1024;

    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    struct Column {
        inline static const std::string                //
            ANGULAR_STEP_DEPTH = "Angular Step Depth"; //
    };

  public:
    SegmentTulipDepth(int tulipBins, std::set<int> originRefs)
        : m_originRefs(std::move(originRefs)), m_tulipBins(tulipBins), _padding0(0) {}
    std::string getAnalysisName() const override { return "Tulip Analysis"; }
    AnalysisResult run(Communicator *, ShapeGraph &map, bool) override;
};
