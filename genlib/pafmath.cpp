// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// a collection of math functions

#include "pafmath.h"

#include <inttypes.h>

namespace {
    uint64_t g_rand[11] = {1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29};

    // 25-Jul-2007: changed the g_mult and g_const used for random number generation
    // for some reason, there appeared to be a pattern to the numbers

    // Quick mod - TV
    const uint64_t g_mult = /*(0xF9561B2E << 32) + */ 0x71A7FA85;
    const uint64_t g_const = /*(0x9BB3920E << 32) + */ 0xF5E958B9;

} // namespace

void pafmath::pafsrand(unsigned int seed, int set) // = 0
{
    g_rand[set] = seed;
}

// Pafrand is a Linear Congruential Generator
// After the 25-Jul-2007 changes:
// The current version seems to meet standard randomness conditions
// Tested using Diehard, the 32 bit version ((g_rand[set] >> 32) & 0xffffffff)
// passes all tests for at least the first 5 seeds above
// it is also independent in at least 20 dimensions
// It should not be used for "serious" randomness, but should be fine
// for most things (agents in depthmapX, genetic algorithms, etc)

// 25-Jul-2007: moved up to take top 32 bits

unsigned int pafmath::pafrand(int set) // = 0
{
    g_rand[set] = g_mult * g_rand[set] + g_const;

    return (unsigned int)((g_rand[set] >> 32) & pafmath::PAF_RAND_MAX);
}

///////////////////////////////////////////////////////////////////////////////

double pafmath::poisson(int x, double lambda) {
    double f = exp(-lambda);
    for (int i = 1; i <= x; i++) {
        f *= lambda / double(i);
    }
    return f;
}

double pafmath::cumpoisson(int x, double lambda) {
    double f = exp(-lambda);
    double c = f;
    for (int i = 1; i <= x; i++) {
        f *= lambda / double(i);
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
        f *= lambda / double(i + 1);
        c += f;
    }
    return i;
}
