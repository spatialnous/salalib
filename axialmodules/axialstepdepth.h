// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/iaxial.h"

class AxialStepDepth : IAxial {

    std::set<int> &m_originRefs;

  public:
    struct Column {
        inline static const std::string //
            STEP_DEPTH = "Step Depth";  //
    };

  public:
    AxialStepDepth(std::set<int> &originRefs) : m_originRefs(originRefs) {}
    std::string getAnalysisName() const override { return "Angular Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool simple_version) override;
};
