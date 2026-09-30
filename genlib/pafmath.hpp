// SPDX-FileCopyrightText: 1996-2011 Alasdair Turner (a.turner@ucl.ac.uk)
// SPDX-FileCopyrightText: 2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Paf Template Library --- a set of useful C++ templates

// a collection of math functions

#pragma once

#include <cmath>
#include <cstdint>

namespace pafmath {
    constexpr double M_ROOT_1_2 = 0.70710678118654752440084436210485;
    constexpr double M_1_LN2 = 1.4426950408889634073599246810019;

    // note, in order to stop confusing myself I have ln defined:
    inline double ln(double x) { return std::log(x); }

    inline double sqr(double a) { return (a * a); }

    inline int sgn(double a) { return (a < 0) ? -1 : 1; }

    const unsigned int PAF_RAND_MAX = 0x0FFFFFFF;

    inline double plog2(double a) { return (pafmath::ln(a) * M_1_LN2); }

    // Hillier Hanson dvalue
    /*
    inline double dvalue(double k)
    {
       return 2.0 * (3.3231 * k * log10(k+2) - 2.5863 * k + 1.0) / ((k - 1.0) * (k - 2.0));
    }
    */

    // Hillier Hanson dvalue (from Kruger 1989 -- see Teklenburg et al)
    inline double dvalue(double k) {
        return 2.0 * (k * (pafmath::plog2((k + 2.0) / 3.0) - 1.0) + 1.0) / ((k - 1.0) * (k - 2.0));
    }

    // Hillier Hanson pvalue
    inline double pvalue(double k) {
        return 2.0 * (k - pafmath::plog2(k) - 1.0) / ((k - 1.0) * (k - 2.0));
    }

    // Teklenburg integration (correction 31.01.11 due to Ulrich Thaler
    inline double teklinteg(double nodecount, double totaldepth) {
        return pafmath::ln(0.5 * (nodecount - 2.0)) /
               pafmath::ln(static_cast<double>(totaldepth - nodecount + 1));
    }

    // Penn palmtree
    // Maximum total depth for a rooted graph with n nodes and maximum
    // shortest-path depth r. A radius above n - 1 saturates at the
    // path-graph maximum.
    inline double palmtree(double n, double r) {
        if (n <= 1.0 || r <= 0.0) {
            return 0.0;
        }
        const double radius = r < n - 1.0 ? r : n - 1.0;
        return radius * (n - 0.5 * (radius + 1.0));
    }

    double poisson(int x, double lambda);
    double cumpoisson(int x, double lambda);
    int invcumpoisson(double p, double lambda);

    // Pafrand is a Linear Congruential Generator
    // Each instance owns its own sequence, so
    // one analysis consuming numbers cannot shift another analysis's results.
    // After the 25-Jul-2007 changes:
    // The current version seems to meet standard randomness conditions
    // Tested using Diehard, the 32 bit version ((g_rand[set] >> 32) & 0xffffffff)
    // passes all tests for at least the first 5 seeds above
    // it is also independent in at least 20 dimensions
    // It should not be used for "serious" randomness, but should be fine
    // for most things (agents in sala, genetic algorithms, etc)
    // 25-Jul-2007: moved up to take top 32 bits
    class Pafrand {
        // 25-Jul-2007: changed the g_mult and g_const used for random number generation
        // for some reason, there appeared to be a pattern to the numbers

        static constexpr uint64_t S_MULT = /*(0xF9561B2E << 32) + */ 0x71A7FA85;
        static constexpr uint64_t S_CONST = /*(0x9BB3920E << 32) + */ 0xF5E958B9;
        uint64_t m_state;

      public:
        explicit Pafrand(unsigned int seed = 1) : m_state(seed) {}

        unsigned int next() {
            m_state = S_MULT * m_state + S_CONST;
            return static_cast<unsigned int>((m_state >> 32) & PAF_RAND_MAX);
        }
        // a random number from 0 to 1
        double prandom() { return static_cast<double>(next()) / static_cast<double>(PAF_RAND_MAX); }
        // a random number from 0 to just less than 1
        double prandomr() {
            return static_cast<double>(next()) / static_cast<double>(PAF_RAND_MAX + 1);
        }
    };

    // Default seed. This is set to 1 instad of 0 to match the original set of
    // streams possible for g_rand: 1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29. It relies
    // on the fact that almost no analysis ever called another seed instead relying
    // on the default set = 0 thus g_rand[0] thus seed = 1
    constexpr unsigned int defaultSeed = 1;

} // namespace pafmath
