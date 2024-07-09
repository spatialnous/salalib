// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ivga.h"
#include "salalib/pointmap.h"

class AgentAnalysis : IVGA {

  public:
    struct Column {
        inline static const std::string                      // Originally:
            GATE_COUNTS = "Gate Counts",                     // g_col_total_counts
            INTERNAL_GATE_COUNTS = "__Internal_Gate_Counts", // g_col_gate_counts
            INTERNAL_GATE = "__Internal_Gate";               // g_col_gate
    };

  public:
    std::string getAnalysisName() const override { return "Agent Analysis"; }
    AnalysisResult run(Communicator *comm, PointMap &map, bool = false) override;
    AgentAnalysis() {}
};
