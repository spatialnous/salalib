// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivga.hpp"

#include "../pointmap.hpp"

class VGAVisualLocal : public IVGA {
  private:
    bool m_gatesOnly;
#ifdef USE_EXPLICIT_PADDING
    unsigned : 3 * 8; // padding
    unsigned : 4 * 8; // padding
#endif
  public:
    struct Column {
        inline static const std::string                                      //
            VISUAL_CLUSTERING_COEFFICIENT = "Visual Clustering Coefficient", //
            VISUAL_CONTROL = "Visual Control",                               //
            VISUAL_CONTROLLABILITY = "Visual Controllability";               //
    };

  public:
    std::string getAnalysisName() const override { return "Local Visibility Analysis"; }
    AnalysisResult run(Communicator *comm) override;
    VGAVisualLocal(const PointMap &map, bool gatesOnly) : IVGA(map), m_gatesOnly(gatesOnly) {}
};
