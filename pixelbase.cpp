// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is my code to make a set of axial lines from a set of boundary lines

// spatial data

#include "pixelbase.hpp"

#include "genlib/readwritehelpers.hpp"
#include "genlib/stringutils.hpp"

#include <cmath>
#include <fstream>
#include <set>

/*
// Algorithm from Chi
// make sure dx > dy
   dx = x1 - x0;
   dy = y1 - y0;
   x = x0; y = y0;
   d = 2*dy - dx;
   inc1 = 2*dy;
   inc2 = 2*(dy-dx);
   while (x < x1) {
      if (d <= 0) {
         d += inc1;
         x += 1;
      }
      else {
         d += inc2;
         x++;
         y++;
      }
      pixel_list.push_back( PixelRef(x,y) );
   }
*/

PixelRefVector PixelBase::pixelateLine(Line4f l, int scalefactor) const {
    PixelRefVector pixelList;

    // this is *not* correct for lines that are off the edge...
    // should use non-constrained version (false), and find where line enters the
    // region
    PixelRef a = pixelate(l.start(), true, scalefactor);
    PixelRef b = pixelate(l.end(), true, scalefactor);

    l.normalScale(m_region);

    pixelList.push_back(a);

    int scaledcols = static_cast<int>(m_cols) * scalefactor;
    int scaledrows = static_cast<int>(m_rows) * scalefactor;

    int parity = 1; // Line goes upwards
    if (a.y > b.y) {
        parity = -1; // Line goes downwards
        a.y *= -1;
        b.y *= -1; // Set ay and by saves work on comparisons later on
    }

    // special case 1
    if (a.x == b.x) {
        while (a.y < b.y) {
            a.y += 1;
            pixelList.push_back(PixelRef(a.x, static_cast<short>(parity * a.y)));
        }
    } else if (a.y == b.y) {
        while (a.x < b.x) {
            a.x += 1;
            pixelList.push_back(
                PixelRef(a.x, static_cast<short>(parity * a.y))); // Lines always go left to right
        }
    } else {

        double hwRatio = l.height() / l.width(); // Working all of these out leaves less scope
                                                 // for floating point error
        double whRatio = l.width() / l.height();
        double x0Const = l.ay() - static_cast<double>(parity) * hwRatio * l.ax();
        double y0Const = l.ax() - static_cast<double>(parity) * whRatio * l.ay();

        while (a.x < b.x || a.y < b.y) {
            PixelRef e;
            e.y = static_cast<short>(
                parity * static_cast<int>(static_cast<double>(scaledrows) *
                                          (x0Const + parity * hwRatio *
                                                         (static_cast<double>(a.x + 1) /
                                                          static_cast<double>(scaledcols)))));
            // Note when decending 1.5 -> 1 and ascending 1.5 -> 2
            if (parity < 0) {
                e.x = static_cast<short>(static_cast<double>(scaledcols) *
                                         (y0Const + whRatio * (static_cast<double>(a.y) /
                                                               static_cast<double>(scaledrows))));
            } else {
                e.x = static_cast<short>(static_cast<double>(scaledcols) *
                                         (y0Const + whRatio * (static_cast<double>(a.y + 1) /
                                                               static_cast<double>(scaledrows))));
            }

            if (a.y < e.y) {
                while (a.y < e.y && a.y < b.y) {
                    a.y += 1;
                    pixelList.push_back(PixelRef(a.x, static_cast<short>(parity * a.y)));
                }
                if (a.x < b.x) {
                    a.x += 1;
                    pixelList.push_back(PixelRef(a.x, static_cast<short>(parity * a.y)));
                }
            } else if (a.x < e.x) {
                while (a.x < e.x && a.x < b.x) {
                    a.x += 1;
                    pixelList.push_back(PixelRef(a.x, static_cast<short>(parity * a.y)));
                }
                if (a.y < b.y) {
                    a.y += 1;
                    pixelList.push_back(PixelRef(a.x, static_cast<short>(parity * a.y)));
                }
            } else {
                // Special case: exactly diagonal step (should only require one step):
                // (Should actually never happen) (Doesn't: checked with RFH)

                if (a.x < b.x) {
                    a.x += 1;
                    pixelList.push_back(PixelRef(a.x, static_cast<short>(parity * a.y)));
                }
                if (a.y < b.y) {
                    a.y += 1;
                    pixelList.push_back(PixelRef(a.x, static_cast<short>(parity * a.y)));
                }
            }
        }
    }
    return pixelList;
}

// this version includes all pixels through which the line passes with touching
// counting as both pixels.

PixelRefVector PixelBase::pixelateLineTouching(Line4f l, double tolerance) const {
    PixelRefVector pixelList;

    // now assume that scaling to region then scaling up is going to give
    // pixelation this is not necessarily the case!
    l.normalScale(m_region);
    l.scale(Point2f(static_cast<double>(m_cols), static_cast<double>(m_rows)));

    // but it does give us a nice line...
    LineAxis dir;
    double grad, constant;

    if (l.width() > l.height()) {
        dir = LineAxis::XAXIS;
        grad = l.grad(LineAxis::YAXIS);
        constant = l.constant(LineAxis::YAXIS);
    } else if (l.width() == 0 && l.height() == 0) {
        dir = LineAxis::YAXIS;
        grad = 0;
        constant = 0;
    } else {
        dir = LineAxis::YAXIS;
        grad = l.grad(LineAxis::XAXIS);
        constant = l.constant(LineAxis::XAXIS);
    }
    PixelRef bounds(static_cast<short>(m_cols), static_cast<short>(m_rows));

    if (dir == LineAxis::XAXIS) {
        auto first = static_cast<int>(floor(l.ax() - tolerance));
        auto last = static_cast<int>(floor(l.bx() + tolerance));
        for (int i = first; i <= last; i++) {
            auto j1 = static_cast<int>(floor((first == i ? l.ax() : static_cast<double>(i)) * grad +
                                             constant - l.sign() * tolerance));
            auto j2 =
                static_cast<int>(floor((last == i ? l.bx() : static_cast<double>(i + 1)) * grad +
                                       constant + l.sign() * tolerance));
            if (bounds.encloses(PixelRef(static_cast<short>(i), static_cast<short>(j1)))) {
                pixelList.push_back(PixelRef(static_cast<short>(i), static_cast<short>(j1)));
            }
            if (j1 != j2) {
                if (bounds.encloses(PixelRef(static_cast<short>(i), static_cast<short>(j2)))) {
                    pixelList.push_back(PixelRef(static_cast<short>(i), static_cast<short>(j2)));
                }
                if (abs(j2 - j1) == 2) {
                    // this rare event happens if lines are exactly diagonal
                    int j3 = (j1 + j2) / 2;
                    if (bounds.encloses(PixelRef(static_cast<short>(i), static_cast<short>(j3)))) {
                        pixelList.push_back(
                            PixelRef(static_cast<short>(i), static_cast<short>(j3)));
                    }
                }
            }
        }
    } else {
        auto first = static_cast<int>(floor(l.bottomLeft.y - tolerance));
        auto last = static_cast<int>(floor(l.topRight.y + tolerance));
        for (int i = first; i <= last; i++) {
            auto j1 = static_cast<int>(
                floor((first == i ? l.bottomLeft.y : static_cast<double>(i)) * grad + constant -
                      l.sign() * tolerance));
            auto j2 = static_cast<int>(
                floor((last == i ? l.topRight.y : static_cast<double>(i + 1)) * grad + constant +
                      l.sign() * tolerance));
            if (bounds.encloses(PixelRef(static_cast<short>(j1), static_cast<short>(i)))) {
                pixelList.push_back(PixelRef(static_cast<short>(j1), static_cast<short>(i)));
            }
            if (j1 != j2) {
                if (bounds.encloses(PixelRef(static_cast<short>(j2), static_cast<short>(i)))) {
                    pixelList.push_back(PixelRef(static_cast<short>(j2), static_cast<short>(i)));
                }
                if (abs(j2 - j1) == 2) {
                    // this rare event happens if lines are exactly diagonal
                    int j3 = (j1 + j2) / 2;
                    if (bounds.encloses(PixelRef(static_cast<short>(j3), static_cast<short>(i)))) {
                        pixelList.push_back(
                            PixelRef(static_cast<short>(j3), static_cast<short>(i)));
                    }
                }
            }
        }
    }

    return pixelList;
}

// this version for a quick set of pixels

PixelRefVector PixelBase::quickPixelateLine(PixelRef p, PixelRef q) const {
    PixelRefVector list;

    double dx = q.x - p.x;
    double dy = q.y - p.y;
    int polarity = -1;
    double t = 0;
    // Quick mod - TV
#if defined(_MSC_VER)
    if (abs(dx) == abs(dy)) {
#else
    if (fabs(dx) == fabs(dy)) {
#endif
        polarity = 0;
    }
#if defined(_MSC_VER)
    else if (abs(dx) > abs(dy)) {
        t = abs(dx);
#else
    else if (fabs(dx) > fabs(dy)) {
        t = fabs(dx);
#endif
        polarity = 1;
    } else {
#if defined(_MSC_VER)
        t = abs(dy);
#else
        t = fabs(dy);
#endif
        polarity = 2;
    }

    if (polarity != 0) {
        dx /= t;
        dy /= t;
    }
    double ppx = p.x + 0.5;
    double ppy = p.y + 0.5;

    for (int i = 0; i <= t; i++) {
        if (polarity == 1 && fabs(floor(ppy) - ppy) < 1e-9) {
            list.push_back(PixelRef(static_cast<short>(floor(ppx)), //
                                    static_cast<short>(floor(ppy + 0.5))));
            list.push_back(PixelRef(static_cast<short>(floor(ppx)), //
                                    static_cast<short>(floor(ppy - 0.5))));
        } else if (polarity == 2 && fabs(floor(ppx) - ppx) < 1e-9) {
            list.push_back(PixelRef(static_cast<short>(floor(ppx + 0.5)), //
                                    static_cast<short>(floor(ppy))));
            list.push_back(PixelRef(static_cast<short>(floor(ppx - 0.5)), //
                                    static_cast<short>(floor(ppy))));
        } else {
            list.push_back(PixelRef(static_cast<short>(floor(ppx)), //
                                    static_cast<short>(floor(ppy))));
        }
        ppx += dx;
        ppy += dy;
    }

    return list;
}
