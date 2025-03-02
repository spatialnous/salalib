// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "agent.hpp"

#include "agentanalysis.hpp"

Agent::Agent(AgentProgram *program, PointMap *pointmap, int outputMode)
    : m_program(program), m_pointmap(pointmap), m_node(), m_outputMode(outputMode), m_trailNum(-1),
      m_loc(), m_target(), m_vector(), m_destination(), m_targetPix(), m_occMemory() {}

void Agent::onInit(PixelRef node, int trailNum) {
    m_node = node;
    m_loc = m_pointmap->depixelate(m_node);
    if (m_outputMode & OUTPUT_GATE_COUNTS) {
        // see note about gates in Through vision analysis
        m_gate = (m_pointmap->getPoint(node).filled())
                     ? static_cast<int>(m_pointmap->getAttributeTable()
                                            .getRow(AttributeKey(m_node))
                                            .getValue(AgentAnalysis::Column::INTERNAL_GATE))
                     : -1;
    } else {
        m_gate = -1;
    }
    m_gateEncountered = false;
    m_step = 0;
    m_stuck = false;
    m_stopped = false;
    m_frame = 0;
    m_targetLock = false;
    m_vector = Point2f(1, 0);

    m_atTarget = false;
    m_atDestination = false;

    m_trailNum = trailNum;

    m_vector = onLook(true);

    m_vector.normalise();

    m_targetPix = NoPixel;
}

void Agent::onMove() {
    m_atTarget = false;
    m_frame++;
    if (m_program->destinationDirected && m_loc.dist(m_destination) < 10.0) {
        // reached final destination
        onDestination();
    } else if ((m_program->selType & AgentProgram::SEL_TARGETTED) &&
               m_loc.dist(m_target) < m_pointmap->getSpacing()) {
        // reached target (intermediate destination)
        m_step = 0;
        onTarget();
        m_vector = onLook(false);
    } else if (pafmath::prandomr() < (1.0 / m_program->steps) &&
               !m_targetLock) { // note, on average, will change 1 in steps
        m_step = 0;
        m_vector = onLook(false);
        /*
        if (m_program->destinationDirected) {
           Point2f vec2 = m_destination - m_loc;
           vec2.normalise();
           if (dot(vec2,m_vector) < 0.0) {
              m_vector = onLook(false);
           }
        }
        */
    }
    if (m_stuck) {
        // oops...
        return;
    }
    // now step...
    PixelRef lastnode = m_node;
    onStep();
    if (m_node != lastnode && m_outputMode != OUTPUT_NOTHING) {
        if (m_pointmap->getPoint(m_node).filled()) {
            AttributeRow &row = m_pointmap->getAttributeTable().getRow(AttributeKey(m_node));
            if (m_outputMode & OUTPUT_COUNTS) {
                row.incrValue(AgentAnalysis::Column::GATE_COUNTS);
            }
            if (m_outputMode & OUTPUT_GATE_COUNTS) {
                int obj = static_cast<int>(row.getValue(AgentAnalysis::Column::INTERNAL_GATE));
                if (m_gate != obj) {
                    m_gate = obj;
                    if (m_gate != -1) {
                        row.incrValue(AgentAnalysis::Column::INTERNAL_GATE_COUNTS);
                        // actually crossed into a new gate:
                        m_gateEncountered = true;
                    }
                }
            }
        }
    }
    // done. happy hamster.
}
void Agent::onDestination() { m_atDestination = true; }
void Agent::onTarget() {
    m_occMemory.a().clear();
    m_atTarget = true;
}

////////////////////////////////////////////////////////////////////

void Agent::onStep() {
    m_stopped = false;
    m_step++;
    //
    Point2f nextloc = m_loc + (m_vector * m_pointmap->getSpacing());
    // note: false returns unconstrained pixel: goodStep must check it is in bounds using
    // m_pointmap->includes
    PixelRef nextnode = m_pointmap->pixelate(nextloc, false);
    if (nextnode != m_node) {
        // quick check location is okay...
        if (goodStep(nextnode)) {
            m_node = nextnode;
            m_loc = nextloc;
        } else {
            // try other nearby nodes...
            if (!diagonalStep()) {
                m_stopped = true;
            }
        }
    } else {
        m_loc = nextloc;
    }
    if (!m_stopped && m_trailNum != -1) {
        m_program->trails[static_cast<size_t>(m_trailNum)].push_back(
            Event2f(m_loc, m_program->steps));
    }
}
bool Agent::diagonalStep() {
    Point2f vector1 = m_vector;
    vector1.rotate(M_PI / 4.0);
    Point2f nextloc1 = m_loc + (vector1 * m_pointmap->getSpacing());
    // note: "false" does not constrain to bounds: must be checked using m_pointmap->includes before
    // getPoint is used
    PixelRef nextnode1 = m_pointmap->pixelate(nextloc1, false);

    Point2f vector2 = m_vector;
    vector2.rotate(-M_PI / 4.0);
    Point2f nextloc2 = m_loc + (vector2 * m_pointmap->getSpacing());
    // note: "false" does not constrain to bounds: must be checked using m_pointmap->includes before
    // getPoint is used
    int nextnode2 = m_pointmap->pixelate(nextloc2, false);

    bool good = false;
    if (pafmath::pafrand() % 2 == 0) {
        if (goodStep(nextnode1)) {
            m_node = nextnode1;
            m_loc = nextloc1;
            good = true;
        } else if (goodStep(nextnode2)) {
            m_node = nextnode2;
            m_loc = nextloc2;
            good = true;
        }
    } else {
        if (goodStep(nextnode2)) {
            m_node = nextnode2;
            m_loc = nextloc2;
            good = true;
        } else if (goodStep(nextnode1)) {
            m_node = nextnode1;
            m_loc = nextloc1;
            good = true;
        }
    }
    return good;
}
bool Agent::goodStep(PixelRef node) {
    if (!m_pointmap->includes(node)) {
        return false;
    }
    // n.b., you have to know how the nodes are labelled for this connectValue trick
    PixelRef dir;
    dir.x = node.x - m_node.x;
    dir.y = node.y - m_node.y;
    // now translate dir to correct CONNECT value
    if (m_pointmap->getPoint(m_node).getGridConnections() & connectValue(dir)) {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////

// The various look algorithms

Point2f Agent::onLook(bool wholeisovist) {
    Point2f dir;
    if (m_program->selType & AgentProgram::SEL_GIBSONIAN) {
        dir = onGibsonianLook(wholeisovist);
    } else if ((m_program->selType & AgentProgram::SEL_OCCLUSION) == AgentProgram::SEL_OCCLUSION) {
        dir = onOcclusionLook(wholeisovist, m_program->selType);
    } else {
        switch (m_program->selType) {
        case AgentProgram::SEL_STANDARD:
            dir = onStandardLook(wholeisovist);
            break;
        case AgentProgram::SEL_LOS:
        case AgentProgram::SEL_LOS_OCC:
            if (m_program->destinationDirected) {
                dir = onDirectedLoSLook(wholeisovist, m_program->selType);
            } else {
                dir = onLoSLook(wholeisovist, m_program->selType);
            }
            break;
        case AgentProgram::SEL_OPTIC_FLOW2:
            dir = onGibsonianLook2(wholeisovist);
            break;
        }
    }
    if ((m_program->selType & AgentProgram::SEL_GIBSONIAN) && !m_stuck) {
        // remember what the view looked like here, facing our new direction:
        calcLoS(binfromvec(dir), false);
    }
    if ((m_program->selType & AgentProgram::SEL_GIBSONIAN2) && !m_stuck) {
        calcLoS2(binfromvec(dir), false);
    }

    return dir;
}

Point2f Agent::onStandardLook(bool wholeisovist) {
    int tarpixelate = -1;
    int vbin = m_program->vbin;
    if (wholeisovist || vbin == -1) {
        vbin = 16;
    }
    int directionbin = 32 + binfromvec(m_vector) - vbin;
    int choices = 0;
    // reset for getting list, check in range:
    vbin = vbin * 2 + 1;
    if (vbin > 32) {
        vbin = 32;
    }
    for (int i = 0; i < vbin; i++) {
        choices += m_pointmap->getPoint(m_node).getNode().bincount((directionbin + i) % 32);
    }
    if (choices == 0) {
        if (!wholeisovist) {
            return onStandardLook(true);
        } else {
            m_stuck = true;
            m_target = m_loc;
            m_targetPix = m_node;
            return Point2f(0, 0);
        }
    } else {
        auto chosen = static_cast<int>(pafmath::pafrand() % static_cast<unsigned int>(choices));
        Node &node = m_pointmap->getPoint(m_node).getNode();
        for (; chosen >= node.bincount(directionbin % 32); directionbin++) {
            chosen -= node.bincount(directionbin % 32);
        }
        Bin &bin = node.bin(directionbin % 32);
        bin.first();
        tarpixelate = bin.cursor();
        for (; chosen > 0; chosen--) {
            bin.next();
            tarpixelate = bin.cursor();
        }
    }

    m_targetPix = tarpixelate;
    m_target = m_pointmap->depixelate(tarpixelate);

    return (m_target - m_loc).normalise();
}

// TODO: Expose this functionality to the UIs
Point2f Agent::onWeightedLook(bool wholeisovist) {
    if (wholeisovist) {
        // use standard targetted look instead:
        return onStandardLook(true);
    }
    int tarpixelate = -1;
    int vbin = m_program->vbin;
    if (vbin == -1) {
        vbin = 16;
    }
    int aheadbin = binfromvec(m_vector);
    int directionbin = 32 + binfromvec(m_vector) - vbin;
    std::vector<wpair> weightmap;
    double weight = 0.0;
    // reset for getting list, check in range:
    vbin = vbin * 2 + 1;
    if (vbin > 32) {
        vbin = 32;
    }
    for (int i = 0; i < vbin; i++) {
        Bin &bin = m_pointmap->getPoint(m_node).getNode().bin((directionbin + i) % 32);
        bin.first();

        // Quick mod - TV
#if defined(_MSC_VER)
        int node = bin.is_tail() ? -1 : bin.cursor();
#else
        int node = bin.is_tail() ? -1 : bin.cursor().x;
#endif
        while (node != -1) {
            weight += ((directionbin + i) % 32 == aheadbin) ? 5.0 : 1.0;
            weightmap.push_back(wpair(weight, node));
            bin.next();
            // Quick mod - TV
#if defined(_MSC_VER)
            node = bin.is_tail() ? -1 : bin.cursor();
#else
            node = bin.is_tail() ? -1 : bin.cursor().x;
#endif
        }
    }
    if (weightmap.size() == 0) {
        return onWeightedLook(true);
    } else {
        double chosen = pafmath::prandomr() * weight;
        for (size_t i = 0; i < weightmap.size(); i++) {
            if (chosen < weightmap[i].weight) {
                tarpixelate = weightmap[i].node;
                break;
            }
        }
    }

    m_targetPix = tarpixelate;
    m_target = m_pointmap->depixelate(tarpixelate);

    return (m_target - m_loc).normalise();
}

Point2f Agent::onOcclusionLook(bool wholeisovist, int looktype) {
    if (looktype == AgentProgram::SEL_OCC_MEMORY) {
        m_occMemory.flip();
        m_occMemory.a().clear();
    }

    if (wholeisovist) {
        // use standard targetted look instead:
        return onStandardLook(true);
    }
    PixelRef tarpixelate = NoPixel;
    int vbin = m_program->vbin;
    if (vbin == -1) {
        vbin = 16;
    }
    int directionbin = 32 + binfromvec(m_vector) - vbin;
    // reset for getting list, check in range:
    vbin = vbin * 2 + 1;
    if (vbin > 32) {
        vbin = 32;
    }
    if (looktype == AgentProgram::SEL_OCC_ALL) {
        int choices = 0;
        Node &node = m_pointmap->getPoint(m_node).getNode();
        for (int i = 0; i < vbin; i++) {
            if (node.occlusionBins[(directionbin + i) % 32].size()) {
                choices += static_cast<int>(node.occlusionBins[(directionbin + i) % 32].size());
            }
        }
        if (choices == 0) {
            if (!wholeisovist) {
                return onStandardLook(false);
            }
            // Originally here the agents could be put to the
            // stuck state, however at a point it was decided
            // that it should instead revert to the Standard
            // look. With an earlier check for wholeisovist this
            // check will never be entered, but left here for
            // reference
            // else {
            //     m_stuck = true;
            //     m_targetPix = m_node;
            //     m_target = m_loc;
            //     return Point2f(0, 0);
            // }
        } else {
            size_t chosen = pafmath::pafrand() % static_cast<unsigned int>(choices);
            for (; chosen >= node.occlusionBins[directionbin % 32].size(); directionbin++) {
                chosen -= node.occlusionBins[directionbin % 32].size();
            }
            tarpixelate = node.occlusionBins[directionbin % 32].at(chosen);
        }
    } else {
        int subset = 1;
        if (looktype == AgentProgram::SEL_OCC_BIN45) {
            subset = 3;
        } else if (looktype == AgentProgram::SEL_OCC_BIN60) {
            subset = 5;
        }
        std::vector<wpair> weightmap;
        double weight = 0.0;
        Node &node = m_pointmap->getPoint(m_node).getNode();
        for (int i = 0; i < vbin; i += subset) {
            PixelRef nigpix;
            double fardist = -1.0;
            for (int k = 0; k < subset; k++) {
                for (size_t j = 0; j < node.occlusionBins[(directionbin + i + k) % 32].size();
                     j++) {
                    PixelRef pix = node.occlusionBins[(directionbin + i + k) % 32].at(j);
                    if (dist(pix, m_node) > fardist) {
                        fardist = dist(pix, m_node);
                        nigpix = pix;
                    }
                }
            }
            if (fardist != -1.0) {
                bool cont = true;
                if (looktype == AgentProgram::SEL_OCC_MEMORY) {
                    depthmapX::addIfNotExists(m_occMemory.a(), nigpix);
                    // the turn chance (pafrand() % 2) may have to be modified later...
                    if (!m_atTarget && std::find(m_occMemory.b().begin(), m_occMemory.b().end(),
                                                 nigpix) != m_occMemory.b().end()) {
                        cont = false;
                    }
                }
                if (cont) {
                    switch (looktype) {
                    case AgentProgram::SEL_OCC_WEIGHT_DIST:
                        weight += fardist;
                        break;
                    case AgentProgram::SEL_OCC_WEIGHT_ANG:
                        weight += static_cast<double>(vbin - abs(i - vbin));
                        break;
                    case AgentProgram::SEL_OCC_WEIGHT_DIST_ANG:
                        weight += fardist * static_cast<double>(vbin - abs(i - vbin));
                        break;
                    default:
                        weight += 1.0;
                        break;
                    }
                    weightmap.push_back(wpair(weight, nigpix));
                }
            }
        }
        if (weightmap.size() == 0) {
            if (!wholeisovist) {
                return onStandardLook(false);
            }
            // Originally here the agents could be put to the
            // stuck state, however at a point it was decided
            // that it should instead revert to the Standard
            // look. With an earlier check for wholeisovist this
            // check will never be entered, but left here for
            // reference
            // else {
            //     m_stuck = true;
            //     m_target = m_loc;
            //     return Point2f(0, 0);
            // }
        } else {
            double chosen = pafmath::prandomr() * weight;
            for (size_t i = 0; i < weightmap.size(); i++) {
                if (chosen < weightmap[i].weight) {
                    tarpixelate = weightmap[i].node;
                    break;
                }
            }
        }
    }

    m_targetPix = tarpixelate;
    m_target = m_pointmap->depixelate(tarpixelate);

    return (m_target - m_loc).normalise();
}

// note: LOS look uses similar weighted choice mechanism

Point2f Agent::onLoSLook(bool wholeisovist, int lookType) {
    int bbin = -1;
    if (m_program->destinationDirected) {
        Point2f vec2 = m_destination - m_loc;
        vec2.normalise();
        bbin = binfromvec(vec2);
    }
    int targetbin = -1;
    int vbin = m_program->vbin;
    if (wholeisovist || vbin == -1) {
        vbin = 16;
    }
    int directionbin = 32 + binfromvec(m_vector) - vbin;
    std::vector<wpair> weightmap;
    double weight = 0.0;
    // reset for getting list, check in range:
    vbin = vbin * 2 + 1;
    if (vbin > 32) {
        vbin = 32;
    }
    for (int i = 0; i < vbin; i++) {
        double los =
            (lookType == AgentProgram::SEL_LOS)
                ? m_pointmap->getPoint(m_node).getNode().bindistance((directionbin + i) % 32)
                : m_pointmap->getPoint(m_node).getNode().occdistance((directionbin + i) % 32);
        if (m_program->losSqrd) {
            los *= los;
        }
        if (m_program->destinationDirected) {
            los *= 1.0 - static_cast<double>(binsbetween(((directionbin + i) % 32), bbin)) / 16.0;
        }
        weight += los;
        weightmap.push_back(wpair(weight, (directionbin + i) % 32));
    }
    if (weight == 0.0) {
        if (!wholeisovist) {
            return onLoSLook(true, lookType);
        } else {
            // oops!
            m_stuck = true;
            return Point2f(0, 0);
        }
    } else {
        double chosen = pafmath::prandomr() * weight;
        for (size_t i = 0; i < weightmap.size(); i++) {
            if (chosen < weightmap[i].weight) {
                targetbin = weightmap[i].node;
                break;
            }
        }
    }

    float angle = static_cast<float>(anglefrombin2(targetbin));

    return Point2f(cosf(angle), sinf(angle));
}

Point2f Agent::onDirectedLoSLook(bool wholeisovist, int lookType) {
    Point2f vec2 = m_destination - m_loc;
    vec2.normalise();
    int targetbin = -1;
    int vbin = m_program->vbin;
    if (wholeisovist || vbin == -1) {
        vbin = 16;
    }
    int directionbin = 32 + binfromvec(vec2) - vbin;
    std::vector<wpair> weightmap;
    double weight = 0.0;
    // reset for getting list, check in range:
    vbin = vbin * 2 + 1;
    if (vbin > 32) {
        vbin = 32;
    }
    for (int i = 0; i < vbin; i++) {
        double los =
            (lookType == AgentProgram::SEL_LOS)
                ? m_pointmap->getPoint(m_node).getNode().bindistance((directionbin + i) % 32)
                : m_pointmap->getPoint(m_node).getNode().occdistance((directionbin + i) % 32);
        if (m_program->losSqrd) {
            los *= los;
        }
        weight += los;
        weightmap.push_back(wpair(weight, (directionbin + i) % 32));
    }
    if (weight == 0.0) {
        if (!wholeisovist) {
            return onLoSLook(true, lookType);
        } else {
            // oops!
            m_stuck = true;
            return Point2f(0, 0);
        }
    } else {
        double chosen = pafmath::prandomr() * weight;
        for (size_t i = 0; i < weightmap.size(); i++) {
            if (chosen < weightmap[i].weight) {
                targetbin = weightmap[i].node;
                break;
            }
        }
    }

    float angle = static_cast<float>(anglefrombin2(targetbin));

    return Point2f(cosf(angle), sinf(angle));
}

// Gibsonian agents record their last known information,
// and act according to their rules:

Point2f Agent::onGibsonianLook(bool wholeisovist) {
    // at start, go in any direction:
    if (wholeisovist) {
        return onLoSLook(true, AgentProgram::SEL_LOS);
    }
    //
    calcLoS(binfromvec(m_vector), true);
    // now, choose action according to type of agent:
    int ruleChoice = -1;
    int dir = 0;
    for (int k = 0; k < 4; k++) {
        dir = onGibsonianRule(m_program->ruleOrder[k]);
        if (dir != 0) {
            ruleChoice = m_program->ruleOrder[k];
            break;
        }
    }

    float angle = 0.0;

    if (ruleChoice != -1) {
        angle = static_cast<float>(
            anglefrombin2((binfromvec(m_vector) + (2 * ruleChoice + 1) * dir + 32) % 32));
    }

    // if no rule selection made, carry on in current direction
    return (ruleChoice == -1) ? m_vector : Point2f(cosf(angle), sinf(angle));
}

int Agent::onGibsonianRule(int rule) {
    int option = 0;
    switch (m_program->selType) {
    case AgentProgram::SEL_LENGTH:
        // rule_threshold from 0 to 100m
        if (m_currLos[rule + 1] > m_program->ruleThreshold[rule]) {
            option = 0x01;
        }
        if (m_currLos[rule + 5] > m_program->ruleThreshold[rule]) {
            option |= 0x10;
        }
        break;
    case AgentProgram::SEL_OPTIC_FLOW:
        // rule_threshold reflects from 0x (0) to 5x (100.0)
        if ((m_currLos[rule + 1] + 1) / (m_lastLos[rule + 1] + 1) >
            m_program->ruleThreshold[rule] / 20.0) {
            option = 0x01;
        }
        if ((m_currLos[rule + 5] + 1) / (m_lastLos[rule + 5] + 1) >
            m_program->ruleThreshold[rule] / 20.0) {
            option |= 0x10;
        }
        break;
    case AgentProgram::SEL_COMPARATIVE_LENGTH:
        // rule_threshold reflects from 0x (0) to 10x (100.0)
        if ((m_currLos[rule + 1] + 1) / (m_currLos[0] + 1) >
            m_program->ruleThreshold[rule] / 10.0) {
            option = 0x01;
        }
        if ((m_currLos[rule + 5] + 1) / (m_currLos[0] + 1) >
            m_program->ruleThreshold[rule] / 10.0) {
            option |= 0x10;
        }
        break;
    case AgentProgram::SEL_COMPARATIVE_OPTIC_FLOW:
        // rule_threshold reflects from 0x (0) to 10x (100.0)
        if ((m_currLos[rule + 1] * m_lastLos[0] + 1) / (m_lastLos[rule + 1] * m_currLos[0] + 1) >
            m_program->ruleThreshold[rule] / 10.0) {
            option = 0x01;
        }
        if ((m_currLos[rule + 5] * m_lastLos[0] + 1) / (m_lastLos[rule + 5] * m_currLos[0] + 1) >
            m_program->ruleThreshold[rule] / 10.0) {
            option |= 0x10;
        }
        break;
    }
    int dir = 0;
    if (option == 0x01 && m_program->ruleProbability[0] > pafmath::prandomr()) {
        dir = -1;
    } else if (option == 0x10 && m_program->ruleProbability[0] > pafmath::prandomr()) {
        dir = +1;
    } else if (option == 0x11 &&
               m_program->ruleProbability[0] > pafmath::prandomr() * pafmath::prandomr()) {
        // note, use random * random event as there are two ways to do this
        dir = (pafmath::pafrand() % 2) ? -1 : +1;
    }
    return dir;
}

Point2f Agent::onGibsonianLook2(bool wholeisovist) {
    // at start, go in any direction:
    if (wholeisovist) {
        return onLoSLook(true, AgentProgram::SEL_LOS);
    }
    //
    calcLoS2(binfromvec(m_vector), true);
    int maxbin = 0;
    /*
    // first action: adjust to longest line of sight
    if (m_currLos[3] > m_currLos[0]) {
       maxbin = -m_program->m_vahead;
       if (m_currLos[4] > m_currLos[0] && (pafrand() % 2)) {
          maxbin = m_program->m_vahead;
       }
    }
    else if (m_currLos[4] > m_currLos[0]) {
       maxbin = m_program->m_vahead;
    }
    */
    // second action, apply feeler rule:
    int8_t dir = 0x00;
    if ((m_currLos[1] - m_lastLos[1]) / m_currLos[1] > m_program->feelerThreshold) {
        dir |= 0x01;
    }
    if ((m_currLos[2] - m_lastLos[2]) / m_currLos[2] > m_program->feelerThreshold) {
        dir |= 0x10;
    }
    if (dir == 0x01 && m_program->feelerProbability > pafmath::prandomr()) {
        maxbin = -m_program->vbin;
    } else if (dir == 0x10 && m_program->feelerProbability > pafmath::prandomr()) {
        maxbin = m_program->vbin;
    } else if (dir == 0x11 &&
               m_program->feelerProbability > pafmath::prandomr() * pafmath::prandomr()) {
        maxbin = (pafmath::pafrand() % 2) ? m_program->vbin : -m_program->vbin;
    }
    // third action: detect heading for dead-end
    if (maxbin == 0 && (m_currLos[0] / m_pointmap->getSpacing() < m_program->aheadThreshold)) {
        if (m_currLos[1] >= m_currLos[2]) {
            maxbin = -m_program->vbin;
        } else {
            maxbin = m_program->vbin;
        }
    }

    int bin = binfromvec(m_vector) + maxbin;
    float angle = static_cast<float>(anglefrombin2(bin));

    return (maxbin == 0) ? m_vector : Point2f(cosf(angle), sinf(angle));
}

void Agent::calcLoS(int directionbin, bool curr) {
    float *los;
    if (curr) {
        los = m_currLos;
    } else {
        los = m_lastLos;
    }
    Node &node = m_pointmap->getPoint(m_node).getNode();
    // ahead
    los[0] = node.bindistance(directionbin % 32);
    // directions:
    int count = 1;
    for (int i = 1; i <= 7; i += 2) {
        los[count] = node.bindistance((directionbin - i + 32) % 32);
        count++;
    }
    for (int j = 1; j <= 7; j += 2) {
        los[count] = node.bindistance((directionbin + j) % 32);
        count++;
    }
}

void Agent::calcLoS2(int directionbin, bool curr) {
    float *los;
    if (curr) {
        los = m_currLos;
    } else {
        los = m_lastLos;
    }
    Node &node = m_pointmap->getPoint(m_node).getNode();
    // ahead
    los[0] = node.bindistance(directionbin % 32);
    // directions:
    los[1] = node.bindistance((directionbin - m_program->vbin + 32) % 32);
    los[2] = node.bindistance((directionbin + m_program->vbin) % 32);
    //
    los[3] = node.bindistance((directionbin - m_program->vahead + 32) % 32);
    los[4] = node.bindistance((directionbin + m_program->vahead) % 32);
}
