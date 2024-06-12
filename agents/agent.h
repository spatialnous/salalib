// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "agentprogram.h"

#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

#include "genlib/p2dpoly.h"
#include "genlib/pflipper.h"

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
    bool m_stuck = false;
    bool m_stopped = false;
    bool m_target_lock = false;
    bool m_gate_encountered = false;
    bool m_at_target = false;
    bool m_at_destination = false;
    int m_output_mode = OUTPUT_NOTHING;
    Point2f m_loc;
    Point2f m_target;
    Point2f m_vector;
    PixelRef m_target_pix;
    // a long term goal:
    Point2f m_destination;
    //
    // for recording trails:
    int m_trail_num = -1;
    //
    // for occlusion memory
    pflipper<PixelRefVector> m_occ_memory;
    //
    // extra memory of last observed values for Gibsonian agents:
    float m_last_los[9];
    float m_curr_los[9];

  public:
    Agent() {
        m_program = NULL;
        m_pointmap = NULL;
        m_output_mode = OUTPUT_NOTHING;
    }
    Agent(AgentProgram *program, PointMap *pointmap, int output_mode = OUTPUT_NOTHING);
    void onInit(PixelRef node, int trail_num = -1);
    void onClose();
    Point2f onLook(bool wholeisovist);
    Point2f onStandardLook(bool wholeisovist);
    Point2f onWeightedLook(bool wholeisovist);
    Point2f onOcclusionLook(bool wholeisovist, int looktype);
    Point2f onLoSLook(bool wholeisovist, int look_type);
    Point2f onDirectedLoSLook(bool wholeisovist, int look_type);
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
    bool gateEncountered() { return m_gate_encountered; }
    const Point2f &getLoc() const { return m_loc; }
    //
    bool atTarget() const { return m_at_target; }
    bool atDestination() const { return m_at_destination; }
    //
    const Point2f &getLocation() const { return m_loc; }
    const Point2f &getVector() const { return m_vector; }
    const PixelRef getNode() const { return m_node; }
    int getFrame() const { return m_frame; }
    const PointMap &getPointMap() const { return *m_pointmap; }
};

// note the add 0.5 means angles from e.g., -1/32 to 1/32 are in bin 0
inline int binfromvec(const Point2f &p) { return int(32.0 * (0.5 * p.angle() / M_PI) + 0.5); }

// a random angle based on a bin direction
inline double anglefrombin2(int here) {
    return (2.0 * M_PI) * ((double(here) - 0.5) / 32.0 + prandom() / 32.0);
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
    wpair(double w = 0.0, int n = -1) {
        weight = w;
        node = n;
    }
};

// convert an x / y difference to it's corresponding connection direction
inline char connectValue(PixelRef dir) {
    if (dir.y > 0) {
        return static_cast<char>(Point::CONNECT_NE << (1 - dir.x));
    } else if (dir.y < 0) {
        return static_cast<char>(Point::CONNECT_SW << (dir.x + 1));
    } else if (dir.x == 1) {
        return (char)Point::CONNECT_E;
    } else {
        return (char)Point::CONNECT_W;
    }
}
