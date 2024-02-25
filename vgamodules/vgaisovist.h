// sala - a component of the depthmapX - spatial network analysis platform
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

#include "salalib/ivga.h"
#include "salalib/pointdata.h"

class VGAIsovist : IVGA {
    const std::vector<SalaShape> &m_boundaryShapes;
  public:
    VGAIsovist(const std::vector<SalaShape> &boundaryShapes) :
    m_boundaryShapes(boundaryShapes) {}
    std::string getAnalysisName() const override { return "Isovist Analysis"; }
    AnalysisResult run(Communicator *comm,
                       PointMap &map,
                       bool simple_version) override;
private:
    std::vector<std::pair<std::string, int>> createAttributes(AttributeTable &table, bool simple_version);
    std::set<std::string> setData(Isovist &isovist,
                                  AttributeRow &row,
                                  std::vector<std::pair<std::string, int>> cols,
                                  bool simple_version);
    BSPNode makeBSPtree(Communicator *communicator,
                        const std::vector<SalaShape> &boundaryShapes);
};
