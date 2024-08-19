// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

// wrapper for genlib's BSPNode for easy create/delete

#pragma once

#include "genlib/bsptree.h"

class BSPNodeTree {
    BSPNode *m_root = nullptr;
    bool m_built = false;

  public:
    BSPNodeTree()
        : m_root(nullptr), m_built(false){

                           };
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
