// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "connector.h"
#include "shapemap.h"

#include "genlib/comm.h"

#include <set>

struct AxialVertex;
struct AxialVertexKey;
struct RadialLine;
struct PolyConnector;

// used during angular analysis
struct AnalysisInfo {
    // lists used for multiple radius analysis
    bool leaf;
    bool choicecovered;
    SegmentRef previous;
    int depth;
    double choice;
    double weighted_choice;
    double weighted_choice2; // EFEF
    AnalysisInfo() {
        choicecovered = false;
        leaf = true;
        previous = SegmentRef();
        depth = 0;
        choice = 0.0;
        weighted_choice = 0.0;
        weighted_choice2 = 0.0;
    }
    void clearLine() {
        choicecovered = false;
        leaf = true;
        previous = SegmentRef();
        depth = 0; // choice values are cummulative and not cleared
    }
};

class MapInfoData;

typedef std::vector<std::set<int>> KeyVertices;

class ShapeGraph : public ShapeMap {
    friend class AxialMinimiser;
    friend class MapInfoData;

  protected:
    KeyVertices m_keyvertices; // but still need to return keyvertices here
    int m_keyvertexcount;

  protected:
  public:
    bool outputMifPolygons(std::ostream &miffile, std::ostream &midfile) const;
    void outputNet(std::ostream &netfile) const;

  public:
    ShapeGraph(const std::string &name = "<axial map>", int type = ShapeMap::AXIALMAP);

    ShapeGraph(ShapeGraph &&) = default;
    ShapeGraph &operator=(ShapeGraph &&) = default;

    void initialiseAttributesAxial();
    void makeConnections(const KeyVertices &keyvertices = KeyVertices());
    bool stepdepth(Communicator *comm = NULL);
    // lineset and connectionset are filled in by segment map
    void makeNewSegMap(Communicator *comm);
    void makeSegmentMap(std::vector<Line> &lines, std::vector<Connector> &connectors,
                        double stubremoval);
    void initialiseAttributesSegment();
    void makeSegmentConnections(std::vector<Connector> &connectionset);
    void pushAxialValues(ShapeGraph &axialmap);

    bool readShapeGraphData(std::istream &stream);
    virtual std::tuple<bool, bool, bool, int> read(std::istream &stream);
    bool writeShapeGraphData(std::ostream &stream) const;
    virtual bool write(std::ostream &stream,
                       std::tuple<bool, bool, int> displayData = std::make_tuple(true, false,
                                                                                 -1)) const;
    void writeAxialConnectionsAsDotGraph(std::ostream &stream);
    void writeAxialConnectionsAsPairsCSV(std::ostream &stream);
    void writeSegmentConnectionsAsPairsCSV(std::ostream &stream);
    void writeLinksUnlinksAsPairsCSV(std::ostream &stream, char delimiter = ',');
    void unlinkAtPoint(const Point2f &unlinkPoint);
    void unlinkFromShapeMap(const ShapeMap &shapemap);
};
