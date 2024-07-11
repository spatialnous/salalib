// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "comm.h"
#include "p2dpoly.h"

#include <memory>

// Binary Space Partition

struct BSPNode {

  private:
    Line m_line;
    int m_tag;

  public:
    enum { BSPLEFT, BSPRIGHT };
    std::unique_ptr<BSPNode> m_left;
    std::unique_ptr<BSPNode> m_right;
    BSPNode *m_parent;

    BSPNode(BSPNode *parent = NULL) : m_left(nullptr), m_right(nullptr) {
        m_parent = parent;
        m_tag = -1;
    }
    bool isLeaf() { return m_left == nullptr && m_right == nullptr; }
    int classify(const Point2f &p) {
        Point2f v0 = m_line.end() - m_line.start();
        v0.normalise();
        Point2f v1 = p - m_line.start();
        v1.normalise();
        if (det(v0, v1) >= 0) {
            return BSPLEFT;
        } else {
            return BSPRIGHT;
        }
    }
    const Line &getLine() const { return m_line; }
    void setLine(const Line &line) { m_line = line; }
    int getTag() const { return m_tag; }
    void setTag(const int tag) { m_tag = tag; }
};

namespace BSPTree {
    void make(Communicator *communicator, time_t atime, const std::vector<Line> &lines,
              BSPNode *root);
    int pickMidpointLine(const std::vector<Line> &lines, BSPNode *par);
    std::pair<std::vector<Line>, std::vector<Line>> makeLines(Communicator *communicator,
                                                              time_t atime,
                                                              const std::vector<Line> &lines,
                                                              BSPNode *base);
} // namespace BSPTree
