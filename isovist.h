// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/bsptree.h"

#include <set>

// this is very much like sparksieve:

struct IsoSeg {
    mutable bool tagdelete;
    double startangle;
    double endangle;
    Point2f startpoint;
    Point2f endpoint;
    size_t index;
    int8_t quadrant;
    int tag;
    IsoSeg(double start = 0.0, double end = 0.0, size_t i = 0, int8_t q = 0, int t = -1)
        : tagdelete(false), startangle(start), endangle(end), index(i), quadrant(q), tag(t) {}
    IsoSeg(double start, double end, const Point2f &pstart, Point2f &pend, size_t i, int t = -1)
        : tagdelete(false), startangle(start), endangle(end), startpoint(pstart), endpoint(pend),
          index(i), quadrant(0), tag(t) {}
    friend bool operator==(const IsoSeg &b1, const IsoSeg &b2);
    friend bool operator>(const IsoSeg &b1, const IsoSeg &b2);
    friend bool operator<(const IsoSeg &b1, const IsoSeg &b2);
};
inline bool operator==(const IsoSeg &b1, const IsoSeg &b2) {
    return (b1.startangle == b2.startangle && b1.endangle == b2.endangle);
}

inline bool operator>(const IsoSeg &b1, const IsoSeg &b2) {
    return b1.startangle != b2.startangle ? b1.startangle > b2.startangle
           : b1.endangle != b2.endangle   ? b1.endangle > b2.endangle
                                          : b1.index > b2.index;
}
inline bool operator<(const IsoSeg &b1, const IsoSeg &b2) {
    return b1.startangle != b2.startangle ? b1.startangle < b2.startangle
           : b1.endangle != b2.endangle   ? b1.endangle < b2.endangle
                                          : b1.index < b2.index;
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
    Isovist() { ; }
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
