// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/shapemap.h"

#include <ostream>

namespace exportUtils {
    void writeMapShapesAsCat(ShapeMap &map, std::ostream &stream);
} // namespace exportUtils
