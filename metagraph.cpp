// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "metagraph.h"

#include "genlib/p2dpoly.h"

#include <math.h>
#include <sstream>
#include <tuple>

// From: Alasdair Turner (2004) - Depthmap 4: a researcher's handbook (p. 6):
// [..] CAT, which stands for Chiron and Alasdair Transfer Format [..]

// void MetaGraph::updateParentRegions(ShapeMap &shapeMap) {
//     if (m_drawingFiles.back().m_region.atZero()) {
//         m_drawingFiles.back().m_region = shapeMap.getRegion();
//     } else {
//         m_drawingFiles.back().m_region =
//             runion(m_drawingFiles.back().m_region, shapeMap.getRegion());
//     }
//     if (m_region.atZero()) {
//         m_region = m_drawingFiles.back().m_region;
//     } else {
//         m_region = runion(m_region, m_drawingFiles.back().m_region);
//     }
// }
