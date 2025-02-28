// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "comm.hpp"
#include "line4f.hpp"

#include <memory>

// Binary Space Partition

struct BSPNode {

  private:
    Line4f m_line;
    int m_tag = -1;

    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    enum { BSPLEFT, BSPRIGHT };
    std::unique_ptr<BSPNode> left;
    std::unique_ptr<BSPNode> right;
    BSPNode *parent;

    BSPNode(BSPNode *p = nullptr)
        : m_line(), m_tag(-1), _padding0(0), left(nullptr), right(nullptr), parent(p) {}
    bool isLeaf() { return left == nullptr && right == nullptr; }
    BSPNode(const BSPNode &) = delete;
    BSPNode(BSPNode &&) = default;
    BSPNode &operator=(BSPNode &) = delete;
    int classify(const Point2f &p) {
        Point2f v0 = m_line.end() - m_line.start();
        v0.normalise();
        Point2f v1 = p - m_line.start();
        v1.normalise();
        if (v0.det(v1) >= 0) {
            return BSPLEFT;
        } else {
            return BSPRIGHT;
        }
    }
    const Line4f &getLine() const { return m_line; }
    void setLine(const Line4f &line) { m_line = line; }
    int getTag() const { return m_tag; }
    void setTag(const int tag) { m_tag = tag; }
};

namespace BSPTree {
    void make(Communicator *communicator, time_t atime, const std::vector<Line4f> &lines,
              BSPNode *root);
    int pickMidpointLine(const std::vector<Line4f> &lines, BSPNode *par);
    std::pair<std::vector<Line4f>, std::vector<Line4f>> makeLines(Communicator *communicator,
                                                                  time_t atime,
                                                                  const std::vector<Line4f> &lines,
                                                                  BSPNode *base);
} // namespace BSPTree
