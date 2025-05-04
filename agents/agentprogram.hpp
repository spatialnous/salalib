// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../genlib/event2f.hpp"

#include <string>
#include <vector>

struct AgentProgram {
    // comparative is comparative with current heading
    enum {
        SEL_LOS = 0x0001,
        SEL_LOS_OCC = 0x0002,
        SEL_TARGETTED = 0x1000,
        SEL_STANDARD = 0x1001,
        SEL_WEIGHTED = 0x1002,
        SEL_GIBSONIAN = 0x2000,
        SEL_LENGTH = 0x2001,
        SEL_OPTIC_FLOW = 0x2002,
        SEL_COMPARATIVE_LENGTH = 0x2003,
        SEL_COMPARATIVE_OPTIC_FLOW = 0x2004,
        SEL_GIBSONIAN2 = 0x4000,
        SEL_OPTIC_FLOW2 = 0x4001,
        SEL_OCCLUSION = 0x9000,
        SEL_OCC_ALL = 0x9001,
        SEL_OCC_BIN45 = 0x9002,
        SEL_OCC_BIN60 = 0x9003,
        SEL_OCC_STANDARD = 0x9004,
        SEL_OCC_WEIGHT_DIST = 0x9005,
        SEL_OCC_WEIGHT_ANG = 0x9006,
        SEL_OCC_WEIGHT_DIST_ANG = 0x9007,
        SEL_OCC_MEMORY = 0x9008
    };
    int selType;
    int steps;
    int vbin;
    // these three variables for evolved Gibsonian agents:
    int ruleOrder[4];
    float ruleThreshold[4];
    float ruleProbability[4];
    // these are for optic flow 2 agents
    int vahead;              // how wide your ahead vision is
    float aheadThreshold;    // will turn if neg flow greater than this threshold (set in range
                             // 1/100 to 1)
    float feelerThreshold;   // will turn if flow greater than this threshold (set in range 1 to 5)
    float feelerProbability; // turn with this much probability if a feeler triggers
    //
    // simple long range destinations:
    bool destinationDirected;
    bool losSqrd;

  private:
    [[maybe_unused]] unsigned _padding0 : 2 * 8;

  public:
    // if it is going to evolved, then have it remember its fitness:
    double fitness;
    //
    AgentProgram();
    //
    // for evolution
    void mutate();
    friend AgentProgram crossover(const AgentProgram &progA, const AgentProgram &progB);
    // to reload later:
    void save(const std::string &filename);
    bool open(const std::string &filename);
    std::vector<std::vector<Event2f>> trails;
};
