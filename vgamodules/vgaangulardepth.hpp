// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgaangular.hpp"

#include "../latticemap.hpp"

#include <set>
#include <string>
#include <string_view>
#include <utility>

class VGAAngularDepth : public IVGAAngular {

    std::set<PixelRef> m_originRefs;

  public:
    struct Column {
        static constexpr std::string_view              //
            ANGULAR_STEP_DEPTH = "Angular Step Depth"; //
    };

  public:
    VGAAngularDepth(const LatticeMap &map, std::set<PixelRef> originRefs)
        : IVGAAngular(map), m_originRefs(std::move(originRefs)) {}
    std::string getAnalysisName() const override { return "Angular Depth"; }
    AnalysisResult run(Communicator *comm) override;
};
