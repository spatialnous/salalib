// Copyright (C) 2000-2010, University College London, Alasdair Turner
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2019, Petros Koutsolampros

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
