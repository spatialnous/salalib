// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2019 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "agentset.h"

#include "salalib/pixelref.h"

AgentSet::AgentSet() {
    m_release_rate = 0.1;
    m_lifetime = 1000;
}

void AgentSet::init(int agent, int trail_num) {
    if (m_release_locations.size()) {
        auto which = pafrand() % m_release_locations.size();
        agents[agent].onInit(m_release_locations[which], trail_num);
    } else {
        const PointMap &map = agents[agent].getPointMap();
        PixelRef pix;
        do {
            pix = map.pickPixel(prandom(m_release_locations_seed));
        } while (!map.getPoint(pix).filled());
        agents[agent].onInit(pix, trail_num);
    }
}

void AgentSet::move() {
    // go through backwards so remove does not affect later agents
    for (auto rev_iter = agents.rbegin(); rev_iter != agents.rend(); ++rev_iter) {
        rev_iter->onMove();
        if (rev_iter->getFrame() >= m_lifetime) {
            agents.erase(std::next(rev_iter).base());
        }
    }
}
