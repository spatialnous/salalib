// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "agentprogram.hpp"

#include "../ianalysis.hpp"
#include "../pointmap.hpp"
#include "agent.hpp"
#include <cstring>

class AgentAnalysis : public IAnalysis {

  public: // publicly accessible struct
    struct TrailRecordOptions {
        std::optional<size_t> limit = std::nullopt;
        std::reference_wrapper<ShapeMap> map;
    };

  private: // members
    PointMap &m_pointMap;
    AgentProgram m_agentProgram;

    size_t m_systemTimesteps;
    double m_releaseRate = 0.1;
    size_t m_agentLifetime = 5000;
    int m_agentAlgorithm = AgentProgram::SEL_STANDARD;
    unsigned short m_agentFOV = 15;

    [[maybe_unused]] unsigned _padding0 : 2 * 8; // padding

    size_t m_agentStepsToDecision = 3;
    std::optional<size_t> m_randomReleaseLocationsSeed = 0;
    const std::vector<Point2f> &m_specificReleasePoints;

    const std::optional<std::reference_wrapper<ShapeMap>> m_gateLayer = std::nullopt;

    std::optional<TrailRecordOptions> m_recordTrails;

  private: // internal functions
    void init(std::vector<Agent> &agents, std::vector<PixelRef> &releaseLocations, size_t agent,
              int trailNum);

    void move(std::vector<Agent> &agents);

    void runAgentEngine(std::vector<Agent> &agents, std::vector<PixelRef> &releaseLocations,
                        Communicator *comm, PointMap *pointmap);

    void insertTrailsInMap(ShapeMap &trailsMap);

  public: // column data
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
                  int agentAlgorithm, std::optional<size_t> randomReleaseLocationsSeed,
                  const std::vector<Point2f> &specificReleasePoints,
                  const std::optional<std::reference_wrapper<ShapeMap>> &gateLayer,
                  std::optional<TrailRecordOptions> recordTrails)
        : m_pointMap(pointMap), m_agentProgram(), m_systemTimesteps(systemTimesteps),
          m_releaseRate(releaseRate), m_agentLifetime(agentLifetime),
          m_agentAlgorithm(agentAlgorithm), m_agentFOV(agentFOV), _padding0(0),
          m_agentStepsToDecision(agentStepsToDecision),
          m_randomReleaseLocationsSeed(randomReleaseLocationsSeed),
          m_specificReleasePoints(specificReleasePoints), m_gateLayer(gateLayer),
          m_recordTrails(recordTrails) {}
    std::string getAnalysisName() const override { return "Agent Analysis"; }
    AnalysisResult run(Communicator *comm) override;

  public: // utility functions
    bool setTooRecordTrails() { return m_recordTrails.has_value(); }
};
