// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

// wrapper for genlib's BSPNode for easy create/delete

#pragma once

#include "genlib/bsptree.hpp"

class BSPNodeTree {
    BSPNode *m_root = nullptr;
    bool m_built = false;

    [[maybe_unused]] unsigned _padding0 : 3 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  public:
    BSPNodeTree() : m_root(nullptr), m_built(false), _padding0(0), _padding1(0) {}
    void resetBSPtree() { m_built = false; }
    void destroy() {
        if (m_root) {
            delete m_root;
            m_root = nullptr;
        }
        m_built = false;
    }

    BSPNode *getRoot() { return m_root; }
    void makeNewRoot(bool destroyIfBuilt) {
        if (destroyIfBuilt && m_built) {
            destroy();
        }
        m_root = new BSPNode();
    }
    void setBuilt(bool built) { m_built = built; }

    ~BSPNodeTree() {
        destroy();
        m_built = false;
    }
    bool built() { return m_built; }
};
