// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "agentprogram.hpp"

#include "../pixelref.hpp"
#include "../pointmap.hpp"

#include "../genlib/pflipper.hpp"

class Agent {
  public:
    enum {
        OUTPUT_NOTHING = 0x00,
        OUTPUT_COUNTS = 0x01,
        OUTPUT_GATE_COUNTS = 0x02,
        OUTPUT_TRAILS = 0x04
    };

  protected:
    AgentProgram *m_program;
    PointMap *m_pointmap;
    //
    PixelRef m_node;
    int m_step = 0;
    int m_frame = 0;
    int m_gate = -1;
    int m_outputMode = OUTPUT_NOTHING;
    // for recording trails:
    int m_trailNum = -1;
    Point2f m_loc;
    Point2f m_target;
    Point2f m_vector;
    // a long term goal:
    Point2f m_destination;
    PixelRef m_targetPix;
    // extra memory of last observed values for Gibsonian agents:
    float m_lastLos[9];
    float m_currLos[9];
    bool m_stuck = false;
    bool m_stopped = false;
    bool m_targetLock = false;
    bool m_gateEncountered = false;
    bool m_atTarget = false;
    // a long term goal:
    bool m_atDestination = false;

  private:
    [[maybe_unused]] unsigned _padding0 : 2 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

    // for occlusion memory
    pflipper<PixelRefVector> m_occMemory;

  public:
    Agent()
        : m_program(nullptr), m_pointmap(nullptr), m_node(), m_outputMode(OUTPUT_NOTHING), m_loc(),
          m_target(), m_vector(), m_destination(), m_targetPix(), _padding0(0), _padding1(0),
          m_occMemory() {}
    Agent(AgentProgram *program, PointMap *pointmap, int outputMode = OUTPUT_NOTHING);
    Agent(const Agent &) = default;
    Agent &operator=(const Agent &) = default;
    void onInit(PixelRef node, int trailNum = -1);
    void onClose();
    Point2f onLook(bool wholeisovist);
    Point2f onStandardLook(bool wholeisovist);
    Point2f onWeightedLook(bool wholeisovist);
    Point2f onOcclusionLook(bool wholeisovist, int looktype);
    Point2f onLoSLook(bool wholeisovist, int lookType);
    Point2f onDirectedLoSLook(bool wholeisovist, int lookType);
    Point2f onGibsonianLook(bool wholeisovist);
    Point2f onGibsonianLook2(bool wholeisovist);
    int onGibsonianRule(int rule);
    void calcLoS(int directionbin, bool curr);
    void calcLoS2(int directionbin, bool curr);
    void onMove();
    void onTarget();
    void onDestination();
    void onStep();
    bool diagonalStep();
    bool goodStep(PixelRef node);
    bool gateEncountered() { return m_gateEncountered; }
    const Point2f &getLoc() const { return m_loc; }
    //
    bool atTarget() const { return m_atTarget; }
    bool atDestination() const { return m_atDestination; }
    //
    const Point2f &getLocation() const { return m_loc; }
    const Point2f &getVector() const { return m_vector; }
    const PixelRef getNode() const { return m_node; }
    int getFrame() const { return m_frame; }
    const PointMap &getPointMap() const { return *m_pointmap; }
};

// note the add 0.5 means angles from e.g., -1/32 to 1/32 are in bin 0
inline int binfromvec(const Point2f &p) {
    return static_cast<int>(32.0 * (0.5 * p.angle() / M_PI) + 0.5);
}

// a random angle based on a bin direction
inline double anglefrombin2(int here) {
    return (2.0 * M_PI) * ((static_cast<double>(here) - 0.5) / 32.0 + pafmath::prandom() / 32.0);
}

inline int binsbetween(int bin1, int bin2) {
    int b = abs(bin1 - bin2);
    if (b > 16) {
        b = 32 - b;
    }
    return b;
}

// weighting
struct wpair {
    double weight;
    int node;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    wpair(double w = 0.0, int n = -1) : weight(w), node(n), _padding0(0) {}
};

// convert an x / y difference to it's corresponding connection direction
inline int8_t connectValue(PixelRef dir) {
    if (dir.y > 0) {
        return static_cast<int8_t>(Point::CONNECT_NE << (1 - dir.x));
    } else if (dir.y < 0) {
        return static_cast<int8_t>(Point::CONNECT_SW << (dir.x + 1));
    } else if (dir.x == 1) {
        return Point::CONNECT_E;
    } else {
        return Point::CONNECT_W;
    }
}
