// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgaangular.h"

#include "../pointmap.h"

class VGAAngularDepth : public IVGAAngular {

    const std::set<PixelRef> &m_originRefs;

  public:
    struct Column {
        inline static const std::string                //
            ANGULAR_STEP_DEPTH = "Angular Step Depth"; //
    };

  public:
    VGAAngularDepth(const PointMap &map, std::set<PixelRef> &originRefs)
        : IVGAAngular(map), m_originRefs(originRefs) {}
    std::string getAnalysisName() const override { return "Angular Depth"; }
    AnalysisResult run(Communicator *comm) override;
};
