// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ivga.h"
#include "salalib/pointmap.h"

class VGAThroughVision : IVGA {
  public:
    struct Column {
        inline static const std::string        //
            THROUGH_VISION = "Through vision"; //
    };

  public:
    std::string getAnalysisName() const override { return "Through Vision Analysis"; }
    AnalysisResult run(Communicator *comm, PointMap &map, bool) override;
};
