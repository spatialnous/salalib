// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "axialminimiser.h"

#include "tolerances.h"

static int compareValueTriplet(const void *p1, const void *p2) {
    ValueTriplet *vp1 = (ValueTriplet *)p1;
    ValueTriplet *vp2 = (ValueTriplet *)p2;
    return (vp1->value1 > vp2->value1   ? 1
            : vp1->value1 < vp2->value1 ? -1
                                        : (vp1->value2 > vp2->value2   ? 1
                                           : vp1->value2 < vp2->value2 ? -1
                                                                       : 0));
}

AxialMinimiser::AxialMinimiser(const ShapeGraph &alllinemap, int noOfAxsegcuts, int noOfRadialsegs)
    : m_alllinemap((ShapeGraph *)&alllinemap), m_vps(new ValueTriplet[noOfAxsegcuts]),
      m_removed(new bool[noOfAxsegcuts]), m_affected(new bool[noOfAxsegcuts]),
      m_vital(new bool[noOfAxsegcuts]), m_radialsegcounts(new int[noOfRadialsegs]) {}

AxialMinimiser::~AxialMinimiser() {
    delete[] m_vital;
    delete[] m_affected;
    delete[] m_radialsegcounts;
    delete[] m_vps;
    delete[] m_removed;
}

// Alan and Bill's algo...

void AxialMinimiser::removeSubsets(std::map<int, std::set<int>> &axsegcuts,
                                   std::map<RadialKey, RadialSegment> &radialsegs,
                                   std::map<RadialKey, std::set<int>> &rlds,
                                   std::vector<RadialLine> &radialLines,
                                   std::vector<std::vector<int>> &keyvertexconns,
                                   std::vector<int> &keyvertexcounts) {
    bool removedflag = true;

    m_axialconns = m_alllinemap->m_connectors;

    for (size_t x = 0; x < radialsegs.size(); x++) {
        m_radialsegcounts[x] = 0;
    }
    int y = -1;
    for (const auto &axSegCut : axsegcuts) {
        y++;
        for (int cut : axSegCut.second) {
            m_radialsegcounts[cut] += 1;
        }
        m_removed[y] = false;
        m_vital[y] = false;
        m_affected[y] = true;
        m_vps[y].index = y;
        double length = m_axialconns[y].connections.size();
        m_vps[y].value1 = (int)length;
        length = depthmapX::getMapAtIndex(m_alllinemap->m_shapes, y)->second.getLine().length();
        m_vps[y].value2 = (float)length;
    }

    // sort according to number of connections then length
    qsort(m_vps, m_axialconns.size(), sizeof(ValueTriplet), compareValueTriplet);

    while (removedflag) {

        removedflag = false;
        for (size_t i = 0; i < m_axialconns.size(); i++) {
            auto ii = static_cast<size_t>(m_vps[i].index);
            if (m_removed[ii] || !m_affected[ii] || m_vital[ii]) {
                continue;
            }
            // vital connections code (uses original unaltered connections)
            {
                bool vitalconn = false;
                for (size_t j = 0; j < keyvertexconns[ii].size(); j++) {
                    // first check to see if removing this line will cause elimination of
                    // a vital connection
                    if (keyvertexcounts[keyvertexconns[ii][j]] <= 1) {
                        // connect vital... just go on to the next one:
                        vitalconn = true;
                        break;
                    }
                }
                if (vitalconn) {
                    m_vital[ii] = true;
                    continue;
                }
            }
            //
            Connector &axa = m_axialconns[ii];
            m_affected[ii] = false;
            bool subset = false;
            for (size_t j = 0; j < axa.connections.size(); j++) {
                auto indextob = axa.connections[j];
                if (indextob == ii ||
                    m_removed[indextob]) { // <- removed[indextob] should never happen
                                           // as it should have been removed below
                    continue;
                }
                Connector &axb = m_axialconns[indextob];
                if (axa.connections.size() <= axb.connections.size()) {
                    // change to 10.08, coconnecting is 1 -> connection to other line is
                    // implicitly handled
                    int coconnecting = 1;
                    // first check it's a connection subset
                    // note that changes in 10.08 mean that lines no longer connect to
                    // themselves this means that the subset 1 connects {2,3} and 2
                    // connects {1,3} are equivalent
                    for (size_t axai = 0, axbi = 0;
                         axai < axa.connections.size() && axbi < axb.connections.size();
                         axai++, axbi++) {
                        // extra 10.08 -> step over connection to b
                        if (axa.connections[axai] == indextob) {
                            axai++;
                        }
                        // extra 10.08 add axb.connections[axbi] == ii -> step over
                        // connection to
                        // a
                        while (axbi < axb.connections.size() &&
                               (axb.connections[axbi] == ii ||
                                axa.connections[axai] > axb.connections[axbi])) {
                            axbi++;
                        }
                        if (axbi >= axb.connections.size()) {
                            break;
                        } else if (axa.connections[axai] == axb.connections[axbi]) {
                            coconnecting++;
                        } else if (axa.connections[axai] < axb.connections[axbi]) {
                            break;
                        }
                    }
                    if (coconnecting >= (int)axa.connections.size()) {
                        subset = true;
                        break;
                    }
                }
            }
            if (subset) {
                size_t removeindex = ii;
                // now check removing it won't break any topological loops
                bool presumedvital = false;
                auto &axSegCut = depthmapX::getMapAtIndex(axsegcuts, removeindex)->second;
                for (int cut : axSegCut) {
                    if (m_radialsegcounts[cut] <= 1) {
                        presumedvital = true;
                        break;
                    }
                }
                if (presumedvital) {
                    presumedvital =
                        checkVital(removeindex, axSegCut, radialsegs, rlds, radialLines);
                }
                if (presumedvital) {
                    m_vital[removeindex] = true;
                }
                // if not, remove it...
                if (!m_vital[removeindex]) {
                    m_removed[removeindex] = true;
                    auto &affectedconnections = m_axialconns[removeindex].connections;
                    for (auto affectedconnection : affectedconnections) {
                        if (!m_removed[affectedconnection]) {
                            auto &connections = m_axialconns[affectedconnection].connections;
                            depthmapX::findAndErase(connections, removeindex);
                            m_affected[affectedconnection] = true;
                        }
                    }
                    removedflag = true;
                    for (int cut : axSegCut) {
                        m_radialsegcounts[cut] -= 1;
                    }
                    // vital connections
                    for (size_t k = 0; k < keyvertexconns[removeindex].size(); k++) {
                        keyvertexcounts[keyvertexconns[removeindex][k]] -= 1;
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

// My algo... v. simple... fewest longest

void AxialMinimiser::fewestLongest(std::map<int, std::set<int>> &axsegcuts,
                                   std::map<RadialKey, RadialSegment> &radialsegs,
                                   std::map<RadialKey, std::set<int>> &rlds,
                                   std::vector<RadialLine> &radialLines,
                                   std::vector<std::vector<int>> &keyvertexconns,
                                   std::vector<int> &keyvertexcounts) {
    // m_axialconns = m_alllinemap->m_connectors;
    int livecount = 0;

    for (size_t y = 0; y < m_axialconns.size(); y++) {
        if (!m_removed[y] && !m_vital[y]) {
            m_vps[livecount].index = (int)y;
            m_vps[livecount].value1 = (int)m_axialconns[y].connections.size();
            m_vps[livecount].value2 = (float)depthmapX::getMapAtIndex(m_alllinemap->m_shapes, y)
                                          ->second.getLine()
                                          .length();
            livecount++;
        }
    }

    qsort(m_vps, livecount, sizeof(ValueTriplet), compareValueTriplet);

    for (int i = 0; i < livecount; i++) {

        int j = m_vps[i].index;
        // vital connections code (uses original unaltered connections)
        bool vitalconn = false;
        size_t k;
        for (k = 0; k < keyvertexconns[j].size(); k++) {
            // first check to see if removing this line will cause elimination of a
            // vital connection
            if (keyvertexcounts[keyvertexconns[j][k]] <= 1) {
                // connect vital... just go on to the next one:
                vitalconn = true;
                break;
            }
        }
        if (vitalconn) {
            continue;
        }
        //
        bool presumedvital = false;
        auto &axSegCut = depthmapX::getMapAtIndex(axsegcuts, j)->second;
        for (int cut : axSegCut) {
            if (m_radialsegcounts[cut] <= 1) {
                presumedvital = true;
                break;
            }
        }
        if (presumedvital) {
            presumedvital = checkVital(j, axSegCut, radialsegs, rlds, radialLines);
        }
        if (!presumedvital) {
            // don't let anything this is connected to go down to zero connections
            auto &affectedconnections = m_axialconns[j].connections;
            for (auto affectedconnection : affectedconnections) {
                if (!m_removed[affectedconnection]) {
                    auto &connections = m_axialconns[size_t(affectedconnection)].connections;
                    if (connections.size() <= 2) { // <- note number of connections includes
                                                   // itself... so you and one other
                        presumedvital = true;
                        break;
                    }
                }
            }
        }
        if (!presumedvital) {
            m_removed[j] = true;
            auto &affectedconnections = m_axialconns[size_t(j)].connections;
            for (auto affectedconnection : affectedconnections) {
                if (!m_removed[affectedconnection]) {
                    auto &connections = m_axialconns[size_t(affectedconnection)].connections;
                    depthmapX::findAndErase(connections, static_cast<size_t>(j));
                    m_affected[affectedconnection] = true;
                }
            }
            for (auto cut : axSegCut) {
                m_radialsegcounts[cut] -= 1;
            }
            // vital connections
            for (size_t k = 0; k < keyvertexconns[j].size(); k++) {
                keyvertexcounts[keyvertexconns[j][k]] -= 1;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

bool AxialMinimiser::checkVital(int checkindex, std::set<int> &axSegCut,
                                std::map<RadialKey, RadialSegment> &radialsegs,
                                std::map<RadialKey, std::set<int>> &rlds,
                                std::vector<RadialLine> &radialLines) {
    std::map<int, SalaShape> &axiallines = m_alllinemap->m_shapes;

    bool presumedvital = true;
    int nonvitalcount = 0, vitalsegs = 0;
    // again, this time more rigourously... check any connected pairs don't cover
    // the link...
    for (int cut : axSegCut) {
        if (m_radialsegcounts[cut] <= 1) {
            bool nonvitalseg = false;
            vitalsegs++;
            auto radialSegIter = depthmapX::getMapAtIndex(radialsegs, cut);
            const RadialKey &key = radialSegIter->first;
            RadialSegment &seg = radialSegIter->second;
            std::set<int> &divisorsa = rlds.find(key)->second;
            std::set<int> &divisorsb = rlds.find(seg.radialB)->second;
            auto iterKey = std::find(radialLines.begin(), radialLines.end(), key);
            if (iterKey == radialLines.end()) {
                throw depthmapX::RuntimeException("Radial key not found in radial lines");
            }
            const RadialLine &rlinea = *iterKey;

            auto iterSegB = std::find(radialLines.begin(), radialLines.end(), seg.radialB);
            if (iterSegB == radialLines.end()) {
                throw depthmapX::RuntimeException("Radial key not found in radial lines");
            }
            const RadialLine &rlineb = *iterSegB;
            for (int diva : divisorsa) {
                if (diva == checkindex || m_removed[diva]) {
                    continue;
                }
                for (int divb : divisorsb) {
                    if (divb == checkindex || m_removed[divb]) {
                        continue;
                    }
                    auto &connections = m_axialconns[size_t(diva)].connections;
                    if (std::find(connections.begin(), connections.end(), divb) !=
                        connections.end()) {
                        // as a further challenge, they must link within in the zone of
                        // interest, not on the far side of it... arg!
                        Point2f p = intersection_point(axiallines[diva].getLine(),
                                                       axiallines[divb].getLine(), TOLERANCE_A);
                        if (p.insegment(rlinea.keyvertex, rlinea.openspace, rlineb.openspace,
                                        TOLERANCE_A)) {
                            nonvitalseg = true;
                        }
                    }
                }
            }
            if (nonvitalseg) {
                nonvitalcount++;
            }
        }
    }
    if (nonvitalcount == vitalsegs) {
        presumedvital = false;
    }
    return presumedvital;
}
