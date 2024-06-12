// Copyright (C) 2000-2010, University College London, Alasdair Turner
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2017-2024, Petros Koutsolampros

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
