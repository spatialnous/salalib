// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "agentengine.h"

#include "agenthelpers.h"

// run one agent engine only

AgentEngine::AgentEngine() {
    m_timesteps = 5000;
    m_gatelayer = std::nullopt;
    m_record_trails = false;
}

void AgentEngine::run(Communicator *comm, PointMap *pointmap) {
    for (auto &agentSet : agentSets) {
        if (agentSet.m_sel_type == AgentProgram::SEL_LOS_OCC) {
            pointmap->requireIsovistAnalysis();
        }
    }
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_timesteps);
    }

    AttributeTable &table = pointmap->getAttributeTable();
    auto displaycol = table.getOrInsertColumn(g_col_total_counts);

    int output_mode = Agent::OUTPUT_COUNTS;
    if (m_gatelayer != std::nullopt) {
        output_mode |= Agent::OUTPUT_GATE_COUNTS;
    }

    int trail_num = -1;
    if (m_record_trails) {
        if (m_trail_count < 1) {
            m_trail_count = 1;
        }
        for (auto &agentSet : agentSets) {
            agentSet.m_trails = std::vector<std::vector<Event2f>>(m_trail_count);
        }
        trail_num = 0;
    }

    // remove any agents that are left from a previous run
    for (auto &agentSet : agentSets) {
        agentSet.agents.clear();
    }

    for (size_t i = 0; i < m_timesteps; i++) {
        for (auto &agentSet : agentSets) {
            int q = invcumpoisson(prandomr(), agentSet.m_release_rate);
            auto length = agentSet.agents.size();
            int k;
            for (k = 0; k < q; k++) {
                agentSet.agents.push_back(Agent(&(agentSet), pointmap, output_mode));
            }
            for (k = 0; k < q; k++) {
                agentSet.init(length + k, trail_num);
                if (trail_num != -1) {
                    trail_num++;
                    // after trail count, stop recording:
                    if (trail_num == m_trail_count) {
                        trail_num = -1;
                    }
                }
            }
        }

        for (auto &agentSet : agentSets) {
            agentSet.move();
        }

        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, i);
            }
        }
    }

    pointmap->overrideDisplayedAttribute(-2);
    pointmap->setDisplayedAttribute(displaycol);
}

void AgentEngine::insertTrailsInMap(ShapeMap &trailsMap) {
    for (auto &agentSet : agentSets) {
        // there is currently only one AgentSet. If at any point there are more then
        // this could be amended to put the AgentSet id as a property of the trail
        for (auto &trail : agentSet.m_trails) {
            std::vector<Point2f> trailGeometry(trail.begin(), trail.end());
            trailsMap.makePolyShape(trailGeometry, true, false);
        }
    }
}
