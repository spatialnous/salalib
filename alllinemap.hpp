// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "axialpolygons.hpp"
#include "shapegraph.hpp"

namespace AllLine {
    struct MapData {
        size_t index = 0;
        AxialPolygons polygons;
        std::vector<PolyConnector> polyConnections;
        std::vector<RadialLine> radialLines;
        MapData() : index(0), polygons(), polyConnections(), radialLines() {}
    };
    MapData generate(Communicator *comm, ShapeGraph &map,
                     const std::vector<std::reference_wrapper<const ShapeMap>> &drawingLayers,
                     const Point2f &seed);
    void generate(Communicator *comm, ShapeGraph &map, MapData &mapData,
                  const std::vector<std::reference_wrapper<const ShapeMap>> &drawingLayers,
                  const Point2f &seed);
    MapData generate(Communicator *comm, ShapeGraph &map, std::vector<Line4f> &lines,
                     Region4f &region, const Point2f &seed);
    void generate(Communicator *comm, ShapeGraph &map, MapData &mapData, std::vector<Line4f> &lines,
                  Region4f &region, const Point2f &seed);
    ShapeGraph createAllLineMap(const std::string &name = "All-Line Map");
    std::tuple<ShapeGraph, ShapeGraph> extractFewestLineMaps(Communicator *comm, ShapeGraph &map,
                                                             MapData &mapData, unsigned int seed);
    void makeDivisions(ShapeGraph &map, const std::vector<PolyConnector> &polyconnections,
                       const std::vector<RadialLine> &radiallines,
                       std::map<RadialKey, std::set<int>> &radialdivisions,
                       std::map<int, std::set<int>> &axialdividers, Communicator *comm);
} // namespace AllLine
