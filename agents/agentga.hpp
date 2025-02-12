// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "agentprogram.hpp"

const int POPSIZE = 500;
// redo ASSAYs -- assaysize * assays (3 * 200 = 600 evaluations total)
// then take mean fitness: due to large variation in fitnesses with
// short assays such as this
const int ASSAYS = 3;
const int ASSAYSIZE = 25000;
const int GENERATIONS = 10000;
const int TIMESTEPS = 1600;

struct ProgramPopulation {
  public:
    AgentProgram population[POPSIZE];

  public:
    ProgramPopulation() {}
    AgentProgram *makeChild();
    void sort();
};
