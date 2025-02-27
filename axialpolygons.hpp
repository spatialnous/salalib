// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "connector.hpp"
#include "shapegraph.hpp"
#include "spacepix.hpp"

#include "genlib/simplematrix.hpp"

struct AxialVertexKey {
    int refKey;
    short refA;
    short refB;
    AxialVertexKey(int ref = -1, short a = -1, short b = -1) : refKey(ref), refA(a), refB(b) {}
    friend bool operator==(const AxialVertexKey &a, const AxialVertexKey &b);
    friend bool operator!=(const AxialVertexKey &a, const AxialVertexKey &b);
    friend bool operator>(const AxialVertexKey &a, const AxialVertexKey &b);
    friend bool operator<(const AxialVertexKey &a, const AxialVertexKey &b);
};
inline bool operator==(const AxialVertexKey &a, const AxialVertexKey &b) {
    return (a.refKey == b.refKey && a.refA == b.refA && a.refB == b.refB);
}
inline bool operator!=(const AxialVertexKey &a, const AxialVertexKey &b) {
    return (a.refKey != b.refKey || a.refA != b.refA || a.refB != b.refB);
}
inline bool operator>(const AxialVertexKey &a, const AxialVertexKey &b) {
    return (a.refKey > b.refKey ||
            (a.refKey == b.refKey && (a.refA > b.refA || (a.refA == b.refA && a.refB > b.refB))));
}
inline bool operator<(const AxialVertexKey &a, const AxialVertexKey &b) {
    return (a.refKey < b.refKey ||
            (a.refKey == b.refKey && (a.refA < b.refA || (a.refA == b.refA && a.refB < b.refB))));
}

const AxialVertexKey NoVertex(-1, -1, -1);

struct AxialVertex : public AxialVertexKey {
    Point2f point;
    Point2f openspace;
    Point2f a;
    Point2f b;
    bool clockwise;
    bool convex;
    bool initialised;
    bool axial;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    AxialVertex(const AxialVertexKey &vertexKey = NoVertex, const Point2f &p = Point2f(),
                const Point2f &opsp = Point2f())
        : AxialVertexKey(vertexKey), point(p), openspace(opsp), a(), b(), clockwise(false),
          convex(false), initialised(false), axial(false), _padding0(0) {}
};

struct RadialKey {
    AxialVertexKey vertex;
    float ang;
    bool segend;
    // padding the remaining three bytes behind the bool - don't use int : 24 as this will grab the
    // next 4 byte block
    int8_t pad1 : 8;
    short pad2 : 16;

    RadialKey(const AxialVertexKey &v = NoVertex, float a = -1.0f, bool se = false)
        : vertex(v), ang(a), segend(se), pad1(0), pad2(0) {}
    RadialKey(const RadialKey &rk)
        : vertex(rk.vertex), ang(rk.ang), segend(rk.segend), pad1(0), pad2(0) {}
    RadialKey &operator=(const RadialKey &) = default;
};
inline bool operator<(const RadialKey &a, const RadialKey &b) {
    return a.vertex < b.vertex ||
           (a.vertex == b.vertex && (a.ang < b.ang || (a.ang == b.ang && a.segend < b.segend)));
}
inline bool operator>(const RadialKey &a, const RadialKey &b) {
    return a.vertex > b.vertex ||
           (a.vertex == b.vertex && (a.ang > b.ang || (a.ang == b.ang && a.segend > b.segend)));
}
inline bool operator==(const RadialKey &a, const RadialKey &b) {
    return a.vertex == b.vertex && a.ang == b.ang && a.segend == b.segend;
}

struct RadialLine : public RadialKey {
    Point2f openspace;
    Point2f keyvertex;
    Point2f nextvertex;
    RadialLine(const RadialKey &rk = RadialKey())
        : RadialKey(rk), openspace(), keyvertex(), nextvertex() {}
    RadialLine(const AxialVertexKey &v, bool se, const Point2f &o, const Point2f &k,
               const Point2f &n)
        : openspace(o), keyvertex(k), nextvertex(n) {
        vertex = v;
        ang = static_cast<float>(o.angle(k, n));
        segend = se;
    }
    RadialLine(const RadialLine &rl)
        : RadialKey(rl), openspace(rl.openspace), keyvertex(rl.keyvertex),
          nextvertex(rl.nextvertex) {}
    bool cuts(const Line4f &l) const;
    RadialLine &operator=(const RadialLine &) = default;
};

struct RadialSegment {
    std::set<int> indices;
    RadialKey radialB;

    RadialSegment(RadialKey &rb) : indices(), radialB(rb) {}
    RadialSegment(const RadialKey &rb) : indices(), radialB(rb) {}
    RadialSegment() : indices(), radialB() {}
};

struct PolyConnector {
    Line4f line;
    RadialKey key;
    PolyConnector(const Line4f &l = Line4f(), const RadialKey &k = RadialKey()) : line(l), key(k) {}
};

class AxialPolygons : public SpacePixel {
    friend class ShapeGraphs;

  protected:
    std::vector<int> m_vertexPolys;
    depthmapX::ColumnMatrix<std::vector<int>> m_pixelPolys;

  public:
    AxialPolygons() : m_vertexPolys(), m_pixelPolys(0, 0), handledList(), vertexPossibles() {}
    std::set<AxialVertex> handledList;
    std::map<Point2f, std::vector<Point2f>> vertexPossibles;
    void clear();
    void init(std::vector<Line4f> &lines, const Region4f &region);
    void makeVertexPossibles(const std::vector<Line4f> &lines,
                             const std::vector<Connector> &connectionset);
    void makePixelPolys();
    //
    AxialVertex makeVertex(const AxialVertexKey &vertexkey, const Point2f &openspace);
    // find a polygon corner visible from seed:
    AxialVertexKey seedVertex(const Point2f &seed);
    // make axial lines from corner vertices, visible from openspace
    void makeAxialLines(std::set<AxialVertex> &openvertices, std::vector<Line4f> &lines,
                        KeyVertices &keyvertices, std::vector<PolyConnector> &polyConnections,
                        std::vector<RadialLine> &radialLines);
    // extra: make all the polygons possible from the set of m_vertex_possibles
    void makePolygons(std::vector<std::vector<Point2f>> &polygons);
};
