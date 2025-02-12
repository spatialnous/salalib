// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

enum class RadiusType {
    TOPOLOGICAL, // = 0 AKA STEPS
    METRIC,      // = 1
    ANGULAR,     // = 2
    NONE         // = -1
};
