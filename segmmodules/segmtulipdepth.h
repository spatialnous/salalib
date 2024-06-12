// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/isegment.h"

class SegmentTulipDepth : ISegment {
    int m_tulip_bins = 1024;

  public:
    std::string getAnalysisName() const override { return "Tulip Analysis"; }
    SegmentTulipDepth(int tulip_bins) : m_tulip_bins(tulip_bins) {}
    AnalysisResult run(Communicator *, ShapeGraph &map, bool) override;
};
