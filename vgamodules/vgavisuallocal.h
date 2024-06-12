// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ivga.h"
#include "salalib/pointdata.h"

class VGAVisualLocal : IVGA {
  private:
    bool m_gates_only;

  public:
    std::string getAnalysisName() const override { return "Local Visibility Analysis"; }
    AnalysisResult run(Communicator *comm, PointMap &map, bool simple_version) override;
    VGAVisualLocal(bool gates_only) : m_gates_only(gates_only) {}
};
