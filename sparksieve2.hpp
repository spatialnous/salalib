// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is my code to make a set of axial lines from a set of boundary lines

#pragma once

#include "genlib/line4f.hpp"

#include <cstdint>
#include <float.h>
#include <list>
#include <map>
#include <vector>

class sparkSieve2 {
  public:
    struct sparkZone2 {
        double start;
        double end;
        bool remove;

      private:
        [[maybe_unused]] unsigned _padding0 : 3 * 8;
        [[maybe_unused]] unsigned _padding1 : 4 * 8;

      public:
        sparkZone2(double s = 0.0, double e = 0.0)
            : start(s), end(e), remove(false), _padding0(0), _padding1(0) {}
        // to allow ordered lists:
        friend bool operator==(const sparkZone2 &a, const sparkZone2 &b);
        friend bool operator!=(const sparkZone2 &a, const sparkZone2 &b);
        friend bool operator<(const sparkZone2 &a, const sparkZone2 &b);
        friend bool operator>(const sparkZone2 &a, const sparkZone2 &b);
    };

  private:
    Point2f m_centre;
    double m_maxdist; // for creating graphs that only see out a certain distance: set to -1.0 for
                      // infinite
    std::vector<sparkZone2> m_blocks;

  public:
    std::list<sparkZone2> gaps;

  public:
    sparkSieve2(const Point2f &centre, double maxdist = -1.0);
    ~sparkSieve2();
    bool testblock(const Point2f &point, const std::vector<Line4f> &lines, double tolerance);
    void block(const std::vector<Line4f> &lines, int q);
    void collectgarbage();
    double tanify(const Point2f &point, int q);
    //
    bool hasGaps() const { return (!gaps.empty()); }
};

inline bool operator==(const sparkSieve2::sparkZone2 &a, const sparkSieve2::sparkZone2 &b) {
    return (a.start == b.start && a.end == b.end);
}
inline bool operator!=(const sparkSieve2::sparkZone2 &a, const sparkSieve2::sparkZone2 &b) {
    return (a.start != b.start || a.end != b.end);
}
inline bool operator<(const sparkSieve2::sparkZone2 &a, const sparkSieve2::sparkZone2 &b) {
    return (a.start == b.start) ? (a.end > b.end) : (a.start < b.start);
}
inline bool operator>(const sparkSieve2::sparkZone2 &a, const sparkSieve2::sparkZone2 &b) {
    return (a.start == b.start) ? (a.end < b.end) : (a.start > b.start);
}
