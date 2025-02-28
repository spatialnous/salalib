// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "line4f.hpp"

// line set up

// default: nothing:

Line4f::Line4f() : m_bits() {
    m_bits.parity = 0;
    m_bits.direction = 0;
    // Points automatically assigned to 0,0
}

Line4f::Line4f(const Point2f &a, const Point2f &b) : m_bits() {
    if (a.x == b.x) {
        bottomLeft.x = a.x;
        topRight.x = b.x;
        // vertical lines stored consistently as parity 1
        if (a.y <= b.y) {
            bottomLeft.y = a.y;
            topRight.y = b.y;
            m_bits.parity = 1;
            m_bits.direction = 1;
        } else {
            bottomLeft.y = b.y;
            topRight.y = a.y;
            m_bits.parity = 1;
            m_bits.direction = 0;
        }
    } else if (a.x < b.x) {
        bottomLeft.x = a.x;
        topRight.x = b.x;
        if (a.y <= b.y) {
            bottomLeft.y = a.y;
            topRight.y = b.y;
            m_bits.parity = 1;
            m_bits.direction = 1;
        } else {
            bottomLeft.y = b.y;
            topRight.y = a.y;
            m_bits.parity = 0; // -1
            m_bits.direction = 1;
        }
    } else {
        bottomLeft.x = b.x;
        topRight.x = a.x;
        if (b.y <= a.y) {
            bottomLeft.y = b.y;
            topRight.y = a.y;
            m_bits.parity = 1;
            m_bits.direction = 0;
        } else {
            bottomLeft.y = a.y;
            topRight.y = b.y;
            m_bits.parity = 0; // -1
            m_bits.direction = 0;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

double dot(const Line4f &a, const Line4f &b) {
    return (a.bx() - a.ax()) * (b.bx() - b.ax()) + (a.by() - a.ay()) * (b.by() - b.ay());
}

// intersection test: touching counts as an intersection
// (uses dot product comparison)

// NB You must MUST check that line *regions do not intersect* before using this test
// By this test, *all parallel lines intersect*

double Line4f::intersection_point(const Line4f &l, LineAxis axis, double tolerance) const {
    // use axis = XAXIS for width() > height()
    double loc;
    if (axis == LineAxis::XAXIS) {
        if (l.width() == 0.0) {
            loc = l.bottomLeft.x;
        } else {
            // Using long doubles here to force higher accuracy of calculations
            // and thus parity of the x86 and arm64 results
            long double lg = l.grad(LineAxis::YAXIS);
            long double g = grad(LineAxis::YAXIS);
            if (fabs(static_cast<double>(lg - g)) <= tolerance) {
                // these have almost the same gradient, so it's impossible to tell where they
                // intersect: going for midpoint
                Point2f p = l.midpoint();
                loc = (p.x > topRight.x) ? topRight.x : ((p.x < bottomLeft.x) ? bottomLeft.x : p.x);
            } else {
                // this is the same as: constant(YAXIS) - l.constant(YAXIS)) / (l.grad(YAXIS) -
                // grad(YAXIS));
                // Using long doubles here to force higher accuracy of calculations
                // and thus parity of the x86 and arm64 results
                long double laxv = l.ax();
                long double layv = l.ay();
                long double axv = ax();
                long double ayv = ay();
                loc = static_cast<double>(((ayv - (g * axv)) - (layv - lg * laxv)) / (lg - g));
            }
        }
    } else {
        if (l.height() == 0.0) {
            loc = l.bottomLeft.y;
        } else {
            // Using long doubles here to force higher accuracy of calculations
            // and thus parity of the x86 and arm64 results
            long double lg = l.grad(LineAxis::XAXIS);
            long double g = grad(LineAxis::XAXIS);
            if (fabs(static_cast<double>(lg - g)) <= tolerance) {
                // these have almost the same gradient, so it's impossible to tell where they
                // intersect: going for midpoint
                Point2f p = l.midpoint();
                loc = (p.y > topRight.y) ? topRight.y : ((p.y < bottomLeft.y) ? bottomLeft.y : p.y);
            } else {
                // this is the same as: constant(XAXIS) - l.constant(XAXIS)) / (l.grad(XAXIS) -
                // grad(XAXIS));
                // Using long doubles here to force higher accuracy of calculations
                // and thus parity of the x86 and arm64 results
                long double laxv = l.ax();
                long double layv = l.ay();
                long double axv = ax();
                long double ayv = ay();
                loc = static_cast<double>(((axv - (g * ayv)) - (laxv - (lg * layv))) / (lg - g));
            }
        }
    }
    return loc;
}

// intersecting line segments, touching counts
// (uses intersection point comparison)

bool Line4f::intersect_line(const Line4f &l, LineAxis axis, double &loc) const {
    // please be intelligent when passing the axis...
    if (axis == LineAxis::XAXIS) {
        if (l.width() == 0.0) {
            // Special case:
            double y = ay() + sign() * (l.ax() - ax()) * height() / width();
            if (y >= bottomLeft.y && y <= l.topRight.y) { // <- you must have checked
                loc = l.bottomLeft.x;                     //    the regions overlap first
                return true;
            }
        } else {
            // Standard:   (note: if m1 == m2, loc is NaN)
            loc = (constant(LineAxis::YAXIS) - l.constant(LineAxis::YAXIS)) /
                  (l.grad(LineAxis::YAXIS) - grad(LineAxis::YAXIS));
            if (std::isnan(loc)) {
                // lines are parallel --- are they coincident?
                // you must have checked the regions overlap first
                if (constant(LineAxis::YAXIS) == l.constant(LineAxis::YAXIS)) {
                    return true;
                }
            } else if (loc >= l.bottomLeft.x && loc <= l.topRight.x) {
                return true;
            }
        }
    } else {
        if (l.height() == 0.0) {
            // Special case:
            double x = ax() + sign() * (l.ay() - ay()) * width() / height();
            if (x >= bottomLeft.x && x <= topRight.x) { // <- you must have checked
                loc = l.bottomLeft.y;                   //  the regions overlap first
                return true;
            }
        } else {
            // Standard:   (note: if m1 == m2, loc is NaN)
            loc = (constant(LineAxis::XAXIS) - l.constant(LineAxis::XAXIS)) /
                  (l.grad(LineAxis::XAXIS) - grad(LineAxis::XAXIS));
            if (std::isnan(loc)) {
                // lines are parallel --- are they coincident?
                // you must have checked the regions overlap first
                if (constant(LineAxis::XAXIS) == l.constant(LineAxis::XAXIS)) {
                    return true;
                }
            } else if (loc >= l.bottomLeft.y && loc <= l.topRight.y) {
                return true;
            }
        }
    }
    return false;
}

// this converts the loc back into a point:

Point2f Line4f::point_on_line(double loc, LineAxis axis) const {
    Point2f p;
    if (axis == LineAxis::XAXIS) {
        p = Point2f(loc, grad(LineAxis::YAXIS) * loc + constant(LineAxis::YAXIS));
    } else {
        p = Point2f(grad(LineAxis::XAXIS) * loc + constant(LineAxis::XAXIS), loc);
    }
    return p;
}

//////////////////////////////////////////////////////////////////////////////

// distance from a point to a line segment

double Line4f::dist(const Point2f &point) const {
    double d = 0.0;

    Point2f alpha = end() - start();
    Point2f beta = point - end();
    Point2f gamma = start() - end();
    Point2f delta = point - start();

    if (alpha.dot(beta) > 0) {
        d = beta.length();
    } else if (gamma.dot(delta) > 0) {
        d = delta.length();
    } else {
        if (alpha.length() < 1e-9 * beta.length()) {
            // should actually be a user-specified tolerance test
            d = beta.length();
        } else {
            d = fabs(alpha.det(beta)) / alpha.length();
        }
    }
    return d;
}

// crop a line to fit within bounds of region
// if line lies outside region, returns false

bool Line4f::crop(const Region4f &r) {
    if (bx() >= r.bottomLeft.x) {
        if (ax() < r.bottomLeft.x) {
            // crop!
            ay() += sign() * (height() * (r.bottomLeft.x - ax()) / width());
            ax() = r.bottomLeft.x;
        }
        if (ax() <= r.topRight.x) {
            if (bx() > r.topRight.x) {
                // crop!
                by() -= sign() * height() * (bx() - r.topRight.x) / width();
                bx() = r.topRight.x;
            }
            if (topRight.y >= r.bottomLeft.y) {
                if (bottomLeft.y < r.bottomLeft.y) {
                    // crop!
                    if (m_bits.parity) {
                        ax() += width() * (r.bottomLeft.y - bottomLeft.y) / height();
                    } else {
                        bx() -= width() * (r.bottomLeft.y - bottomLeft.y) / height();
                    }
                    bottomLeft.y = r.bottomLeft.y;
                }
                if (bottomLeft.y <= r.topRight.y) {
                    if (topRight.y > r.topRight.y) {
                        // crop!
                        if (m_bits.parity) {
                            bx() -= width() * (topRight.y - r.topRight.y) / height();
                        } else {
                            ax() += width() * (topRight.y - r.topRight.y) / height();
                        }
                        topRight.y = r.topRight.y;
                    }
                    // if we got this far, well done, it's in the region:
                    return true;
                }
            }
        }
    }
    // returns false if the entire line is outside the region:
    return false;
}

// cast a ray to the edge of a box

void Line4f::ray(short dir, const Region4f &r) {
    if (dir == m_bits.direction) {
        if (width() >= height()) {
            by() = ay() + sign() * height() * (r.topRight.x - ax()) / width();
            bx() = r.topRight.x;
        } else if (m_bits.parity) {
            bx() = ax() + width() * (r.topRight.y - ay()) / height();
            by() = r.topRight.y;
        } else {
            bx() = ax() + width() * (ay() - r.bottomLeft.y) / height();
            by() = r.bottomLeft.y;
        }
    } else {
        if (width() >= height()) {
            ay() = by() - sign() * height() * (bx() - r.bottomLeft.x) / width();
            ax() = r.bottomLeft.x;
        } else if (m_bits.parity) {
            ax() = bx() - width() * (by() - r.bottomLeft.y) / height();
            ay() = r.bottomLeft.y;
        } else {
            ax() = bx() - width() * (r.topRight.y - by()) / height();
            ay() = r.topRight.y;
        }
    }
    // now fit within bounds...
    crop(r);
}

Point2f Line4f::intersection_point(const Line4f &b, double tolerance) const {
    LineAxis axis = (width() >= height()) ? LineAxis::XAXIS : LineAxis::YAXIS;
    return point_on_line(intersection_point(b, axis, tolerance), axis);
}

bool Line4f::intersects(const Line4f &b, double tolerance) const {
    // Using long doubles here to force higher accuracy of calculations
    // and thus parity of the x86 and arm64 results

    long double aax = ax();
    long double aay = ay();
    long double abx = bx();
    long double aby = by();
    long double bax = b.ax();
    long double bay = b.ay();
    long double bbx = b.bx();
    long double bby = b.by();

    if (((aay - aby) * (bax - aax) + (abx - aax) * (bay - aay)) *
                ((aay - aby) * (bbx - aax) + (abx - aax) * (bby - aay)) <=
            tolerance &&
        ((bay - bby) * (aax - bax) + (bbx - bax) * (aay - bay)) *
                ((bay - bby) * (abx - bax) + (bbx - bax) * (aby - bay)) <=
            tolerance) {
        return true;
    }

    return false;
}

// intersection test: touching does not count as an intersection
// (uses dot product comparison)

bool Line4f::intersects_no_touch(const Line4f &b, double tolerance) const {
    // Using long doubles here to force higher accuracy of calculations
    // and thus parity of the x86 and arm64 results

    long double aax = ax();
    long double aay = ay();
    long double abx = bx();
    long double aby = by();
    long double bax = b.ax();
    long double bay = b.ay();
    long double bbx = b.bx();
    long double bby = b.by();

    if (((aay - aby) * (bax - aax) + (abx - aax) * (bay - aay)) *
                ((aay - aby) * (bbx - aax) + (abx - aax) * (bby - aay)) <
            -tolerance &&
        ((bay - bby) * (aax - bax) + (bbx - bax) * (aay - bay)) *
                ((bay - bby) * (abx - bax) + (bbx - bax) * (aby - bay)) <
            -tolerance) {
        return true;
    }

    return false;
}

// returns 0 for no intersect, 1 for touching and 2 for crossing
int Line4f::intersects_distinguish(const Line4f &b, double tolerance) const {
    // Using long doubles here to force higher accuracy of calculations
    // and thus parity of the x86 and arm64 results

    long double aax = ax();
    long double aay = ay();
    long double abx = bx();
    long double aby = by();
    long double bax = b.ax();
    long double bay = b.ay();
    long double bbx = b.bx();
    long double bby = b.by();

    long double alpha = ((aay - aby) * (bax - aax) + (abx - aax) * (bay - aay)) *
                        ((aay - aby) * (bbx - aax) + (abx - aax) * (bby - aay));

    long double beta = ((bay - bby) * (aax - bax) + (bbx - bax) * (aay - bay)) *
                       ((bay - bby) * (abx - bax) + (bbx - bax) * (aby - bay));

    if (alpha <= tolerance && beta <= tolerance) {
        if (alpha < -tolerance && beta < -tolerance) {
            return 2;
        } else {
            return 1;
        }
    }

    return 0;
}

// returns 0 for no intersect, 1 for touching and 2 for crossing
// n.b. only used by polygon contains -- throws if the first point of line b is touching line a
// (first point of line b is the point to be tested) -- i.e., throws if point touches polygon
int Line4f::intersects_b(const Line4f &b, double tolerance) const {
    // Using long doubles here to force higher accuracy of calculations
    // and thus parity of the x86 and arm64 results

    long double aax = ax();
    long double aay = ay();
    long double abx = bx();
    long double aby = by();
    long double bax = b.ax();
    long double bay = b.ay();
    long double bbx = b.bx();
    long double bby = b.by();

    long double alpha = ((aay - aby) * (bax - aax) + (abx - aax) * (bay - aay));

    long double beta = ((aay - aby) * (bbx - aax) + (abx - aax) * (bby - aay));

    long double gamma = ((bay - bby) * (aax - bax) + (bbx - bax) * (aay - bay)) *
                        ((bay - bby) * (abx - bax) + (bbx - bax) * (aby - bay));

    if (alpha * beta <= tolerance && gamma <= tolerance) {
        if (alpha * beta < -tolerance && gamma < -tolerance) {
            return 2;
        } else {
            // this function is only used for poly contains point,
            // the throw is defined if the point is *on* the polygon edge
            // (within the tolerance)
            if (fabs(static_cast<double>(alpha)) <= tolerance) {
                throw 1;
            }
            return 1;
        }
    }
    return 0;
}
