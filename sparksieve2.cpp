// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is my code to make a set of axial lines from a set of boundary lines

/////////////////////////////////////////////////////////////////////////////////

// New spark sieve implemementation (more accurate)

#include "sparksieve2.h"

#include <cmath>

sparkSieve2::sparkSieve2(const Point2f &centre, double maxdist) {
    m_centre = centre;
    m_maxdist = maxdist;

    gaps.push_back(sparkZone2(0.0, 1.0));
}

sparkSieve2::~sparkSieve2() {}

bool sparkSieve2::testblock(const Point2f &point, const std::vector<Line> &lines,
                            double tolerance) {
    Line l(m_centre, point);

    // maxdist is to construct graphs with a maximum visible distance: (-1.0 is
    // infinite)
    if (m_maxdist != -1.0 && l.length() > m_maxdist) {
        return true;
    }

    for (const auto &line : lines) {
        // Note: must check regions intersect before using this intersect_line test
        // -- see notes on intersect_line
        if (intersect_region(l, line, tolerance) && intersect_line(l, line, tolerance)) {
            return true;
        }
    }

    return false;
}

//

void sparkSieve2::block(const std::vector<Line> &lines, int q) {
    for (const auto &line : lines) {
        double a = tanify(line.start(), q);
        double b = tanify(line.end(), q);

        sparkZone2 block;
        if (a < b) {
            block.start = a - 1e-10; // 1e-10 required for floating point error
            block.end = b + 1e-10;
        } else {
            block.start = b - 1e-10; // 1e-10 required for floating point error
            block.end = a + 1e-10;
        }
        // this creates a list of blocks sorted by start location
        m_blocks.push_back(block);
    }
    std::sort(m_blocks.begin(), m_blocks.end());
    m_blocks.erase(std::unique(m_blocks.begin(), m_blocks.end()), m_blocks.end());
}

void sparkSieve2::collectgarbage() {
    auto iter = gaps.begin();
    auto blockIter = m_blocks.begin();

    for (; blockIter != m_blocks.end() && iter != gaps.end();) {
        if (blockIter->end < iter->start) {
            blockIter++;
            continue;
        }
        bool create = true;
        if (blockIter->start <= iter->start) {
            create = false;
            if (blockIter->end > iter->start) {
                // simply move the start in front of us
                iter->start = blockIter->end;
            }
        }
        if (blockIter->end >= iter->end) {
            create = false;
            if (blockIter->start < iter->end) {
                // move the end behind us
                iter->end = blockIter->start;
            }
        }
        if (iter->end <= iter->start + 1e-10) { // 1e-10 required for floating point error
            iter = gaps.erase(iter);
            continue; // on the next iteration, stay with this block
        } else if (blockIter->end > iter->end) {
            ++iter;
            continue; // on the next iteration, stay with this block
        } else if (create) {
            // add a new gap (has to be behind us), and move the start in front of us
            gaps.insert(iter, sparkZone2(iter->start, blockIter->start));
            iter->start = blockIter->end;
        }
        blockIter++;
    }
    // reset blocks for next row:
    m_blocks.clear();
}

/* q quadrants:
 *
 *       \ 6 | 7 /
 *       0 \ | / 1
 *       - -   - -
 *       2 / | \ 3
 *       / 4 | 5 \
 */

double sparkSieve2::tanify(const Point2f &point, int q) {
    switch (q) {
    case 0:
        return (point.y - m_centre.y) / (m_centre.x - point.x);
        break;
    case 1:
        return (point.y - m_centre.y) / (point.x - m_centre.x);
        break;
    case 2:
        return (m_centre.y - point.y) / (m_centre.x - point.x);
        break;
    case 3:
        return (m_centre.y - point.y) / (point.x - m_centre.x);
        break;
    case 4:
        return (m_centre.x - point.x) / (m_centre.y - point.y);
        break;
    case 5:
        return (point.x - m_centre.x) / (m_centre.y - point.y);
        break;
    case 6:
        return (m_centre.x - point.x) / (point.y - m_centre.y);
        break;
    case 7:
        return (point.x - m_centre.x) / (point.y - m_centre.y);
        break;
    }
    return -1.0;
}
