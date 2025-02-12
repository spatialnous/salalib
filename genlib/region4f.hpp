// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "edgeu.hpp"
#include "point2f.hpp"

class Region4f {
  public:
    Point2f bottomLeft;
    Point2f topRight;
    Region4f(const Point2f &bl = Point2f(), const Point2f &tr = Point2f())
        : bottomLeft(bl), topRight(tr) {}
    Region4f(const Region4f &r) : bottomLeft(r.bottomLeft), topRight(r.topRight) {}
    Region4f &operator=(const Region4f &r) {
        bottomLeft = r.bottomLeft;
        topRight = r.topRight;
        return *this;
    }
    bool operator==(const Region4f &other) const {
        return bottomLeft == other.bottomLeft && topRight == other.topRight;
    }

    double height() const { return fabs(topRight.y - bottomLeft.y); }
    double width() const
    // The assumption that topRight.x is always > bottomLeft.x is not always true.
    // Returning a negative value here causes an infinite loop at axialmap.cpp line 3106
    // after overlapdist is assigned a negative value at axialmap.cpp line 3084.
    // height() above could also be changed for this reason, but this is a band-aid
    // fix for the real problem, which is why the topRight > bottomLeft assumption
    // is assumed to be 100% valid but is, in some instances, not valid.
    // { return topRight.x - bottomLeft.x; }
    {
        return fabs(topRight.x - bottomLeft.x);
    }
    double area() const { return height() * width(); }
    void normalScale(const Region4f &r) {
        topRight.normalScale(r.bottomLeft, r.width(), r.height());
        bottomLeft.normalScale(r.bottomLeft, r.width(), r.height());
    }
    void denormalScale(const Region4f &r) {
        topRight.denormalScale(r.bottomLeft, r.width(), r.height());
        bottomLeft.denormalScale(r.bottomLeft, r.width(), r.height());
    }
    void scale(const Point2f &scalevec) {
        topRight.scale(scalevec);
        bottomLeft.scale(scalevec);
    }
    void offset(const Point2f &offset) {
        topRight += offset;
        bottomLeft += offset;
    }
    Point2f getCentre() const {
        return Point2f((bottomLeft.x + topRight.x) / 2.0, (bottomLeft.y + topRight.y) / 2.0);
    }

    bool contains(const Point2f &p) const {
        return (p.x > bottomLeft.x && p.x < topRight.x && p.y > bottomLeft.y && p.y < topRight.y);
    }
    bool contains_touch(const Point2f &p) const {
        return (p.x >= bottomLeft.x && p.x <= topRight.x && p.y >= bottomLeft.y &&
                p.y <= topRight.y);
    }
    void encompass(const Point2f &p) {
        if (p.x < bottomLeft.x)
            bottomLeft.x = p.x;
        if (p.x > topRight.x)
            topRight.x = p.x;
        if (p.y < bottomLeft.y)
            bottomLeft.y = p.y;
        if (p.y > topRight.y)
            topRight.y = p.y;
    }

    bool atZero() const { return bottomLeft.atZero() || topRight.atZero(); }

    Point2f getEdgeUPoint(const EdgeU &eu);
    EdgeU getCutEdgeU(const Point2f &inside, const Point2f &outside);

    bool intersects(const Region4f &b, double tolerance = 0.0) const;
    bool overlap_x(const Region4f &b, double tolerance = 0.0) const;
    bool overlap_y(const Region4f &b, double tolerance = 0.0) const;

    // set functions
    Region4f runion(const Region4f &b) const;

    void grow(const double scalar) {
        Point2f dim = topRight - bottomLeft;
        dim.scale(scalar - 1.0);
        topRight += dim;
        bottomLeft -= dim;
    }
};
