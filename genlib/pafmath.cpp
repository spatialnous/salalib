// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

// a collection of math functions

#include "pafmath.hpp"

#include <cmath>
#include <inttypes.h>

///////////////////////////////////////////////////////////////////////////////

double pafmath::poisson(int x, double lambda) {
    double f = exp(-lambda);
    for (int i = 1; i <= x; i++) {
        f *= lambda / static_cast<double>(i);
    }
    return f;
}

double pafmath::cumpoisson(int x, double lambda) {
    double f = exp(-lambda);
    double c = f;
    for (int i = 1; i <= x; i++) {
        f *= lambda / static_cast<double>(i);
        c += f;
    }
    return c;
}

int pafmath::invcumpoisson(double p, double lambda) {
    if (p <= 0) {
        return 0;
    }
    if (p >= 1) {
        // passing this 1 will cause an infinite loop, try this instead:
        p = 1 - 1e-9;
    }
    double f = exp(-lambda);
    double c = f;
    int i = 0;
    for (; c < p; i++) {
        f *= lambda / static_cast<double>(i + 1);
        c += f;
    }
    return i;
}
