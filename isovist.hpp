// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/bsptree.hpp"

#include <set>

// this is very much like sparksieve:

struct IsoSeg {
    mutable bool tagdelete;
    int8_t quadrant;

  private:
    [[maybe_unused]] unsigned _padding0 : 2 * 8;

  public:
    int tag;
    double startangle;
    double endangle;
    Point2f startpoint;
    Point2f endpoint;
    IsoSeg(double start = 0.0, double end = 0.0, int8_t q = 0, int t = -1)
        : tagdelete(false), quadrant(q), _padding0(0), tag(t), startangle(start), endangle(end),
          startpoint(), endpoint() {}
    IsoSeg(double start, double end, const Point2f &pstart, Point2f &pend, int t = -1)
        : tagdelete(false), quadrant(0), _padding0(0), tag(t), startangle(start), endangle(end),
          startpoint(pstart), endpoint(pend) {}
    friend bool operator==(const IsoSeg &b1, const IsoSeg &b2);
    friend bool operator>(const IsoSeg &b1, const IsoSeg &b2);
    friend bool operator<(const IsoSeg &b1, const IsoSeg &b2);
};
inline bool operator==(const IsoSeg &b1, const IsoSeg &b2) {
    return (b1.startangle == b2.startangle && b1.endangle == b2.endangle);
}

inline bool operator>(const IsoSeg &b1, const IsoSeg &b2) {
    return b1.startangle == b2.startangle ? b1.endangle > b2.endangle
                                          : b1.startangle > b2.startangle;
}
inline bool operator<(const IsoSeg &b1, const IsoSeg &b2) {
    return b1.startangle == b2.startangle ? b1.endangle < b2.endangle
                                          : b1.startangle < b2.startangle;
}

class AttributeTable;

struct PointDist {
    Point2f point;
    double dist;
    PointDist(const Point2f &p = Point2f(), double d = 0.0) : point(p), dist(d) {}
};

class Isovist {
  protected:
    Point2f m_centre;
    std::set<IsoSeg> m_blocks;
    std::set<IsoSeg> m_gaps;
    std::vector<Point2f> m_poly;
    std::vector<PointDist> m_occlusionPoints;
    double m_perimeter;
    double m_occludedPerimeter;
    double m_maxRadial;
    double m_minRadial;

  public:
    Isovist()
        : m_centre(), m_blocks(), m_gaps(), m_poly(), m_occlusionPoints(), m_perimeter(),
          m_occludedPerimeter(), m_maxRadial(), m_minRadial() {}
    const std::vector<Point2f> &getPolygon() const { return m_poly; }
    const std::vector<PointDist> &getOcclusionPoints() const { return m_occlusionPoints; }
    const Point2f &getCentre() const { return m_centre; }
    //
    void makeit(BSPNode *root, const Point2f &p, const Region4f &region, double startangle = 0.0,
                double endangle = 0.0, bool forceClosePoly = false);
    void make(BSPNode *here);
    void drawnode(const Line4f &li, int tag);
    void addBlock(const Line4f &li, int tag, double startangle, double endangle);
    std::pair<Point2f, double> getCentroidArea();
    std::pair<double, double> getDriftData();
    double getPerimeter() { return m_perimeter; }
    double getMinRadial() { return m_minRadial; }
    double getMaxRadial() { return m_maxRadial; }
    double getOccludedPerimeter() { return m_occludedPerimeter; }
    //
    int getClosestLine(BSPNode *root, const Point2f &p);
};
