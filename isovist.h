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
    char quadrant;
    int tag;
    IsoSeg(double start = 0.0, double end = 0.0, char q = 0, int t = -1) {
        startangle = start;
        endangle = end;
        tagdelete = false;
        quadrant = q;
        tag = t;
    }
    IsoSeg(double start, double end, const Point2f &pstart, Point2f &pend, int t = -1) {
        startangle = start;
        endangle = end;
        startpoint = pstart;
        endpoint = pend;
        tagdelete = false;
        quadrant = 0;
        tag = t;
    }
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
    Point2f m_point;
    double m_dist;
    PointDist(const Point2f &p = Point2f(), double d = 0.0) {
        m_point = p;
        m_dist = d;
    }
};

class Isovist {
  protected:
    Point2f m_centre;
    std::set<IsoSeg> m_blocks;
    std::set<IsoSeg> m_gaps;
    std::vector<Point2f> m_poly;
    std::vector<PointDist> m_occlusion_points;
    double m_perimeter;
    double m_occluded_perimeter;
    double m_max_radial;
    double m_min_radial;

  public:
    Isovist() { ; }
    const std::vector<Point2f> &getPolygon() const { return m_poly; }
    const std::vector<PointDist> &getOcclusionPoints() const { return m_occlusion_points; }
    const Point2f &getCentre() const { return m_centre; }
    //
    void makeit(BSPNode *root, const Point2f &p, const QtRegion &region, double startangle = 0.0,
                double endangle = 0.0);
    void make(BSPNode *here);
    void drawnode(const Line &li, int tag);
    void addBlock(const Line &li, int tag, double startangle, double endangle);
    std::pair<Point2f, double> getCentroidArea();
    std::pair<double, double> getDriftData();
    double getPerimeter() { return m_perimeter; }
    double getMinRadial() { return m_min_radial; }
    double getMaxRadial() { return m_max_radial; }
    double getOccludedPerimeter() { return m_occluded_perimeter; }
    //
    int getClosestLine(BSPNode *root, const Point2f &p);
};
