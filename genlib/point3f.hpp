// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "point2f.hpp"

class Point3f {
  public:
    double x;
    double y;
    double z;
    Point3f(double a = 0.0, double b = 0.0, double c = 0.0) : x(a), y(b), z(c) {}
    Point3f(const Point2f &p)
        : x(p.x), y(0.0), z(p.y) {} // Note! not z = -y (due to an incosistency earlier...)
    bool inside(const Point3f &bl, const Point3f &tr) // now inclusive (...)
    {
        return (x >= bl.x && y >= bl.y && z >= bl.z && x <= tr.x && y <= tr.y && z <= tr.z);
    }
    operator Point2f() {
        return Point2f(x, z); // Note! not x, -z (due to an inconsistency earlier...)
    }
    Point2f xy() { return Point2f(x, y); } // From the x, y plane
    // A few simple vector ops:
    double length() const { return (double)sqrt(x * x + y * y + z * z); }
    Point3f &scale(const double scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    Point3f &normalise() { return scale(1.0 / length()); }
    Point3f &rotate(double theta, double phi) {
        double t = x;
        x = t * cos(theta) - y * sin(theta);
        y = y * cos(theta) + t * sin(theta);
        t = x;
        x = t * cos(phi) - z * sin(phi);
        z = z * cos(phi) - t * sin(phi);
        return *this;
    }
    //
    friend double dot(const Point3f &a, const Point3f &b);
    friend Point3f cross(const Point3f &a, const Point3f &b);
};

inline double dot(const Point3f &a, const Point3f &b) {
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}
inline Point3f cross(const Point3f &a, const Point3f &b) {
    return Point3f(a.y * b.z - b.y * a.z, a.z * b.x - b.z * a.x, a.x * b.y - b.x * a.y);
}
