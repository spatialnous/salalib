// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/iaxial.h"

class AxialIntegration : IAxial {
  private:
    std::set<double> m_radius_set;
    int m_weighted_measure_col;
    bool m_choice;
    bool m_fulloutput;

  public:
    std::string getAnalysisName() const override { return "Angular Analysis"; }

    AnalysisResult run(Communicator *, ShapeGraph &map, bool) override;
    AxialIntegration(std::set<double> radius_set, int weighted_measure_col, bool choice,
                     bool fulloutput)
        : m_radius_set(radius_set), m_weighted_measure_col(weighted_measure_col), m_choice(choice),
          m_fulloutput(fulloutput) {}
};
