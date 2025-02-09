// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "isovist.h"
#include "salalib/tolerances.h"

#include <cmath>
#include <float.h>
#include <iostream>
#include <time.h>

///////////////////////////////////////////////////////////////////////

// Interestingly, apparently ray tracing is faster using voxel techniques than
// octrees etc: Akira Fujimotot, Takayuki Tanaka, and Kansei Iwata. ARTS:
// Accelerated ray-tracing system.  IEEE Computer Graphics and Applications,
// 6(4):16--26, April 1986

// This uses BSP trees, and appears to be superfast once the tree is built

void Isovist::makeit(BSPNode *root, const Point2f &p, const Region4f &region, double startangle,
                     double endangle, bool forceClosePoly) {
    const double ISOVIST_POINT_MATCH_TOLERANCE = 1e-9;
    // region is used to give an idea of scale, so isovists can be linked when
    // there is floating point error
    double tolerance = std::max(region.width(), region.height()) * ISOVIST_POINT_MATCH_TOLERANCE;

    m_centre = p;
    m_blocks.clear();
    m_gaps.clear();

    // still doesn't work when need centre point, but this will work for 180
    // degree isovists
    bool complete = false;

    if (startangle == endangle || (startangle == 0.0 && endangle == 2.0 * M_PI)) {
        startangle = 0.0;
        endangle = 2.0 * M_PI;
        complete = true;
    }

    bool parity = false;

    if (startangle > endangle) {
        m_gaps.insert(IsoSeg(0.0, endangle));
        m_gaps.insert(IsoSeg(startangle, 2.0 * M_PI));
    } else {
        parity = true;
        m_gaps.insert(IsoSeg(startangle, endangle));
    }

    make(root);

    // now it is constructed, make the isovist polygon:
    m_poly.clear();
    m_perimeter = 0.0;
    m_occludedPerimeter = 0.0;
    m_occlusionPoints.clear();

    bool markedcentre = false;
    auto prev = m_blocks.begin();
    auto curr = m_blocks.begin();
    for (; curr != m_blocks.end(); ++curr) {
        if (!complete && !markedcentre && !parity && curr->startangle == startangle) {
            // centre
            m_poly.push_back(p);
            // perimeter! occlusivity!
            markedcentre = true;
        }
        if (curr != m_blocks.begin() && !prev->endpoint.approxeq(curr->startpoint, tolerance)) {
            m_poly.push_back(curr->startpoint);
            // record perimeter information:
            double occluded = prev->endpoint.dist(curr->startpoint);
            m_perimeter += occluded;
            m_occludedPerimeter += occluded;
            // record the near *point* for use in agent analysis
            // (as the point will not move between isovists, so can record *which*
            // occlusion this is, and spot novel ones)
            if (prev->endpoint.dist(m_centre) < curr->startpoint.dist(m_centre)) {
                m_occlusionPoints.push_back(PointDist(prev->endpoint, occluded));
            } else {
                m_occlusionPoints.push_back(PointDist(curr->startpoint, occluded));
            }
        }
        m_poly.push_back(curr->endpoint);
        m_perimeter += curr->startpoint.dist(curr->endpoint);
        prev = curr;
    }
    // for some reason to do with ordering, if parity is true, the centre point
    // must be last not first
    if (!complete && parity) {
        // centre
        m_poly.push_back(p);
        // perimeter! occlusivity!
    }
    if (m_blocks.size() &&
        !m_blocks.rbegin()->endpoint.approxeq(m_blocks.begin()->startpoint, tolerance)) {
        m_poly.push_back(m_blocks.begin()->startpoint);
        // record perimeter information:
        double occluded = m_blocks.rbegin()->endpoint.dist(m_blocks.begin()->startpoint);
        m_perimeter += occluded;
        m_occludedPerimeter += occluded;
        // record the near *point* for use in agent analysis
        // (as the point will not move between isovists, so can record *which*
        // occlusion this is, and spot novel ones)
        if (occluded > 1.5) {
            if (m_blocks.rbegin()->endpoint.dist(m_centre) <
                m_blocks.begin()->startpoint.dist(m_centre)) {
                m_occlusionPoints.push_back(PointDist(m_blocks.rbegin()->endpoint, occluded));
            } else {
                m_occlusionPoints.push_back(PointDist(m_blocks.begin()->startpoint, occluded));
            }
        }
    }
    if (forceClosePoly) {
        // if the polygon is not closed force it to close
        if (!m_poly.front().approxeq(m_poly.back(), 0.00001)) {
            std::cout << "x1: " << m_poly.front().x << std::endl                         //
                      << "y1: " << m_poly.front().y << std::endl                         //
                      << "x2: " << m_poly.back().x << std::endl                          //
                      << "y2: " << m_poly.back().y << std::endl                          //
                      << "x3: " << (m_poly.front().x - m_poly.back().x) << std::endl     //
                      << "y3: " << (m_poly.front().y - m_poly.back().y) << std::endl     //
                      << "x4: " << fabs(m_poly.front().x - m_poly.back().x) << std::endl //
                      << "y4: " << fabs(m_poly.front().y - m_poly.back().y) << std::endl //
                      << std::endl;
            m_poly.push_back(m_poly.front());
        }
    }
}

int Isovist::getClosestLine(BSPNode *root, const Point2f &p) {
    m_centre = p;
    m_blocks.clear();
    m_gaps.clear();

    m_gaps.insert(IsoSeg(0.0, 2.0 * M_PI));

    make(root);

    int mintag = -1;
    double mindist = 0.0;

    for (auto &block : m_blocks) {
        Line4f l(block.startpoint, block.endpoint);
        if (mintag == -1 || l.dist(p) < mindist) {
            mindist = l.dist(p);
            mintag = block.tag;
        }
    }

    return mintag;
}

void Isovist::make(BSPNode *here) {
    if (m_gaps.size()) {
        int which = here->classify(m_centre);
        if (which == BSPNode::BSPLEFT) {
            if (here->left.get())
                make(here->left.get());
            drawnode(here->getLine(), here->getTag());
            if (here->right)
                make(here->right.get());
        } else {
            if (here->right.get())
                make(here->right.get());
            drawnode(here->getLine(), here->getTag());
            if (here->left)
                make(here->left.get());
        }
    }
}

void Isovist::drawnode(const Line4f &li, int tag) {
    long double pipi = 2.0 * M_PI;

    Point2f p1 = li.start() - m_centre;
    p1.normalise();
    Point2f p2 = li.end() - m_centre;
    p2.normalise();

    auto p1x = static_cast<long double>(p1.x);
    auto p2x = static_cast<long double>(p2.x);

    long double acosl1x = acosl(p1x);
    long double acosl2x = acosl(p2x);

    long double angle1 = (p1.y < 0) ? (pipi - acosl1x) : acosl1x;
    long double angle2 = (p2.y < 0) ? (pipi - acosl2x) : acosl2x;

    if (angle2 > angle1) {
        if (angle2 - angle1 >= M_PI) {
            // 0 to angle1 and angle2 to 2 pi
            addBlock(li, tag, 0.0, static_cast<double>(angle1));
            addBlock(li, tag, static_cast<double>(angle2), static_cast<double>(pipi));
        } else {
            // angle1 to angle2
            addBlock(li, tag, static_cast<double>(angle1), static_cast<double>(angle2));
        }
    } else {
        if (angle1 - angle2 >= M_PI) {
            // 0 to angle2 and angle1 to 2 pi
            addBlock(li, tag, 0.0, static_cast<double>(angle2));
            addBlock(li, tag, static_cast<double>(angle1), static_cast<double>(pipi));
        } else {
            // angle2 to angle1
            addBlock(li, tag, static_cast<double>(angle2), static_cast<double>(angle1));
        }
    }
    //
    for (auto it = m_gaps.begin(); it != m_gaps.end();) {
        if (it->tagdelete) {
            it = m_gaps.erase(it);
        } else {
            ++it;
        }
    }
}

union myUnion {
    double dValue;
    uint64_t iValue;
};

void Isovist::addBlock(const Line4f &li, int tag, double startangle, double endangle) {
    auto gap = m_gaps.begin();
    bool finished = false;

    while (!finished) {
        while (gap != m_gaps.end() && gap->endangle < startangle) {
            gap++;
        }
        if (gap != m_gaps.end() && gap->startangle < endangle + 1e-9) {
            long double a, b;
            if (gap->startangle > startangle - 1e-9) {
                a = gap->startangle;
                if (gap->endangle < endangle + 1e-9) {
                    b = gap->endangle;
                    gap->tagdelete = true;
                } else {
                    b = endangle;
                    IsoSeg isoseg = *gap;
                    isoseg.startangle = endangle;
                    auto hint = gap;
                    hint++;
                    m_gaps.erase(gap);
                    gap = m_gaps.insert(hint, isoseg);
                }
            } else {
                a = startangle;
                if (gap->endangle < endangle + 1e-9) {
                    b = gap->endangle;
                    IsoSeg isoseg = *gap;
                    isoseg.endangle = startangle;
                    auto hint = gap;
                    hint++;
                    m_gaps.erase(gap);
                    gap = m_gaps.insert(hint, isoseg);
                } else {
                    b = endangle;
                    m_gaps.insert(IsoSeg(endangle, gap->endangle, gap->quadrant));
                    IsoSeg isoseg = *gap;
                    isoseg.endangle = startangle;
                    auto hint = gap;
                    hint++;
                    m_gaps.erase(gap);
                    gap = m_gaps.insert(hint, isoseg);
                    gap++; // advance past gap just added
                }
            }

            // using cos and sin directly to achieve double binary parity between macos and linux
            Point2f pa = li.intersection_point(
                Line4f(m_centre, m_centre + Point2f(static_cast<double>(cosl(a)),
                                                    static_cast<double>(sinl(a)))));
            Point2f pb = li.intersection_point(
                Line4f(m_centre, m_centre + Point2f(static_cast<double>(cosl(b)),
                                                    static_cast<double>(sinl(b)))));

            Point2f p1 = li.start() - m_centre;
            p1.normalise();
            Point2f p2 = li.end() - m_centre;
            p2.normalise();

            myUnion lisx;
            lisx.dValue = p1.x;
            myUnion lisy;
            lisy.dValue = p1.y;
            myUnion liex;
            liex.dValue = p2.x;
            myUnion liey;
            liey.dValue = p2.y;

            long double pipi = 2.0 * M_PI;

            auto p1x = static_cast<long double>(p1.x);
            auto p2x = static_cast<long double>(p1.x);

            long double acosl1x = acosl(p1x);
            long double acosl2x = acosl(p2x);

            long double angle1 = (p1.y < 0) ? (pipi - acosl1x) : acosl1x;
            long double angle2 = (p2.y < 0) ? (pipi - acosl2x) : acosl2x;

            myUnion a1;
            a1.dValue = static_cast<double>(angle1);
            myUnion a2;
            a2.dValue = static_cast<double>(angle2);

            myUnion au;
            au.dValue = static_cast<double>(a);
            myUnion pax;
            pax.dValue = pa.x;
            myUnion pay;
            pay.dValue = pa.y;
            myUnion av;
            av.dValue = static_cast<double>(cosl(a));

            myUnion bu;
            bu.dValue = static_cast<double>(b);
            myUnion pbx;
            pbx.dValue = pb.x;
            myUnion pby;
            pby.dValue = pb.y;
            myUnion bv;
            bv.dValue = static_cast<double>(cosl(b));
            //            bv.dValue = sin(b);
            //            bv.dValue = pointfromangle(b).x;

            std::cout << "li:\t" << lisx.dValue << " " << lisx.iValue //
                      << "\n\t" << lisy.dValue << " " << lisy.iValue  //
                      << "\n\t" << liex.dValue << " " << liex.iValue  //
                      << "\n\t" << liey.dValue << " " << liey.iValue << std::endl
                      << std::endl;
            std::cout << "a1:\t" << a1.dValue << " " << a1.iValue //
                      << std::endl;
            std::cout << "a2:\t" << a2.dValue << " " << a2.iValue //
                      << std::endl;
            std::cout << "a:\t" << a << " " << au.iValue         //
                      << "\n\t" << av.dValue << " " << av.iValue //
                      << "\n\t" << pa.x << " " << pax.iValue     //
                      << "\n\t" << pa.y << " " << pay.iValue << std::endl
                      << std::endl;
            std::cout << "b:\t" << b << " " << bu.iValue         //
                      << "\n\t" << bv.dValue << " " << bv.iValue //
                      << "\n\t" << pb.x << " " << pbx.iValue     //
                      << "\n\t" << pb.y << " " << pby.iValue << std::endl
                      << std::endl;
            std::cout << std::endl;

            m_blocks.insert(IsoSeg(static_cast<double>(a), static_cast<double>(b), pa, pb,
                                   m_blocks.size(), tag));
        } else {
            finished = true;
        }
        if (gap == m_gaps.end())
            break;
        gap++;
    }
}

std::pair<Point2f, double> Isovist::getCentroidArea() {
    // the area / centre of gravity calculation is a duplicate of the SalaPolygon
    // version, included here for general information about the isovist
    double area = 0.0;
    Point2f centroid = Point2f(0, 0);
    for (size_t i = 0; i < m_poly.size(); i++) {
        Point2f &p1 = m_poly.at(i);
        Point2f &p2 = m_poly.at((i + 1) % m_poly.size());
        double aI = (p1.x * p2.y - p2.x * p1.y) / 2.0;
        area += aI;
        aI /= 6.0;
        centroid.x += (p1.x + p2.x) * aI;
        centroid.y += (p1.y + p2.y) * aI;
        double dpoint = m_centre.dist(p1);
        double dline = Line4f(p1, p2).dist(m_centre);
        if (i != 0) {
            // This is not minimum radial -- it's the distance to the closest corner!
            if (dline < m_minRadial) {
                m_minRadial = dline;
                Line4f(p1, p2).dist(m_centre);
            }
            if (dpoint > m_maxRadial) {
                m_maxRadial = dpoint;
            }
        } else {
            m_maxRadial = dpoint;
            m_minRadial = dline;
        }
    }
    centroid.scale(2.0 / fabs(area));
    return std::make_pair(centroid, area);
}

std::pair<double, double> Isovist::getDriftData() {
    auto [centroid, area] = getCentroidArea();
    Point2f driftvec = centroid - m_centre;
    double driftmag = driftvec.length();
    driftvec.normalise();
    double driftang = driftvec.angle();
    return std::make_pair(driftmag, driftang);
}
