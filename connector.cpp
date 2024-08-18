// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connector.h"

#include "genlib/containerutils.h"
#include "genlib/readwritehelpers.h"

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

    stream.read((char *)&segmentAxialref, sizeof(segmentAxialref));

    dXreadwrite::readIntoMap(stream, forwardSegconns);
    dXreadwrite::readIntoMap(stream, backSegconns);

    return true;
}

bool Connector::write(std::ostream &stream) const {
    // n.b., must set displayed attribute as soon as loaded...
    dXreadwrite::writeCastVector<int>(stream, connections);
    stream.write((char *)&segmentAxialref, sizeof(segmentAxialref));

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
int Connector::getConnectedRef(int cursor, int mode) const {
    int cur = -1;
    if (cursor != -1) {
        switch (mode) {
        case CONN_ALL:
            if (cursor < int(connections.size())) {
                cur = connections[size_t(cursor)];
            }
            break;
        case SEG_CONN_ALL:
            if (cursor < int(backSegconns.size())) {
                cur = depthmapX::getMapAtIndex(backSegconns, cursor)->first.ref;
            } else if (size_t(cursor) - backSegconns.size() < forwardSegconns.size()) {
                cur = depthmapX::getMapAtIndex(forwardSegconns, cursor - int(backSegconns.size()))
                          ->first.ref;
            }
            break;
        case SEG_CONN_FW:
            if (cursor < int(forwardSegconns.size())) {
                cur = depthmapX::getMapAtIndex(forwardSegconns, cursor)->first.ref;
            }
            break;
        case SEG_CONN_BK:
            if (cursor < int(backSegconns.size())) {
                cur = depthmapX::getMapAtIndex(backSegconns, cursor)->first.ref;
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
            if (cursor < (int)backSegconns.size()) {
                direction = depthmapX::getMapAtIndex(backSegconns, cursor)->first.dir;
            } else if (size_t(cursor) - backSegconns.size() < forwardSegconns.size()) {
                direction =
                    depthmapX::getMapAtIndex(forwardSegconns, cursor - int(backSegconns.size()))
                        ->first.dir;
            }
            break;
        case SEG_CONN_FW:
            direction = depthmapX::getMapAtIndex(forwardSegconns, cursor)->first.dir;
            break;
        case SEG_CONN_BK:
            direction = depthmapX::getMapAtIndex(backSegconns, cursor)->first.dir;
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
            if (cursor < int(backSegconns.size())) {
                weight = depthmapX::getMapAtIndex(backSegconns, cursor)->second;
            } else if (size_t(cursor) - backSegconns.size() < forwardSegconns.size()) {
                weight =
                    depthmapX::getMapAtIndex(forwardSegconns, cursor - int(backSegconns.size()))
                        ->second;
            }
            break;
        case SEG_CONN_FW:
            weight = depthmapX::getMapAtIndex(forwardSegconns, cursor)->second;
            break;
        case SEG_CONN_BK:
            weight = depthmapX::getMapAtIndex(backSegconns, cursor)->second;
            break;
        }
    }
    return weight;
}
