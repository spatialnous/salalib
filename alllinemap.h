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

#include "salalib/axialpolygons.h"
#include "salalib/shapegraph.h"
#include "salalib/spacepixfile.h"

class AllLineMap : public ShapeGraph {
  public:
    void generate(Communicator *comm, std::vector<SpacePixelFile> &drawingLayers,
                  const Point2f &seed);
    void generate(Communicator *comm, std::vector<Line> &lines, QtRegion &region,
                  const Point2f &seed);
    AllLineMap(const std::string &name = "All-Line Map") : ShapeGraph(name, ShapeMap::ALLLINEMAP) {}
    AxialPolygons m_polygons;
    std::vector<PolyConnector> m_poly_connections;
    std::vector<RadialLine> m_radial_lines;
    void setKeyVertexCount(int keyvertexcount) { m_keyvertexcount = keyvertexcount; }
    std::tuple<std::unique_ptr<ShapeGraph>, std::unique_ptr<ShapeGraph>>
    extractFewestLineMaps(Communicator *comm);
    void makeDivisions(const std::vector<PolyConnector> &polyconnections,
                       const std::vector<RadialLine> &radiallines,
                       std::map<RadialKey, std::set<int>> &radialdivisions,
                       std::map<int, std::set<int>> &axialdividers, Communicator *comm);
};
