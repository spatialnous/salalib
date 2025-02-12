// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "pafmath.hpp"

#include <algorithm>

// Note: code depends on XAXIS being 0 and YAXIS being 1 --- do not change
enum class LineAxis { NOAXIS = -1, XAXIS = 0, YAXIS = 1 };

// NaN on Intel:
// Quick mod - TV
// const double P2DNULL = (const double)0xFFFFFFFF7FF7FFFF;
// for non-Intel:  0x7FF7FFFFFFFFFFFF

class Point2f {
  public:
    double x;
    double y;
    Point2f()
        : x(0.0), y(0.0)
    //      { x = P2DNULL; y = P2DNULL; }
    {}
    Point2f(double a, double b) : x(a), y(b) {}
    bool atZero() const
    //      { return x == P2DNULL || y == P2DNULL; }
    {
        return x == 0.0 && y == 0.0;
    }
    void normalScale(const Point2f &rbl, double width, double height);
    void denormalScale(const Point2f &rbl, double width, double height);
    void operator+=(const Point2f &p) {
        x += p.x;
        y += p.y;
    }
    void operator-=(const Point2f &p) {
        x -= p.x;
        y -= p.y;
    }
    void operator*=(const double s) {
        x *= s;
        y *= s;
    }
    void operator/=(const double s) {
        x /= s;
        y /= s;
    }
    double &operator[](LineAxis i) { return (i == LineAxis::XAXIS) ? x : y; }
    const double &operator[](LineAxis i) const { return (i == LineAxis::XAXIS) ? x : y; }
    Point2f operator-() const { return Point2f(-x, -y); }
    Point2f operator+(const Point2f &p2) const { return Point2f(x + p2.x, y + p2.y); }
    Point2f operator-(const Point2f &p2) const { return Point2f(x - p2.x, y - p2.y); }
    bool operator==(const Point2f &p2) const { return (x == p2.x && y == p2.y); }
    bool operator!=(const Point2f &p2) const { return (x != p2.x || y != p2.y); }
    bool operator>(const Point2f &p2) const { return (x > p2.x || (x == p2.x && y > p2.y)); }
    bool operator<(const Point2f &p2) const { return (x < p2.x || (x == p2.x && y < p2.y)); }
    Point2f operator*(const double s) const { return Point2f(s * x, s * y); }
    Point2f operator/(const double s) const { return Point2f(x / s, y / s); }
    double dot(const Point2f &p2) const { return (x * p2.x + y * p2.y); }
    double det(const Point2f &p2) const { return (x * p2.y - y * p2.x); }
    double dist(const Point2f &p2) const {
        return sqrt(pafmath::sqr(x - p2.x) + pafmath::sqr(y - p2.y));
    }
    double angle(const Point2f &p2, const Point2f &p3) const {
        Point2f a = *this - p2;
        Point2f b = p3 - p2;
        a.normalise();
        b.normalise();
        // ensure in range (f.p. error can throw out)
        double d = std::min<double>(std::max<double>(a.dot(b), -1.0), 1.0);
        return (pafmath::sgn(a.det(b)) == 1) ? acos(d) : 2.0 * M_PI - acos(d);
    }
    bool approxeq(const Point2f &p2, double tolerance = 0.0) const {
        return (fabs(x - p2.x) <= tolerance && fabs(y - p2.y) <= tolerance);
    }

    // a couple of useful tests
    bool intriangle(const Point2f &p1, const Point2f &p2, const Point2f &p3);
    bool insegment(const Point2f &key, const Point2f &p2, const Point2f &p3,
                   double tolerance = 0.0);

    // A few simple vector ops:
    double length() const { return sqrt(x * x + y * y); }
    Point2f &scale(const double scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    Point2f &scale(const Point2f &scalevec) {
        x *= scalevec.x;
        y *= scalevec.y;
        return *this;
    }
    Point2f &normalise() { return scale(1.0 / length()); }
    Point2f &rotate(const double angle) {
        double t = x;
        x = x * cos(angle) - y * sin(angle);
        y = y * cos(angle) + t * sin(angle);
        return *this;
    }
    double angle() const { return (y < 0) ? (2.0 * M_PI - acos(x)) : acos(x); }

    Point2f gps2os() const;
};
