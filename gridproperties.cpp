// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gridproperties.hpp"

#include <cmath>

GridProperties::GridProperties(double maxDimension) : m_max(), m_min(), m_default() {
    int maxexponent = static_cast<int>(floor(log10(maxDimension)) - 1);
    int minexponent = maxexponent - 2;
    int mantissa =
        static_cast<int>(floor(maxDimension / pow(10.0, static_cast<double>(maxexponent + 1))));

    m_default = static_cast<double>(mantissa) * pow(10.0, static_cast<double>(maxexponent - 1));
    m_max = 2.0 * mantissa * pow(10.0, static_cast<double>(maxexponent));
    m_min = static_cast<double>(mantissa) * pow(10.0, static_cast<double>(minexponent));
}
