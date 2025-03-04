// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgavisual.hpp"

#include "../pixelref.hpp"
#include "../pointmap.hpp"

class VGAVisualGlobalDepth : public IVGAVisual {

    std::set<PixelRef> m_originRefs;

  public:
    struct Column {
        inline static const std::string              //
            VISUAL_STEP_DEPTH = "Visual Step Depth"; //
    };

  public:
    VGAVisualGlobalDepth(const PointMap &map, std::set<PixelRef> originRefs)
        : IVGAVisual(map), m_originRefs(std::move(originRefs)) {}
    std::string getAnalysisName() const override { return "Global Visibility Depth"; }
    AnalysisResult run(Communicator *comm) override;
};
