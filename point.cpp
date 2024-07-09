// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "point.h"

#include "ngraph.h"

float Point::getBinDistance(int i) { return m_node->bindistance(i); }

std::istream &Point::read(std::istream &stream) {
    stream.read((char *)&m_state, sizeof(m_state));
    // block is the same size as m_noderef used to be for ease of replacement:
    // (note block NO LONGER used!)
    stream.read((char *)&m_block, sizeof(m_block));

    int dummy = 0;
    stream.read(reinterpret_cast<char *>(&dummy), sizeof(dummy));

    stream.read((char *)&m_grid_connections, sizeof(m_grid_connections));

    stream.read((char *)&m_merge, sizeof(m_merge));
    bool ngraph;
    stream.read((char *)&ngraph, sizeof(ngraph));
    if (ngraph) {
        m_node = std::unique_ptr<Node>(new Node());
        m_node->read(stream);
    }

    stream.read((char *)&m_location, sizeof(m_location));

    return stream;
}

std::ostream &Point::write(std::ostream &stream) const {
    stream.write((char *)&m_state, sizeof(m_state));
    // block is the same size as m_noderef used to be for ease of replacement:
    // note block is no longer used at all
    stream.write((char *)&m_block, sizeof(m_block));
    int dummy = 0;
    stream.write((char *)&dummy, sizeof(dummy));
    stream.write((char *)&m_grid_connections, sizeof(m_grid_connections));
    stream.write((char *)&m_merge, sizeof(m_merge));
    bool ngraph;
    if (m_node) {
        ngraph = true;
        stream.write((char *)&ngraph, sizeof(ngraph));
        m_node->write(stream);
    } else {
        ngraph = false;
        stream.write((char *)&ngraph, sizeof(ngraph));
    }
    stream.write((char *)&m_location, sizeof(m_location));
    return stream;
}
