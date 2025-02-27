// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salashape.hpp"

struct SalaEvent {
    enum { SALA_NULL_EVENT, SALA_CREATED, SALA_DELETED, SALA_MOVED };
    int action;
    int shapeRef;
    SalaShape geometry;
    SalaEvent(int actionIn = SALA_NULL_EVENT, int shapeRefIn = -1)
        : action(actionIn), shapeRef(shapeRefIn), geometry() {}
};
