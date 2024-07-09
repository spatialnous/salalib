// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "agentanalysis.h"

#include "salalib/agents/agentengine.h"
#include "salalib/pushvalues.h"

AnalysisResult AgentAnalysis::run(Communicator *comm, PointMap &map, bool) {
    if (comm) {
        map.clearAllPoints();
    }
    //    AttributeTable &table = map.getAttributeTable();

    //    AgentEngine agent_engine;

    //    if (agent_engine.m_gatelayer.has_value()) {
    //        // switch the reference numbers from the gates layer to the vga layer
    //        auto colgates = table.insertOrResetColumn(AgentAnalysis::Column::INTERNAL_GATE);
    //        PushValues::shapeToPointMap(m_agent_engine.m_gatelayer, );
    //        pushValuesToLayer(VIEWDATA, m_agent_engine.m_gatelayer.value(), //
    //                          VIEWVGA, getDisplayedPointMapRef(),           //
    //                          std::nullopt, colgates, PUSH_FUNC_TOT);
    //        table.insertOrResetColumn(Column::INTERNAL_GATE_COUNTS);
    //    }

    //    agent_engine.run(comm, map);

    //    if (agent_engine.m_record_trails) {
    //        std::string mapName = "Agent Trails";
    //        int count = 1;
    //        while (std::find_if(std::begin(m_dataMaps), std::end(m_dataMaps), [&](ShapeMap const
    //        &m) {
    //                   return m.getName() == mapName;
    //               }) != m_dataMaps.end()) {
    //            mapName = "Agent Trails " + std::to_string(count);
    //            count++;
    //        }
    //        m_dataMaps.emplace_back(mapName);
    //        agent_engine.insertTrailsInMap(m_dataMaps.back());

    //        m_state |= DATAMAPS;
    //    }

    //    if (agent_engine.m_gatelayer.has_value()) {
    //        // switch column counts from vga layer to gates layer...
    //        auto colcounts = table.getColumnIndex(Column::INTERNAL_GATE_COUNTS);
    //        AttributeTable &tableout =
    //            m_dataMaps[m_agent_engine.m_gatelayer.value()].getAttributeTable();
    //        auto targetcol = tableout.insertOrResetColumn("Agent Counts");
    //        pushValuesToLayer(VIEWVGA, getDisplayedPointMapRef(),           //
    //                          VIEWDATA, m_agent_engine.m_gatelayer.value(), //
    //                          colcounts, targetcol, PUSH_FUNC_TOT);
    //        // and delete the temporary columns:
    //        table.removeColumn(colcounts);
    //        auto colgates = table.getColumnIndex(AgentAnalysis::Column::INTERNAL_GATE);
    //        table.removeColumn(colgates);
    //    }
    //    pointmap->overrideDisplayedAttribute(-2);
    //    auto displaycol = table.getOrInsertColumn(AgentAnalysis::Column::GATE_COUNTS);
    //    pointmap->setDisplayedAttribute(displaycol);
    return AnalysisResult();
}
