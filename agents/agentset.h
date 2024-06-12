// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "agent.h"
#include "agentprogram.h"

struct AgentSet : public AgentProgram {
    std::vector<Agent> agents;
    std::vector<int> m_release_locations;
    int m_release_locations_seed = 0;
    double m_release_rate;
    int m_lifetime;
    AgentSet();
    void move();
    void init(int agent, int trail_num = -1);
};
