// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "agentga.h"

#include "genlib/pafmath.h"

int thisrun = 0;

///////////////////////////////////////////////////////////////////////////////////////////////

static int rankselect(int popsize) {
    int num = int(prandom() * popsize * (popsize + 1) * 0.5);
    for (int i = 0; i < popsize; i++) {
        if (num < (popsize - i)) {
            return i;
        }
        num -= (popsize - i);
    }
    return 0; // <- this shouldn't happen
}

// note: this is tested and right: higher fitness, lower rank (so population[0] is best)
int progcompare(const void *a, const void *b) {
    double test = (((AgentProgram *)a)->m_fitness - ((AgentProgram *)b)->m_fitness);
    if (test < 0.0) {
        return 1;
    } else if (test > 0.0) {
        return -1;
    }
    return 0;
}

AgentProgram *ProgramPopulation::makeChild() {
    int a = rankselect(POPSIZE);
    int b = rankselect(POPSIZE);
    while (a == b)
        b = rankselect(POPSIZE);
    m_population[POPSIZE - 1] = crossover(m_population[a], m_population[b]);
    m_population[POPSIZE - 1].mutate();

    return &(m_population[POPSIZE - 1]);
}

// note: this is correct -- do not use &m_population!
void ProgramPopulation::sort() { qsort(m_population, POPSIZE, sizeof(AgentProgram), progcompare); }
