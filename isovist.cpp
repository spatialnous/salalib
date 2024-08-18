// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "isovist.h"

#include <cmath>
#include <float.h>
#include <time.h>

///////////////////////////////////////////////////////////////////////

// Interestingly, apparently ray tracing is faster using voxel techniques than
// octrees etc: Akira Fujimotot, Takayuki Tanaka, and Kansei Iwata. ARTS:
// Accelerated ray-tracing system.  IEEE Computer Graphics and Applications,
// 6(4):16--26, April 1986

// This uses BSP trees, and appears to be superfast once the tree is built

void Isovist::makeit(BSPNode *root, const Point2f &p, const QtRegion &region, double startangle,
                     double endangle) {
    // region is used to give an idea of scale, so isovists can be linked when
    // there is floating point error
    double tolerance = std::max(region.width(), region.height()) * 1e-9;

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
        if (curr != m_blocks.begin() && !approxeq(prev->endpoint, curr->startpoint, tolerance)) {
            m_poly.push_back(curr->startpoint);
            // record perimeter information:
            double occluded = dist(prev->endpoint, curr->startpoint);
            m_perimeter += occluded;
            m_occludedPerimeter += occluded;
            // record the near *point* for use in agent analysis
            // (as the point will not move between isovists, so can record *which*
            // occlusion this is, and spot novel ones)
            if (dist(prev->endpoint, m_centre) < dist(curr->startpoint, m_centre)) {
                m_occlusionPoints.push_back(PointDist(prev->endpoint, occluded));
            } else {
                m_occlusionPoints.push_back(PointDist(curr->startpoint, occluded));
            }
        }
        m_poly.push_back(curr->endpoint);
        m_perimeter += dist(curr->startpoint, curr->endpoint);
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
        !approxeq(m_blocks.rbegin()->endpoint, m_blocks.begin()->startpoint, tolerance)) {
        m_poly.push_back(m_blocks.begin()->startpoint);
        // record perimeter information:
        double occluded = dist(m_blocks.rbegin()->endpoint, m_blocks.begin()->startpoint);
        m_perimeter += occluded;
        m_occludedPerimeter += occluded;
        // record the near *point* for use in agent analysis
        // (as the point will not move between isovists, so can record *which*
        // occlusion this is, and spot novel ones)
        if (occluded > 1.5) {
            if (dist(m_blocks.rbegin()->endpoint, m_centre) <
                dist(m_blocks.begin()->startpoint, m_centre)) {
                m_occlusionPoints.push_back(PointDist(m_blocks.rbegin()->endpoint, occluded));
            } else {
                m_occlusionPoints.push_back(PointDist(m_blocks.begin()->startpoint, occluded));
            }
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
        Line l(block.startpoint, block.endpoint);
        if (mintag == -1 || dist(p, l) < mindist) {
            mindist = dist(p, l);
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

void Isovist::drawnode(const Line &li, int tag) {
    Point2f p1 = li.start() - m_centre;
    p1.normalise();
    double angle1 = p1.angle();
    Point2f p2 = li.end() - m_centre;
    p2.normalise();
    double angle2 = p2.angle();
    if (angle2 > angle1) {
        if (angle2 - angle1 >= M_PI) {
            // 0 to angle1 and angle2 to 2 pi
            addBlock(li, tag, 0.0, angle1);
            addBlock(li, tag, angle2, 2.0 * M_PI);
        } else {
            // angle1 to angle2
            addBlock(li, tag, angle1, angle2);
        }
    } else {
        if (angle1 - angle2 >= M_PI) {
            // 0 to angle2 and angle1 to 2 pi
            addBlock(li, tag, 0.0, angle2);
            addBlock(li, tag, angle1, 2.0 * M_PI);
        } else {
            // angle2 to angle1
            addBlock(li, tag, angle2, angle1);
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

void Isovist::addBlock(const Line &li, int tag, double startangle, double endangle) {
    auto gap = m_gaps.begin();
    bool finished = false;

    while (!finished) {
        while (gap != m_gaps.end() && gap->endangle < startangle) {
            gap++;
        }
        if (gap != m_gaps.end() && gap->startangle < endangle + 1e-9) {
            double a, b;
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
            Point2f pa = intersection_point(li, Line(m_centre, m_centre + pointfromangle(a)));
            Point2f pb = intersection_point(li, Line(m_centre, m_centre + pointfromangle(b)));
            m_blocks.insert(IsoSeg(a, b, pa, pb, tag));
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
        double dpoint = dist(m_centre, p1);
        double dline = dist(m_centre, Line(p1, p2));
        if (i != 0) {
            // This is not minimum radial -- it's the distance to the closest corner!
            if (dline < m_minRadial) {
                m_minRadial = dline;
                dist(m_centre, Line(p1, p2));
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
