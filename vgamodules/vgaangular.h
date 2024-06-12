// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ivga.h"
#include "salalib/pointdata.h"

class VGAAngular : IVGA {
  private:
    double m_radius;
    bool m_gates_only;

  public:
    std::string getAnalysisName() const override { return "Angular Analysis"; }
    AnalysisResult run(Communicator *, PointMap &map, bool) override;
    VGAAngular(double radius, bool gates_only) : m_radius(radius), m_gates_only(gates_only) {}
};
