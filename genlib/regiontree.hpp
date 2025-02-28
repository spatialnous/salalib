// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "line4f.hpp"
// not sure if this code is used any more:

// Now the difficult bit: making the line segments into polygons...
// The polygons are stored in a tree format so that intersection testing is easier

class RegionTree {
  protected:
    Line4f *m_pRegion;
    RegionTree *m_pLeft;
    RegionTree *m_pRight;

  public:
    RegionTree() : m_pRegion(nullptr), m_pLeft(this), m_pRight(this) {}
    RegionTree(const RegionTree &) = default;
    RegionTree &operator=(const RegionTree &) = default;
    virtual ~RegionTree() {
        if (m_pRegion)
            delete m_pRegion;
    }
    RegionTree *copy() const;
    void destroy();
    //
    virtual bool is_leaf() const = 0;
    //
    RegionTree &left() const { return *m_pLeft; }
    RegionTree &right() const { return *m_pRight; }

    void setLeft(RegionTree *left) { m_pLeft = left; }
    void setRight(RegionTree *right) { m_pRight = right; }

    void setRegion(Line4f *region) { m_pRegion = region; }
    //
    Region4f &getInternalRegion() const { return *static_cast<Region4f *>(m_pRegion); }
    Line4f &getInternalLine() const { return *static_cast<Line4f *>(m_pRegion); }
    //
    bool intersect(const RegionTree &b) const;
    bool subintersect(const RegionTree &b) const;
    int intersections(const Line4f &b) const;
};

// Branch on a region tree...

class RegionTreeBranch : public RegionTree {
  public:
    RegionTreeBranch() : RegionTree() {}
    RegionTreeBranch(const Line4f &r, RegionTree &a, RegionTree &b) {
        m_pLeft = static_cast<RegionTree *>(&a);
        m_pRight = static_cast<RegionTree *>(&b);
        m_pRegion = new Line4f(r); // copy
    }
    bool is_leaf() const override { return false; }
};

// Leaf on a region tree...

class RegionTreeLeaf : public RegionTree {
  public:
    RegionTreeLeaf() : RegionTree() {}
    RegionTreeLeaf(const Line4f &l) {
        // no subnodes (but nice recursive properties)
        m_pLeft = this;
        m_pRight = this;
        m_pRegion = new Line4f(l);
    }
    bool is_leaf() const override { return true; }
};
