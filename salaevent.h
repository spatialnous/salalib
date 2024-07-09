// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salashape.h"

struct SalaEvent {
    enum { SALA_NULL_EVENT, SALA_CREATED, SALA_DELETED, SALA_MOVED };
    int m_action;
    int m_shape_ref;
    SalaShape m_geometry;
    SalaEvent(int action = SALA_NULL_EVENT, int shape_ref = -1) {
        m_action = action;
        m_shape_ref = shape_ref;
    }
};
