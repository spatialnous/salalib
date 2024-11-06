// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "shapemap.h"

#include <ostream>

namespace exportUtils {

    // From: Alasdair Turner (2004) - Depthmap 4: a researcher's handbook (p. 6):
    // [..] CAT, which stands for Chiron and Alasdair Transfer Format [..]
    void writeMapShapesAsCat(ShapeMap &map, std::ostream &stream);

} // namespace exportUtils
