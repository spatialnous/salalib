#pragma once

#include "salalib/axialpolygons.h"
#include "salalib/shapegraph.h"

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
