// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "agentprogram.h"

#include "salalib/agents/agent.h"
#include "salalib/ianalysis.h"
#include "salalib/pointmap.h"

class AgentAnalysis : public IAnalysis {

    PointMap &m_pointMap;

    AgentProgram agentProgram;

    size_t m_systemTimesteps;
    double m_releaseRate = 0.1;
    size_t m_agentLifetime = 5000;
    unsigned short m_agentFOV = 15;
    size_t m_agentStepsToDecision = 3;
    int m_agentAlgorithm = AgentProgram::SEL_STANDARD;
    std::optional<size_t> m_randomReleaseLocationsSeed = 0;
    const std::vector<Point2f> &m_specificReleasePoints;

    const std::optional<std::reference_wrapper<ShapeMap>> m_gateLayer = std::nullopt;

    std::optional<std::pair<size_t, std::reference_wrapper<ShapeMap>>> m_recordTrails;

    void init(std::vector<Agent> &agents, std::vector<PixelRef> &releaseLocations, size_t agent,
              int trail_num) {
        if (releaseLocations.size()) {
            auto which = pafrand() % releaseLocations.size();
            agents[agent].onInit(releaseLocations[which], trail_num);
        } else {
            const PointMap &map = agents[agent].getPointMap();
            PixelRef pix;
            do {
                pix = map.pickPixel(prandom(static_cast<int>(*m_randomReleaseLocationsSeed)));
            } while (!map.getPoint(pix).filled());
            agents[agent].onInit(pix, trail_num);
        }
    }

    void move(std::vector<Agent> &agents) {
        // go through backwards so remove does not affect later agents
        for (auto rev_iter = agents.rbegin(); rev_iter != agents.rend(); ++rev_iter) {
            rev_iter->onMove();
            if (rev_iter->getFrame() >= static_cast<int>(m_agentLifetime)) {
                agents.erase(std::next(rev_iter).base());
            }
        }
    }

    void runAgentEngine(std::vector<Agent> &agents, std::vector<PixelRef> &releaseLocations,
                        Communicator *comm, PointMap *pointmap) {

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
            if (m_recordTrails->first < 1) {
                m_recordTrails->first = 1;
            }

            agentProgram.m_trails = std::vector<std::vector<Event2f>>(m_recordTrails->first);

            trail_num = 0;
        }

        // remove any agents that are left from a previous run

        agents.clear();

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
                    if (trail_num == static_cast<int>(m_recordTrails->first)) {
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

    void insertTrailsInMap(ShapeMap &trailsMap) {
        // there is currently only one AgentSet. If at any point there are more then
        // this could be amended to put the AgentSet id as a property of the trail
        for (auto &trail : agentProgram.m_trails) {
            std::vector<Point2f> trailGeometry(trail.begin(), trail.end());
            trailsMap.makePolyShape(trailGeometry, true, false);
        }
    }

  public:
    struct Column {
        inline static const std::string                      // Originally:
            GATE_COUNTS = "Gate Counts",                     // g_col_total_counts
            INTERNAL_GATE_COUNTS = "__Internal_Gate_Counts", // g_col_gate_counts
            INTERNAL_GATE = "__Internal_Gate",               // g_col_gate
            AGENT_COUNTS = "Agent Counts";                   // Goes into the gateLayer
    };

  public:
    AgentAnalysis(PointMap &pointMap, size_t systemTimesteps, double releaseRate,
                  size_t agentLifetime, unsigned short agentFOV, size_t agentStepsToDecision,
                  int agentAlgorithm, int randomReleaseLocationsSeed,
                  const std::vector<Point2f> &specificReleasePoints,
                  const std::optional<std::reference_wrapper<ShapeMap>> &gateLayer,
                  std::optional<std::pair<size_t, std::reference_wrapper<ShapeMap>>> recordTrails)
        : m_pointMap(pointMap), m_systemTimesteps(systemTimesteps), m_releaseRate(releaseRate),
          m_agentLifetime(agentLifetime), m_agentFOV(agentFOV),
          m_agentStepsToDecision(agentStepsToDecision), m_agentAlgorithm(agentAlgorithm),
          m_randomReleaseLocationsSeed(randomReleaseLocationsSeed),
          m_specificReleasePoints(specificReleasePoints), m_gateLayer(gateLayer),
          m_recordTrails(recordTrails) {}
    std::string getAnalysisName() const override { return "Agent Analysis"; }
    AnalysisResult run(Communicator *comm) override;

  public: // utility functions
    bool setTooRecordTrails() { return m_recordTrails.has_value(); }
};
