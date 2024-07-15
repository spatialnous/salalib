// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "agentanalysis.h"

#include "salalib/agents/agent.h"

#include "salalib/pushvalues.h"

void AgentAnalysis::init(std::vector<Agent> &agents, std::vector<PixelRef> &releaseLocations,
                         size_t agent, int trail_num) {
    if (releaseLocations.size()) {
        auto which = pafrand() % releaseLocations.size();
        agents[agent].onInit(releaseLocations[which], trail_num);
    } else {
        const PointMap &map = agents[agent].getPointMap();
        PixelRef pix;
        do {
            pix = map.pickPixel(prandom(static_cast<int>(m_randomReleaseLocationsSeed.value())));
        } while (!map.getPoint(pix).filled());
        agents[agent].onInit(pix, trail_num);
    }
}

void AgentAnalysis::move(std::vector<Agent> &agents) {
    // go through backwards so remove does not affect later agents
    for (auto rev_iter = agents.rbegin(); rev_iter != agents.rend(); ++rev_iter) {
        rev_iter->onMove();
        if (rev_iter->getFrame() >= static_cast<int>(m_agentLifetime)) {
            agents.erase(std::next(rev_iter).base());
        }
    }
}

void AgentAnalysis::runAgentEngine(std::vector<Agent> &agents,
                                   std::vector<PixelRef> &releaseLocations, Communicator *comm,
                                   PointMap *pointmap) {

    if (agentProgram.m_sel_type == AgentProgram::SEL_LOS_OCC) {
        pointmap->requireIsovistAnalysis();
    }

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_systemTimesteps);
    }

    AttributeTable &table = pointmap->getAttributeTable();
    table.getOrInsertColumn(AgentAnalysis::Column::GATE_COUNTS);

    int output_mode = Agent::OUTPUT_COUNTS;
    if (m_gateLayer.has_value()) {
        output_mode |= Agent::OUTPUT_GATE_COUNTS;
    }

    int trail_num = -1;
    if (m_recordTrails.has_value()) {
        if (m_recordTrails->limit < 1) {
            m_recordTrails->limit = 1;
        }

        auto recordTrailsLimit = m_recordTrails->limit;
        if (recordTrailsLimit.has_value()) {
            agentProgram.m_trails = std::vector<std::vector<Event2f>>(*recordTrailsLimit);
        } else {
            agentProgram.m_trails = std::vector<std::vector<Event2f>>(50);
        }

        trail_num = 0;
    }

    // remove any agents that are left from a previous run

    agents.clear();

    int maxValue =
        m_recordTrails->limit.has_value() ? static_cast<int>(*m_recordTrails->limit) : -1;

    for (size_t i = 0; i < m_systemTimesteps; i++) {
        auto q = static_cast<size_t>(invcumpoisson(prandomr(), m_releaseRate));
        auto length = agents.size();
        size_t k;
        for (k = 0; k < q; k++) {
            agents.push_back(Agent(&(agentProgram), pointmap, output_mode));
        }
        for (k = 0; k < q; k++) {
            init(agents, releaseLocations, length + k, trail_num);
            if (trail_num != -1) {
                trail_num++;
                // after trail count, stop recording:
                if (maxValue > 0 && trail_num == maxValue) {
                    trail_num = -1;
                }
            }
        }

        move(agents);

        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, i);
            }
        }
    }
}

void AgentAnalysis::insertTrailsInMap(ShapeMap &trailsMap) {
    // there is currently only one AgentSet. If at any point there are more then
    // this could be amended to put the AgentSet id as a property of the trail
    for (auto &trail : agentProgram.m_trails) {
        std::vector<Point2f> trailGeometry(trail.begin(), trail.end());
        trailsMap.makePolyShape(trailGeometry, true, false);
    }
}

AnalysisResult AgentAnalysis::run(Communicator *comm) {
    AttributeTable &table = m_pointMap.getAttributeTable();

    if (m_agentFOV == 32) {
        agentProgram.m_vbin = -1;
    } else {
        agentProgram.m_vbin = (m_agentFOV - 1) / 2;
    }

    agentProgram.m_steps = m_agentStepsToDecision;
    agentProgram.m_sel_type = m_agentAlgorithm;

    std::vector<PixelRef> releaseLocations;

    // if the m_release_locations is not set the locations are
    // set later by picking random pixels
    auto &map = m_pointMap;
    if (!m_randomReleaseLocationsSeed.has_value()) {
        releaseLocations.clear();
        for_each(m_specificReleasePoints.begin(), m_specificReleasePoints.end(),
                 [&releaseLocations, &map](const Point2f &point) -> void {
                     releaseLocations.push_back(map.pixelate(point, false));
                 });
    }

    std::vector<Agent> agents;

    if (m_gateLayer.has_value()) {
        // switch the reference numbers from the gates layer to the vga layer
        table.insertOrResetColumn(AgentAnalysis::Column::INTERNAL_GATE);
        // Transferring refs here, so we need to get the column name of the "Ref" column
        const std::string &colIn = m_gateLayer->get().getAttributeTable().getColumnName(-1);
        PushValues::shapeToPoint(m_gateLayer->get(), colIn, m_pointMap,
                                 AgentAnalysis::Column::INTERNAL_GATE, PushValues::Func::TOT);

        table.insertOrResetColumn(Column::INTERNAL_GATE_COUNTS);
    }

    runAgentEngine(agents, releaseLocations, comm, &m_pointMap);

    if (m_recordTrails.has_value()) {
        std::string mapName = "Agent Trails";
        insertTrailsInMap(m_recordTrails->map);
    }

    if (m_gateLayer.has_value()) {
        // switch column counts from vga layer to gates layer...
        auto colcounts = table.getColumnIndex(Column::INTERNAL_GATE_COUNTS);
        AttributeTable &tableout = m_gateLayer->get().getAttributeTable();
        tableout.insertOrResetColumn(Column::AGENT_COUNTS);
        PushValues::pointToShape(m_pointMap, Column::INTERNAL_GATE_COUNTS, *m_gateLayer,
                                 Column::AGENT_COUNTS, PushValues::Func::TOT);

        // and delete the temporary columns:
        table.removeColumn(colcounts);
        auto colgates = table.getColumnIndex(AgentAnalysis::Column::INTERNAL_GATE);
        table.removeColumn(colgates);
    }
    return AnalysisResult();
}
