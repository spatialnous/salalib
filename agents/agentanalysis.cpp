// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "agentanalysis.h"

#include "salalib/agents/agent.h"

#include "salalib/pushvalues.h"

AnalysisResult AgentAnalysis::run(Communicator *comm) {
    if (comm) {
        m_pointMap.clearAllPoints();
    }
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
        insertTrailsInMap(m_recordTrails->second);
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
