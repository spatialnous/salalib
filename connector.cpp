// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connector.h"

#include "genlib/containerutils.h"
#include "genlib/readwritehelpers.h"

#include <fstream>
#include <math.h>
#include <time.h>

bool Connector::read(std::istream &stream) {
    m_connections.clear();
    m_forward_segconns.clear();
    m_back_segconns.clear();

    // n.b., must set displayed attribute as soon as loaded...

    // The metagraph file format uses signed integers for connections
    // therefore we have to first read that vector and then convert
    dXreadwrite::readFromCastIntoVector<int>(stream, m_connections);

    stream.read((char *)&m_segment_axialref, sizeof(m_segment_axialref));

    dXreadwrite::readIntoMap(stream, m_forward_segconns);
    dXreadwrite::readIntoMap(stream, m_back_segconns);

    return true;
}

bool Connector::write(std::ostream &stream) const {
    // n.b., must set displayed attribute as soon as loaded...
    dXreadwrite::writeCastVector<int>(stream, m_connections);
    stream.write((char *)&m_segment_axialref, sizeof(m_segment_axialref));

    dXreadwrite::writeMap(stream, m_forward_segconns);
    dXreadwrite::writeMap(stream, m_back_segconns);

    return true;
}

/////////////////////////////////////////////////////////////////////////////////

// Cursor extras

size_t Connector::count(int mode) const {
    size_t c = 0;
    switch (mode) {
    case CONN_ALL:
        c = m_connections.size();
        break;
    case SEG_CONN_ALL:
        c = m_back_segconns.size() + m_forward_segconns.size();
        break;
    case SEG_CONN_FW:
        c = m_forward_segconns.size();
        break;
    case SEG_CONN_BK:
        c = m_back_segconns.size();
        break;
    }
    return c;
}
int Connector::getConnectedRef(int cursor, int mode) const {
    int cur = -1;
    if (cursor != -1) {
        switch (mode) {
        case CONN_ALL:
            if (cursor < int(m_connections.size())) {
                cur = m_connections[size_t(cursor)];
            }
            break;
        case SEG_CONN_ALL:
            if (cursor < int(m_back_segconns.size())) {
                cur = depthmapX::getMapAtIndex(m_back_segconns, cursor)->first.ref;
            } else if (size_t(cursor) - m_back_segconns.size() < m_forward_segconns.size()) {
                cur = depthmapX::getMapAtIndex(m_forward_segconns,
                                               cursor - int(m_back_segconns.size()))
                          ->first.ref;
            }
            break;
        case SEG_CONN_FW:
            if (cursor < int(m_forward_segconns.size())) {
                cur = depthmapX::getMapAtIndex(m_forward_segconns, cursor)->first.ref;
            }
            break;
        case SEG_CONN_BK:
            if (cursor < int(m_back_segconns.size())) {
                cur = depthmapX::getMapAtIndex(m_back_segconns, cursor)->first.ref;
            }
            break;
        }
    }
    return cur;
}
int Connector::direction(int cursor, int mode) const {
    int direction = 0;
    if (cursor != -1) {
        switch (mode) {
        case SEG_CONN_ALL:
            if (cursor < (int)m_back_segconns.size()) {
                direction = depthmapX::getMapAtIndex(m_back_segconns, cursor)->first.dir;
            } else if (size_t(cursor) - m_back_segconns.size() < m_forward_segconns.size()) {
                direction = depthmapX::getMapAtIndex(m_forward_segconns,
                                                     cursor - int(m_back_segconns.size()))
                                ->first.dir;
            }
            break;
        case SEG_CONN_FW:
            direction = depthmapX::getMapAtIndex(m_forward_segconns, cursor)->first.dir;
            break;
        case SEG_CONN_BK:
            direction = depthmapX::getMapAtIndex(m_back_segconns, cursor)->first.dir;
            break;
        }
    }
    return direction;
}
float Connector::weight(int cursor, int mode) const {
    float weight = 0.0f;
    if (cursor != -1) {
        switch (mode) {
        case SEG_CONN_ALL:
            if (cursor < int(m_back_segconns.size())) {
                weight = depthmapX::getMapAtIndex(m_back_segconns, cursor)->second;
            } else if (size_t(cursor) - m_back_segconns.size() < m_forward_segconns.size()) {
                weight = depthmapX::getMapAtIndex(m_forward_segconns,
                                                  cursor - int(m_back_segconns.size()))
                             ->second;
            }
            break;
        case SEG_CONN_FW:
            weight = depthmapX::getMapAtIndex(m_forward_segconns, cursor)->second;
            break;
        case SEG_CONN_BK:
            weight = depthmapX::getMapAtIndex(m_back_segconns, cursor)->second;
            break;
        }
    }
    return weight;
}
