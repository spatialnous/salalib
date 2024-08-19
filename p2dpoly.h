// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 2d poly (own format, adapted from the original Sala libraries

// The idea is that from this format,
// we can read into Cosmo3d as well as Sala based applications

#pragma once

// Using doubles right the way through can really eat memory for isovist
// polygon files, thus we use a defined type, change as appropriate:

#include "pafmath.h"

#include <algorithm>

// Note: code depends on XAXIS being 0 and YAXIS being 1 --- do not change
enum { NOAXIS = -1, XAXIS = 0, YAXIS = 1 };

class Point2f;
bool approxeq(const Point2f &p1, const Point2f &p2, double tolerance = 0.0);
class QtRegion;
bool intersect_region(const QtRegion &a, const QtRegion &b, double tolerance = 0.0);
bool overlap_x(const QtRegion &a, const QtRegion &b, double tolerance = 0.0);
bool overlap_y(const QtRegion &a, const QtRegion &b, double tolerance = 0.0);
class Line;
bool intersect_line(const Line &a, const Line &b, double tolerance = 0.0);
bool intersect_line_no_touch(const Line &a, const Line &b, double tolerance = 0.0);
int intersect_line_distinguish(const Line &a, const Line &b, double tolerance = 0.0);
int intersect_line_b(const Line &a, const Line &b, double tolerance = 0.0);
Point2f intersection_point(const Line &a, const Line &b, double tolerance = 0.0);

// NaN on Intel:
// Quick mod - TV
// const double P2DNULL = (const double)0xFFFFFFFF7FF7FFFF;
// for non-Intel:  0x7FF7FFFFFFFFFFFF

// Point

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
    void normalScale(const QtRegion &); // inline function: below region
    void denormalScale(const QtRegion &);
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
    double &operator[](int i) { return (i == XAXIS) ? x : y; }
    const double &operator[](int i) const { return (i == XAXIS) ? x : y; }
    friend Point2f operator-(Point2f &p);
    friend Point2f operator+(const Point2f &p1, const Point2f &p2);
    friend Point2f operator-(const Point2f &p1, const Point2f &p2);
    friend bool operator==(const Point2f &p1, const Point2f &p2);
    friend bool operator!=(const Point2f &p1, const Point2f &p2);
    friend bool operator>(const Point2f &a, const Point2f &b);
    friend bool operator<(const Point2f &a, const Point2f &b);
    friend Point2f operator*(const double s, const Point2f &p);
    friend Point2f operator/(const Point2f &p, const double s);
    friend double dot(const Point2f &p1, const Point2f &p2);
    friend double det(const Point2f &p1, const Point2f &p2);
    friend double dist(const Point2f &p1, const Point2f &p2);
    friend double dist(const Point2f &point, const Line &line);
    friend double angle(const Point2f &p1, const Point2f &p2, const Point2f &p3);
    friend bool approxeq(const Point2f &p1, const Point2f &p2, double tolerance);
    friend Point2f pointfromangle(double angle);
    // a couple of useful tests
    bool intriangle(const Point2f &p1, const Point2f &p2, const Point2f &p3);
    bool insegment(const Point2f &key, const Point2f &p2, const Point2f &p3,
                   double tolerance = 0.0);
    // for OS transformation (note: accurate only to 5 metres according to OS)
    Point2f longlat2os(const Point2f &p);

  public:
    // A few simple vector ops:
    double length() const { return (double)sqrt(x * x + y * y); }
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
};

inline Point2f operator-(Point2f &p) { return Point2f(-p.x, -p.y); }

inline Point2f operator+(const Point2f &p1, const Point2f &p2) {
    return Point2f(p1.x + p2.x, p1.y + p2.y);
}

inline Point2f operator-(const Point2f &p1, const Point2f &p2) {
    return Point2f(p1.x - p2.x, p1.y - p2.y);
}

inline bool operator==(const Point2f &p1, const Point2f &p2) {
    return (p1.x == p2.x && p1.y == p2.y);
}
inline bool operator!=(const Point2f &p1, const Point2f &p2) {
    return (p1.x != p2.x || p1.y != p2.y);
}
inline bool operator>(const Point2f &p1, const Point2f &p2) {
    return (p1.x > p2.x || (p1.x == p2.x && p1.y > p2.y));
}
inline bool operator<(const Point2f &p1, const Point2f &p2) {
    return (p1.x < p2.x || (p1.x == p2.x && p1.y < p2.y));
}

inline Point2f operator*(const double s, const Point2f &p) { return Point2f(s * p.x, s * p.y); }

inline Point2f operator/(const Point2f &p, const double s) { return Point2f(p.x / s, p.y / s); }

inline double dot(const Point2f &p1, const Point2f &p2) { return (p1.x * p2.x + p1.y * p2.y); }

// greater than 0 => p2 left (anticlockwise) of p1, less than 0 => p2 right (clockwise) of p1
inline double det(const Point2f &p1, const Point2f &p2) { return (p1.x * p2.y - p1.y * p2.x); }

inline double dist(const Point2f &p1, const Point2f &p2) {
    return sqrt(pafmath::sqr(p1.x - p2.x) + pafmath::sqr(p1.y - p2.y));
}

inline double angle(const Point2f &p1, const Point2f &p2, const Point2f &p3) {
    Point2f a = p1 - p2;
    Point2f b = p3 - p2;
    a.normalise();
    b.normalise();
    // ensure in range (f.p. error can throw out)
    double d = std::min<double>(std::max<double>(dot(a, b), -1.0), 1.0);
    return (pafmath::sgn(det(a, b)) == 1) ? acos(d) : 2.0 * M_PI - acos(d);
}

inline bool approxeq(const Point2f &p1, const Point2f &p2, double tolerance) {
    return (fabs(p1.x - p2.x) <= tolerance && fabs(p1.y - p2.y) <= tolerance);
}

inline bool Point2f::insegment(const Point2f &key, const Point2f &p2, const Point2f &p3,
                               double tolerance) {
    Point2f va = p2 - key;
    Point2f vb = p3 - key;
    Point2f vp = *this - key;
    double ap = det(va, vp);
    double bp = det(vb, vp);
    if ((dot(va, vp) > 0 && dot(vb, vp) > 0) &&
        (pafmath::sgn(ap) != pafmath::sgn(bp) || fabs(ap) < tolerance || fabs(bp) < tolerance)) {
        return true;
    }
    return false;
}

inline bool Point2f::intriangle(const Point2f &p1, const Point2f &p2, const Point2f &p3) {
    // touching counts
    int test = pafmath::sgn(det(p2 - p1, *this - p1));
    if (test == pafmath::sgn(det(p3 - p2, *this - p2)) &&
        test == pafmath::sgn(det(p1 - p3, *this - p3))) {
        return true;
    }
    return false;
}

inline Point2f pointfromangle(double angle) {
    Point2f p;
    p.x = cos(angle);
    p.y = sin(angle);
    return p;
}

Point2f gps2os(const Point2f &p);

// an event is a point plus time (as in spacetime technical language)
class Event2f : public Point2f {
  public:
    double t; // time in seconds
    Event2f() : Point2f(), t(0.0) {}
    Event2f(double x, double y, double t) : Point2f(x, y), t(t) {}
    Event2f(Point2f &p) : Point2f(p), t(0.0) {}
    Event2f(Point2f &p, double t) : Point2f(p), t(t) {}
};

///////////////////////////////////////////////////////////////////////////////////////////

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
        return Point2f(x, z);
    }                                      // Note! not x, -z (due to an inconsistency earlier...)
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

//

//////////////////////////////////////////////////////////////////////////////

// used for clipping of polygons to regions

struct EdgeU {
    int edge;
    double u;
    EdgeU(int e = -1, double u = 0.0) : edge(e), u(u) {}
    EdgeU(const EdgeU &eu) : edge(eu.edge), u(eu.u) {}
    friend bool ccwEdgeU(const EdgeU &a, const EdgeU &b, const EdgeU &c);
};

// QtRegion

class QtRegion {
  public:
    Point2f bottomLeft;
    Point2f topRight;
    QtRegion(const Point2f &bl = Point2f(), const Point2f &tr = Point2f())
        : bottomLeft(bl), topRight(tr) {}
    QtRegion(const QtRegion &r) : bottomLeft(r.bottomLeft), topRight(r.topRight) {}
    QtRegion &operator=(const QtRegion &r) {
        bottomLeft = r.bottomLeft;
        topRight = r.topRight;
        return *this;
    }
    bool operator==(const QtRegion &other) const {
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
    void normalScale(const QtRegion &r) {
        topRight.normalScale(r);
        bottomLeft.normalScale(r);
    }
    void denormalScale(const QtRegion &r) {
        topRight.denormalScale(r);
        bottomLeft.denormalScale(r);
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
    //
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
    //
    bool atZero() const { return bottomLeft.atZero() || topRight.atZero(); }
    //
    Point2f getEdgeUPoint(const EdgeU &eu);
    EdgeU getCutEdgeU(const Point2f &inside, const Point2f &outside);
    //
    friend bool intersect_region(const QtRegion &a, const QtRegion &b, double tolerance);
    friend bool overlap_x(const QtRegion &a, const QtRegion &b, double tolerance);
    friend bool overlap_y(const QtRegion &a, const QtRegion &b, double tolerance);
    //
    // set functions
    friend QtRegion runion(const QtRegion &a, const QtRegion &b);
    friend QtRegion rintersect(const QtRegion &a, const QtRegion &b); // undefined?
    //
    void grow(const double scalar) {
        Point2f dim = topRight - bottomLeft;
        dim.scale(scalar - 1.0);
        topRight += dim;
        bottomLeft -= dim;
    }
};

// First time we have a region available to use...
inline void Point2f::normalScale(const QtRegion &r) {
    if (r.width() != 0)
        x = (x - r.bottomLeft.x) / r.width();
    else
        x = 0.0;
    if (r.height() != 0)
        y = (y - r.bottomLeft.y) / r.height();
    else
        y = 0.0;
}

inline void Point2f::denormalScale(const QtRegion &r) {
    x = x * r.width() + r.bottomLeft.x;
    y = y * r.height() + r.bottomLeft.y;
}

// Lines are stored left to right as regions,
// the parity tells us whether the region should be inverted
// top to bottom to get the line

class Line : public QtRegion {
  protected:
    struct Bits {
        Bits() : xDummy(0), yDummy(0), zDummy(0) {}
        char parity : 8;    // 1 ... positive, 0 ... negative
        char direction : 8; // 1 ... positive, 0 ... negative

        // dummy variables as it seems to be necessary that the width of this struct is 8 bytes
        // and I don't want any uninitialised memory that gets written to file accidentally
        char xDummy : 8;
        char yDummy : 8;
        int zDummy : 32;
    };
    Bits m_bits;

  public:
    Line();
    Line(const Point2f &a, const Point2f &b);
    Line(const QtRegion &r) : QtRegion(r) {
        m_bits.parity = 1;
        m_bits.direction = 1;
    }
    Line(const Line &l) : QtRegion(l), m_bits(l.m_bits) {}
    Line &operator=(const Line &l) {
        this->QtRegion::operator=(l);
        m_bits = l.m_bits;
        return *this;
    }
    bool operator==(const Line &other) const {
        // we could be comparing QtRegion and then the bits, but this
        // is a line, and the two functions t_start and t_end seem
        // to provide all the necessary information for the test.
        return t_start() == other.t_start() && t_end() == other.t_end();
    }

    friend bool intersect_line(const Line &a, const Line &b, double tolerance);
    friend bool intersect_line_no_touch(const Line &a, const Line &b, double tolerance);
    friend int intersect_line_distinguish(const Line &a, const Line &b, double tolerance);
    friend int intersect_line_b(const Line &a, const Line &b, double tolerance);
    //
    // fills in the location along the axis where the intersection happens
    bool intersect_line(const Line &l, int axis, double &loc) const;
    double intersection_point(const Line &l, int axis, double tolerance = 0.0) const;
    // this converts a loc retrieved from intersect line or intersection point back into a point:
    Point2f point_on_line(double loc, int axis) const;
    // ...and a quick do it all in one go:
    friend Point2f intersection_point(const Line &a, const Line &b, double tolerance);
    //
    bool crop(const QtRegion &r);
    void ray(short dir, const QtRegion &r);
    //
    friend double dot(const Line &a, const Line &b);
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
    double grad(int axis) const {
        return (axis == YAXIS) ? sign() * height() / width() : sign() * width() / height();
    }
    double constant(int axis) const {
        return (axis == YAXIS) ? ay() - grad(axis) * ax() : ax() - grad(axis) * ay();
    }
    //
    double length() const {
        return (double)sqrt((topRight.x - bottomLeft.x) * (topRight.x - bottomLeft.x) +
                            (topRight.y - bottomLeft.y) * (topRight.y - bottomLeft.y));
    }
    //
    short direction() const { return m_bits.direction; }
    Point2f vector() const { return t_end() - t_start(); }
};

inline Point2f intersection_point(const Line &a, const Line &b, double tolerance) {
    int axis = (a.width() >= a.height()) ? XAXIS : YAXIS;
    return a.point_on_line(a.intersection_point(b, axis, tolerance), axis);
}

// plain 2-point line without regions
struct SimpleLine {
  public:
    SimpleLine(const Line &line) {
        m_start.x = line.t_start().x;
        m_start.y = line.t_start().y;
        m_end.x = line.t_end().x;
        m_end.y = line.t_end().y;
    }
    SimpleLine(const Point2f &a, const Point2f &b) {
        m_start.x = a.x;
        m_start.y = a.y;
        m_end.x = b.x;
        m_end.y = b.y;
    }
    SimpleLine(double x1, double y1, double x2, double y2) {
        m_start.x = x1;
        m_start.y = y1;
        m_end.x = x2;
        m_end.y = y2;
    }
    const Point2f &start() const { return m_start; }
    const Point2f &end() const { return m_end; }

  private:
    Point2f m_start;
    Point2f m_end;
};

////////////////////////////////////////////////////////////////////////////////////////

// not sure if this code is used any more:

// Now the difficult bit: making the line segments into polygons...
// The polygons are stored in a tree format so that intersection testing is easier

class RegionTree {
    friend class Poly;

  protected:
    Line *m_pRegion;
    RegionTree *m_pLeft;
    RegionTree *m_pRight;

  public:
    RegionTree() : m_pRegion(nullptr), m_pLeft(this), m_pRight(this) {}
    virtual ~RegionTree() {
        if (m_pRegion)
            delete m_pRegion;
    }
    //
    virtual bool is_leaf() const = 0;
    //
    RegionTree &left() const { return *m_pLeft; }
    RegionTree &right() const { return *m_pRight; }
    //
    operator QtRegion() const { return *(QtRegion *)m_pRegion; }
    operator Line() const { return *(Line *)m_pRegion; }
    //
    friend bool intersect(const RegionTree &a, const RegionTree &b);
    friend bool subintersect(const RegionTree &a, const RegionTree &b);
    friend int intersections(const RegionTree &a, const Line &b);
};

// Branch on a region tree...

class RegionTreeBranch : public RegionTree {
  public:
    RegionTreeBranch() : RegionTree() { ; }
    RegionTreeBranch(const Line &r, const RegionTree &a, const RegionTree &b) {
        m_pLeft = (RegionTree *)&a;
        m_pRight = (RegionTree *)&b;
        m_pRegion = new Line(r); // copy
    }
    bool is_leaf() const override { return false; }
};

// Leaf on a region tree...

class RegionTreeLeaf : public RegionTree {
  public:
    RegionTreeLeaf() : RegionTree() { ; }
    RegionTreeLeaf(const Line &l) {
        // no subnodes (but nice recursive properties)
        m_pLeft = this;
        m_pRight = this;
        m_pRegion = new Line(l);
    }
    bool is_leaf() const override { return true; }
};

class Poly {
  protected:
    int m_lineSegments;
    RegionTree *m_pRoot;

  public:
    Poly() : m_lineSegments(0), m_pRoot(nullptr) {}
    Poly(const Poly &p) : m_lineSegments(p.m_lineSegments), m_pRoot(copy_region_tree(p.m_pRoot)) {}
    Poly &operator=(const Poly &p) {
        if (this != &p) {
            m_lineSegments = p.m_lineSegments;
            m_pRoot = copy_region_tree(p.m_pRoot);
        }
        return *this;
    }
    virtual ~Poly() { destroy_region_tree(); }
    // essentially, the copy constructor...
    RegionTree *copy_region_tree(const RegionTree *tree);
    // essentially, the destructor...
    void destroy_region_tree();
    //
    RegionTree &get_region_tree() const { return *m_pRoot; }
    //
    void add_line_segment(const Line &l);
    //
    int get_line_segments() { return m_lineSegments; }
    QtRegion get_bounding_box() { return *(QtRegion *)(m_pRoot->m_pRegion); }
    //
    bool contains(const Point2f &p);
    friend bool intersect(const Poly &a, const Poly &b);
};
