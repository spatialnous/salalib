// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ivga.h"
#include "salalib/pixelref.h"
#include "salalib/pointmap.h"

#include "genlib/simplematrix.h"

class VGAVisualGlobalDepth : IVGA {

    const std::set<PixelRef> &m_originRefs;

  public:
    struct Column {
        inline static const std::string              //
            VISUAL_STEP_DEPTH = "Visual Step Depth"; //
    };

  public:
    VGAVisualGlobalDepth(std::set<PixelRef> &originRefs) : m_originRefs(originRefs) {}
    std::string getAnalysisName() const override { return "Global Visibility Depth"; }
    AnalysisResult run(Communicator *comm, PointMap &map, bool simple_version) override;
    void extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
                       depthmapX::RowMatrix<PixelRef> &extents);
};
