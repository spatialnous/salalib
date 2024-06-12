// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/isegment.h"

class SegmentTulip : ISegment {
  private:
    std::set<double> m_radius_set;
    bool m_sel_only;
    int m_tulip_bins;
    int m_weighted_measure_col;
    int m_weighted_measure_col2;
    int m_routeweight_col;
    int m_radius_type;
    bool m_choice;
    bool m_interactive;

  public:
    std::string getAnalysisName() const override { return "Tulip Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool) override;
    SegmentTulip(std::set<double> radius_set, bool sel_only, int tulip_bins,
                 int weighted_measure_col, int radius_type, bool choice, bool interactive = false,
                 int weighted_measure_col2 = -1, int routeweight_col = -1)
        : m_radius_set(radius_set), m_sel_only(sel_only), m_tulip_bins(tulip_bins),
          m_weighted_measure_col(weighted_measure_col),
          m_weighted_measure_col2(weighted_measure_col2), m_routeweight_col(routeweight_col),
          m_radius_type(radius_type), m_choice(choice), m_interactive(interactive) {}
};
