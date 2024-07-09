// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

enum class AnalysisType {
    ISOVIST,          // = 0 also sometimes "topological"
    VISUAL,           // = 1
    METRIC,           // = 2
    ANGULAR,          // = 3
    THRU_VISION,      // = 4
    CLIQUE_GRAPH,     // = 5
    KERNEL_GRAPH,     // = 6
    MATRIX_REDUCTION, // = 7
    NONE              // = -1
};
