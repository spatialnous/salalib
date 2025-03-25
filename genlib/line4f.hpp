// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "region4f.hpp"

#include <cstdint>

// Lines are stored left to right as regions,
// the parity tells us whether the region should be inverted
// top to bottom to get the line

class Line4f : public Region4f {
  protected:
    struct Bits {
        Bits() : parity(0), direction(0), xDummy(0), yDummy(0), zDummy(0) {}
        int8_t parity : 8;    // 1 ... positive, 0 ... negative
        int8_t direction : 8; // 1 ... positive, 0 ... negative

        // dummy variables as it seems to be necessary that the width of this struct is 8 bytes
        // and I don't want any uninitialised memory that gets written to file accidentally
        int8_t xDummy : 8;
        int8_t yDummy : 8;
        int zDummy : 32;
    };
    Bits m_bits;

  public:
    Line4f();
    Line4f(const Point2f &a, const Point2f &b);
    Line4f(const Region4f &r) : Region4f(r), m_bits() {
        m_bits.parity = 1;
        m_bits.direction = 1;
    }
    Line4f(const Line4f &l) : Region4f(l), m_bits(l.m_bits) {}
    Line4f &operator=(const Line4f &l) {
        this->Region4f::operator=(l);
        m_bits = l.m_bits;
        return *this;
    }
    bool operator==(const Line4f &other) const {
        // we could be comparing Region4f and then the bits, but this
        // is a line, and the two functions t_start and t_end seem
        // to provide all the necessary information for the test.
        return t_start() == other.t_start() && t_end() == other.t_end();
    }

    double dist(const Point2f &point) const;

    bool intersects(const Line4f &b, double tolerance = 0.0) const;
    bool intersects_no_touch(const Line4f &b, double tolerance = 0.0) const;
    int intersects_distinguish(const Line4f &b, double tolerance = 0.0) const;
    int intersects_b(const Line4f &b, double tolerance = 0.0) const;
    //
    // fills in the location along the axis where the intersection happens
    bool intersect_line(const Line4f &l, LineAxis axis, double &loc) const;
    double intersection_point(const Line4f &l, LineAxis axis, double tolerance = 0.0) const;
    // this converts a loc retrieved from intersect line or intersection point back into a point:
    Point2f point_on_line(double loc, LineAxis axis) const;
    // ...and a quick do it all in one go:
    Point2f intersection_point(const Line4f &b, double tolerance = 0.0) const;
    //
    bool crop(const Region4f &r);
    void ray(short dir, const Region4f &r);
    //
    friend double dot(const Line4f &a, const Line4f &b);
    //
    double ax() const { return bottomLeft.x; }
    double &ax() { return bottomLeft.x; }
    double bx() const { return topRight.x; }
    double &bx() { return topRight.x; }
    double ay() const { return m_bits.parity ? bottomLeft.y : topRight.y; }
    double &ay() { return m_bits.parity ? bottomLeft.y : topRight.y; }
    double by() const { return m_bits.parity ? topRight.y : bottomLeft.y; }
    double &by() { return m_bits.parity ? topRight.y : bottomLeft.y; }
    //
    const Point2f start() const {
        return Point2f(bottomLeft.x, (m_bits.parity ? bottomLeft.y : topRight.y));
    }
    const Point2f end() const {
        return Point2f(topRight.x, (m_bits.parity ? topRight.y : bottomLeft.y));
    }
    const Point2f midpoint() const { return Point2f((start() + end()) / 2); }
    //
    // helpful to have a user friendly indication of direction:
    bool rightward() const { return m_bits.direction == 1; }
    bool upward() const { return m_bits.direction == m_bits.parity; }
    //
    const Point2f t_start() const {
        return Point2f((rightward() ? bottomLeft.x : topRight.x),
                       (upward() ? bottomLeft.y : topRight.y));
    }
    const Point2f t_end() const {
        return Point2f((rightward() ? topRight.x : bottomLeft.x),
                       (upward() ? topRight.y : bottomLeft.y));
    }
    //
    short sign() const { return m_bits.parity ? 1 : -1; }
    //
    double grad(LineAxis axis) const {
        return (axis == LineAxis::YAXIS) ? sign() * height() / width()
                                         : sign() * width() / height();
    }
    double constant(LineAxis axis) const {
        // Using long doubles here to force higher accuracy of calculations
        // and thus parity of the x86 and arm64 results
        if (axis == LineAxis::YAXIS) {
            long double gaxis = grad(axis);
            long double axv = ax();
            long double ayv = ay();
            return static_cast<double>(ayv - (gaxis * axv));
        } else {
            long double gaxis = grad(axis);
            long double axv = ax();
            long double ayv = ay();
            return static_cast<double>(axv - (gaxis * ayv));
        }
    }
    //
    double length() const {
        return sqrt((topRight.x - bottomLeft.x) * (topRight.x - bottomLeft.x) +
                    (topRight.y - bottomLeft.y) * (topRight.y - bottomLeft.y));
    }
    //
    short direction() const { return m_bits.direction; }
    Point2f vector() const { return t_end() - t_start(); }
};
