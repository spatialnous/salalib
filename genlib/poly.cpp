// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "poly.hpp"

#include <vector>

namespace {

    int bitcount(int a) {
        int ret = 0;
        while (a != 0) {
            ret += (a & 1) ? 1 : 0;
            a = a >> 1;
        }
        return ret;
    }
} // namespace

// Polygon set up (the hard bit!)

void Poly::add_line_segment(const Line4f &l) {
    m_lineSegments++;
    RegionTreeLeaf *leaf = new RegionTreeLeaf(l);

    if (m_pRoot == nullptr) {
        // first ever node

        m_pRoot = static_cast<RegionTree *>(leaf);
    } else {
        // traverse the tree to the insertion point
        //   you'll just have to take my word for it that the next line
        //   gives you the correct position to insert
        int cutLevel = bitcount(m_lineSegments - 1) - 2;

        if (cutLevel < 0) {
            // replace the root node

            Region4f newRegion = m_pRoot->getInternalRegion().runion(leaf->getInternalRegion());
            RegionTree *newRoot = new RegionTreeBranch(newRegion, *m_pRoot, *leaf);
            m_pRoot = newRoot;
        } else {
            RegionTree *here = m_pRoot;
            for (int i = 0; i < cutLevel; i++) {
                here = &here->right();
            }

            // cut and insert

            RegionTree &insertionPoint = here->right();

            Region4f newRegion =
                insertionPoint.getInternalRegion().runion(leaf->getInternalRegion());

            RegionTree *newNode = new RegionTreeBranch(newRegion, insertionPoint, *leaf);

            here->setRight(newNode);

            // traverse up tree unioning regions
            // (saving data by not recording parents!)
            // Note must be '>=' to catch current root node --- I really stuffed up earlier with
            // '>'!
            while (cutLevel >= 0) {
                here = m_pRoot;
                for (int j = 0; j < cutLevel; j++) {
                    here = &here->right();
                }
                here->setRegion(new Line4f(
                    here->left().getInternalRegion().runion(here->right().getInternalRegion())));
                cutLevel--;
            }
        }
    }
}

// ...and after all the efficient stuff, we have a really
// inefficient polygon copy... hmm

RegionTree *Poly::copy_region_tree(const RegionTree *tree) {

    if (!tree) {
        return nullptr;
    }
    return tree->copy();
}

// polygon destruction

void Poly::destroy_region_tree() {
    if (!m_pRoot) {
        return;
    }

    m_pRoot->destroy();

    m_pRoot = nullptr;
}

// contains? intersects??

// Here they are!

bool Poly::contains(const Point2f &p) {
    // n.b., intersections throws on some accidental alignments --
    // we must use a point outside the polygon to extend our test
    // line from to prevent them
    Line4f l(p, Point2f(get_bounding_box().topRight.x + get_bounding_box().width(),
                        get_bounding_box().topRight.y + get_bounding_box().height()));

    int doubleN;

    // note, touching intersections count 1/2
    try {
        doubleN = m_pRoot->intersections(l);
    } catch (int) {
        throw 1; // throws if on edge
    }

    if (doubleN % 2 == 0 && doubleN % 4 != 0) {
        return true;
    }

    return false;
}

bool Poly::intersects(const Poly &b) {
    if (m_pRoot->intersect(*(b.m_pRoot))) {
        return true;
    }
    return false;
}
