// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "regiontree.h"
#include <cstdint>

class Poly {
  protected:
    RegionTree *m_pRoot;
    int m_lineSegments;
#ifdef USE_EXPLICIT_PADDING
    unsigned : 4 * 8; // padding
#endif
  public:
    Poly() : m_pRoot(nullptr), m_lineSegments(0) {}
    Poly(const Poly &p) : m_pRoot(copy_region_tree(p.m_pRoot)), m_lineSegments(p.m_lineSegments) {}
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

    RegionTree &get_region_tree() const { return *m_pRoot; }

    void add_line_segment(const Line4f &l);

    int get_line_segments() { return m_lineSegments; }
    Region4f get_bounding_box() { return m_pRoot->getInternalRegion(); }

    bool contains(const Point2f &p);

    bool intersects(const Poly &b);
};
