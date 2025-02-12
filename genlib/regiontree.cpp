// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "salalib/genlib/regiontree.hpp"
#include <vector>

RegionTree *RegionTree::copy() const {

    RegionTree *newtree;

    if (is_leaf()) {
        newtree = new RegionTreeLeaf();
        newtree->m_pRegion = &getInternalLine();
        return newtree;
    } else {
        newtree = new RegionTreeBranch();
    }

    std::vector<RegionTree *> newlist;
    std::vector<const RegionTree *> oldlist;

    oldlist.push_back(static_cast<const RegionTree *>(this));
    newlist.push_back(static_cast<RegionTree *>(newtree));

    do {
        const RegionTree *oldnode = oldlist.back();
        oldlist.pop_back();
        RegionTree *newnode = newlist.back();
        newlist.pop_back();

        newnode->m_pRegion = new Line4f(*oldnode->m_pRegion);

        if (oldnode->m_pLeft) {
            if (oldnode->m_pLeft->is_leaf()) {
                newnode->m_pLeft = new RegionTreeLeaf();
                newnode->m_pLeft->m_pRegion = new Line4f(*(oldnode->m_pLeft->m_pRegion));
            } else {
                oldlist.push_back(oldnode->m_pLeft);
                newnode->m_pLeft = new RegionTreeBranch();
                newlist.push_back(newnode->m_pLeft);
            }
        }
        if (oldnode->m_pRight) {
            if (oldnode->m_pRight->is_leaf()) {
                newnode->m_pRight = new RegionTreeLeaf();
                newnode->m_pRight->m_pRegion = new Line4f(*(oldnode->m_pRight->m_pRegion));
            } else {
                oldlist.push_back(oldnode->m_pRight);
                newnode->m_pRight = new RegionTreeBranch();
                newlist.push_back(newnode->m_pRight);
            }
        }

    } while (oldlist.size() > 0);

    return newtree;
}

void RegionTree::destroy() {
    std::vector<RegionTree *> delNodeList;
    std::vector<short> delNodeDir;

    delNodeList.push_back(this);

    do {
        RegionTree *currentNode = delNodeList.back();

        if (currentNode->m_pLeft == currentNode && currentNode->m_pRight == currentNode) {

            delete currentNode;
            delNodeList.pop_back();

            if (delNodeList.size() > 0) {
                if (delNodeDir.back() == 0) {
                    delNodeList.back()->m_pLeft = delNodeList.back();
                    delNodeDir.pop_back();
                } else {
                    delNodeList.back()->m_pRight = delNodeList.back();
                    delNodeDir.pop_back();
                }
            }
        } else {
            if (currentNode->m_pRight == nullptr) {
                currentNode->m_pRight = currentNode;
            } else if (currentNode->m_pRight != currentNode) {
                delNodeList.push_back(currentNode->m_pRight);
                delNodeDir.push_back(1);
            } else {
                delNodeList.push_back(currentNode->m_pLeft);
                delNodeDir.push_back(0);
            }
        }
    } while (delNodeList.size() > 0);
}

bool RegionTree::intersect(const RegionTree &b) const {
    if (is_leaf() && b.is_leaf()) {
        if (getInternalRegion().intersects(b.getInternalRegion())) {
            return getInternalLine().intersects(b.getInternalLine());
        } else {
            return false;
        }
    } else {
        if (getInternalRegion().intersects(b.getInternalRegion())) {
            return subintersect(b);
        } else {
            return false;
        }
    }
}

bool RegionTree::subintersect(const RegionTree &b) const {
    if (left().intersect(b.left())) {
        return true;
    } else if (right().intersect(b.right())) {
        return true;
    } else if (left().intersect(b.right())) {
        return true;
    } else if (right().intersect(b.left())) {
        return true;
    }

    return false;
}

// Intersection count

int RegionTree::intersections(const Line4f &b) const {
    int n = 0;

    if (!is_leaf()) {
        if (getInternalRegion().intersects(Region4f(b))) {
            n += left().intersections(b);
            n += right().intersections(b);
        }
    } else {
        // Note: touching lines count 1, non-touching lines count 2, this allows through
        // vertex lines (where it touches both vertices)
        n += this->getInternalLine().intersects_b(b);
    }

    return n;
}
