// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "agentset.h"

#include <optional>

class AgentEngine {
  public: // public for now for speed
    std::vector<AgentSet> agentSets;
    std::optional<size_t> m_gatelayer = std::nullopt;
    size_t m_timesteps;

  public:
    bool m_record_trails;
    int m_trail_count = 50;

  public:
    AgentEngine();
    void run(Communicator *comm, PointMap *pointmap);
    void insertTrailsInMap(ShapeMap &trailsMap);
};
