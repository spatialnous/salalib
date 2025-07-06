// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connector.hpp"

#include "genlib/containerutils.hpp"
#include "genlib/readwritehelpers.hpp"

#include <fstream>
#include <time.h>

bool Connector::read(std::istream &stream) {
    connections.clear();
    forwardSegconns.clear();
    backSegconns.clear();

    // n.b., must set displayed attribute as soon as loaded...

    // The metagraph file format uses signed integers for connections
    // therefore we have to first read that vector and then convert
    dXreadwrite::readFromCastIntoVector<int>(stream, connections);

    stream.read(reinterpret_cast<char *>(&segmentAxialref), sizeof(segmentAxialref));

    dXreadwrite::readIntoMap(stream, forwardSegconns);
    dXreadwrite::readIntoMap(stream, backSegconns);

    return true;
}

bool Connector::write(std::ostream &stream) const {
    // n.b., must set displayed attribute as soon as loaded...
    dXreadwrite::writeCastVector<int>(stream, connections);
    stream.write(reinterpret_cast<const char *>(&segmentAxialref), sizeof(segmentAxialref));

    dXreadwrite::writeMap(stream, forwardSegconns);
    dXreadwrite::writeMap(stream, backSegconns);

    return true;
}

/////////////////////////////////////////////////////////////////////////////////

// Cursor extras

size_t Connector::count(int mode) const {
    size_t c = 0;
    switch (mode) {
    case CONN_ALL:
        c = connections.size();
        break;
    case SEG_CONN_ALL:
        c = backSegconns.size() + forwardSegconns.size();
        break;
    case SEG_CONN_FW:
        c = forwardSegconns.size();
        break;
    case SEG_CONN_BK:
        c = backSegconns.size();
        break;
    }
    return c;
}
int Connector::getConnectedRef(const int cursor, const int mode) const {
    int cur = -1;
    if (cursor != -1) {
        const size_t ucursor = static_cast<size_t>(cursor);
        switch (mode) {
        case CONN_ALL:
            if (ucursor < connections.size()) {
                cur = static_cast<int>(connections[ucursor]);
            }
            break;
        case SEG_CONN_ALL:
            if (ucursor < backSegconns.size()) {
                cur = genlib::getMapAtIndex(backSegconns, ucursor)->first.ref;
            } else if (ucursor - backSegconns.size() < forwardSegconns.size()) {
                cur = genlib::getMapAtIndex(forwardSegconns, ucursor - backSegconns.size())
                          ->first.ref;
            }
            break;
        case SEG_CONN_FW:
            if (ucursor < forwardSegconns.size()) {
                cur = genlib::getMapAtIndex(forwardSegconns, ucursor)->first.ref;
            }
            break;
        case SEG_CONN_BK:
            if (ucursor < backSegconns.size()) {
                cur = genlib::getMapAtIndex(backSegconns, ucursor)->first.ref;
            }
            break;
        }
    }
    return cur;
}
int Connector::direction(const int cursor, const int mode) const {
    int direction = 0;
    if (cursor != -1) {
        const size_t ucursor = static_cast<size_t>(cursor);
        switch (mode) {
        case SEG_CONN_ALL:
            if (ucursor < backSegconns.size()) {
                direction = genlib::getMapAtIndex(backSegconns, ucursor)->first.dir;
            } else if (ucursor - backSegconns.size() < forwardSegconns.size()) {
                direction = genlib::getMapAtIndex(forwardSegconns, ucursor - backSegconns.size())
                                ->first.dir;
            }
            break;
        case SEG_CONN_FW:
            direction = genlib::getMapAtIndex(forwardSegconns, ucursor)->first.dir;
            break;
        case SEG_CONN_BK:
            direction = genlib::getMapAtIndex(backSegconns, ucursor)->first.dir;
            break;
        }
    }
    return direction;
}
float Connector::weight(const int cursor, const int mode) const {
    float weight = 0.0f;
    if (cursor != -1) {
        const size_t ucursor = static_cast<size_t>(cursor);
        switch (mode) {
        case SEG_CONN_ALL:
            if (ucursor < backSegconns.size()) {
                weight = genlib::getMapAtIndex(backSegconns, ucursor)->second;
            } else if (ucursor - backSegconns.size() < forwardSegconns.size()) {
                weight =
                    genlib::getMapAtIndex(forwardSegconns, ucursor - backSegconns.size())->second;
            }
            break;
        case SEG_CONN_FW:
            weight = genlib::getMapAtIndex(forwardSegconns, ucursor)->second;
            break;
        case SEG_CONN_BK:
            weight = genlib::getMapAtIndex(backSegconns, ucursor)->second;
            break;
        }
    }
    return weight;
}
