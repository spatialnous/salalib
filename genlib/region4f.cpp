// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "region4f.hpp"

// EdgeU is used for polygon clipping to viewports
// get the actual point from an EdgeU
Point2f Region4f::getEdgeUPoint(const EdgeU &eu) {
    switch (eu.edge) {
    case 0:
        return Point2f(bottomLeft.x + (eu.u * width()), bottomLeft.y);
    case 1:
        return Point2f(topRight.x, bottomLeft.y + (eu.u * height()));
    case 2:
        return Point2f(topRight.x - (eu.u * width()), topRight.y);
    case 3:
        return Point2f(bottomLeft.x, topRight.y - (eu.u * height()));
    }
    return Point2f();
}

// EdgeU is used for polygon clipping to viewports
// get where the polygon exits the viewport
EdgeU Region4f::getCutEdgeU(const Point2f &inside, const Point2f &outside) {
    EdgeU eu;
    if (outside.x < bottomLeft.x) {
        double y = outside.y +
                   (inside.y - outside.y) * (bottomLeft.x - outside.x) / (inside.x - outside.x);
        if (y >= bottomLeft.y && y <= topRight.y) {
            eu.edge = 3;
            eu.u = (topRight.y - y) / height();
        }
    }
    if (eu.edge == -1 && outside.x > topRight.x) {
        double y =
            inside.y + (outside.y - inside.y) * (topRight.x - inside.x) / (outside.x - inside.x);
        if (y >= bottomLeft.y && y <= topRight.y) {
            eu.edge = 1;
            eu.u = (y - bottomLeft.y) / height();
        }
    }
    if (eu.edge == -1 && outside.y < bottomLeft.y) {
        double x = outside.x +
                   (inside.x - outside.x) * (bottomLeft.y - outside.y) / (inside.y - outside.y);
        if (x >= bottomLeft.x && x <= topRight.x) {
            eu.edge = 0;
            eu.u = (x - bottomLeft.x) / width();
        }
    }
    if (eu.edge == -1 && outside.y > topRight.y) {
        double x =
            inside.x + (outside.x - inside.x) * (topRight.y - inside.y) / (outside.y - inside.y);
        if (x >= bottomLeft.x && x <= topRight.x) {
            eu.edge = 2;
            eu.u = (topRight.x - x) / width();
        }
    }
    // if at this stage eu.edge is still -1 there's a problem!
    return eu;
}

//////////////////////////////////////////////////////////////////////////

// union of two regions

Region4f Region4f::runion(const Region4f &b) const {
    Region4f n;
    n.bottomLeft.x = bottomLeft.x < b.bottomLeft.x ? bottomLeft.x : b.bottomLeft.x;
    n.bottomLeft.y = bottomLeft.y < b.bottomLeft.y ? bottomLeft.y : b.bottomLeft.y;
    n.topRight.x = topRight.x > b.topRight.x ? topRight.x : b.topRight.x;
    n.topRight.y = topRight.y > b.topRight.y ? topRight.y : b.topRight.y;
    return n;
}

// test intersecting regions, touching counts

bool Region4f::intersects(const Region4f &b, double tolerance) const {
    if (overlap_x(b, tolerance) && overlap_y(b, tolerance)) {
        return true;
    } else {
        return false;
    }
}

bool Region4f::overlap_x(const Region4f &b, double tolerance) const {
    if (bottomLeft.x > b.bottomLeft.x) {
        if (b.topRight.x >= bottomLeft.x - tolerance) {
            return true;
        }
    } else {
        if (topRight.x >= b.bottomLeft.x - tolerance) {
            return true;
        }
    }
    return false;
}

bool Region4f::overlap_y(const Region4f &b, double tolerance) const {
    if (bottomLeft.y > b.bottomLeft.y) {
        if (b.topRight.y >= bottomLeft.y - tolerance) {
            return true;
        }
    } else {
        if (topRight.y >= b.bottomLeft.y - tolerance) {
            return true;
        }
    }
    return false;
}
