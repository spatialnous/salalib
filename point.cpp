// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "point.hpp"

#include "ngraph.hpp"

float Point::getBinDistance(int i) { return m_node->bindistance(i); }

std::istream &Point::read(std::istream &stream) {
    stream.read(reinterpret_cast<char *>(&m_state), sizeof(m_state));
    // block is the same size as m_noderef used to be for ease of replacement:
    // (note block NO LONGER used!)
    stream.read(reinterpret_cast<char *>(&m_block), sizeof(m_block));

    int dummy = 0;
    stream.read(reinterpret_cast<char *>(&dummy), sizeof(dummy));

    stream.read(reinterpret_cast<char *>(&m_gridConnections), sizeof(m_gridConnections));

    stream.read(reinterpret_cast<char *>(&m_merge), sizeof(m_merge));
    bool ngraph;
    stream.read(reinterpret_cast<char *>(&ngraph), sizeof(ngraph));
    if (ngraph) {
        m_node = std::unique_ptr<Node>(new Node());
        m_node->read(stream);
    }

    stream.read(reinterpret_cast<char *>(&m_location), sizeof(m_location));

    return stream;
}

std::ostream &Point::write(std::ostream &stream) const {
    stream.write(reinterpret_cast<const char *>(&m_state), sizeof(m_state));
    // block is the same size as m_noderef used to be for ease of replacement:
    // note block is no longer used at all
    stream.write(reinterpret_cast<const char *>(&m_block), sizeof(m_block));
    int dummy = 0;
    stream.write(reinterpret_cast<const char *>(&dummy), sizeof(dummy));
    stream.write(reinterpret_cast<const char *>(&m_gridConnections), sizeof(m_gridConnections));
    stream.write(reinterpret_cast<const char *>(&m_merge), sizeof(m_merge));
    bool ngraph;
    if (m_node) {
        ngraph = true;
        stream.write(reinterpret_cast<const char *>(&ngraph), sizeof(ngraph));
        m_node->write(stream);
    } else {
        ngraph = false;
        stream.write(reinterpret_cast<const char *>(&ngraph), sizeof(ngraph));
    }
    stream.write(reinterpret_cast<const char *>(&m_location), sizeof(m_location));
    return stream;
}
