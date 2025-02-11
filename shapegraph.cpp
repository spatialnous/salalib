// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shapegraph.h"

#include "axialpolygons.h"
#include "parsers/mapinfodata.h"
#include "tolerances.h"

#include "genlib/comm.h" // For communicator
#include "genlib/containerutils.h"
#include "genlib/readwritehelpers.h"

#include <cmath>
#include <float.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////////////////

ShapeGraph::ShapeGraph(const std::string &name, int type)
    : ShapeMap(name, type), m_keyvertexcount(0) {

    m_hasgraph = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeGraph::initialiseAttributesAxial() {
    m_attributes->clear();
    // note, expects these to be numbered 0, 1...
    m_attributes->insertOrResetLockedColumn(ShapeGraph::Column::CONNECTIVITY);
    m_attributes->insertOrResetLockedColumn(ShapeGraph::Column::LINE_LENGTH);
}

void ShapeGraph::makeConnections(const KeyVertices &keyvertices) {
    m_connectors.clear();
    m_links.clear();
    m_unlinks.clear();
    m_keyvertices.clear();

    // note, expects these to be numbered 0, 1...
    auto connCol = m_attributes->getColumnIndex(ShapeGraph::Column::CONNECTIVITY);
    auto lengCol = m_attributes->getColumnIndex(ShapeGraph::Column::LINE_LENGTH);

    size_t i = 0;
    for (const auto &shape : m_shapes) {
        int key = shape.first;
        AttributeRow &row = m_attributes->getRow(AttributeKey(key));
        // all indices should match...
        m_connectors.push_back(Connector());
        m_connectors[i].connections =
            getLineConnections(key, TOLERANCE_B * std::max(m_region.height(), m_region.width()));
        row.setValue(connCol, static_cast<float>(m_connectors[i].connections.size()));
        row.setValue(lengCol, static_cast<float>(shape.second.getLine().length()));
        if (keyvertices.size()) {
            // note: depends on lines being recorded in same order as keyvertices...
            m_keyvertices.push_back(keyvertices[i]);
        }
        i++;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

bool ShapeGraph::outputMifPolygons(std::ostream &miffile, std::ostream &midfile) const {
    // take lines from lines layer and make into regions (using the axial polygons)
    std::vector<Line4f> lines;
    for (const auto &shape : m_shapes) {
        lines.push_back(shape.second.getLine());
    }
    AxialPolygons polygons;
    polygons.init(lines, m_region);

    std::vector<std::vector<Point2f>> newpolygons;
    polygons.makePolygons(newpolygons);

    MapInfoData mapinfodata;
    if (m_hasMapInfoData) {
        mapinfodata.m_coordsys = m_mapinfodata.m_coordsys;
        mapinfodata.m_bounds = m_mapinfodata.m_bounds;
    }
    mapinfodata.exportPolygons(miffile, midfile, newpolygons, m_region);

    return true;
}

void ShapeGraph::outputNet(std::ostream &netfile) const {
    double maxdim = std::max(m_region.width(), m_region.height());
    Point2f offset = Point2f((maxdim - m_region.width()) / (2.0 * maxdim),
                             (maxdim - m_region.height()) / (2.0 * maxdim));
    if (isSegmentMap()) {
        netfile << "*Vertices " << m_shapes.size() * 2 << std::endl;
        int i = -1;
        for (const auto &shape : m_shapes) {
            i++;
            Line4f li = shape.second.getLine();
            Point2f p1 = li.start();
            Point2f p2 = li.end();
            p1.x = offset.x + (p1.x - m_region.bottomLeft.x) / maxdim;
            p2.x = offset.x + (p2.x - m_region.bottomLeft.x) / maxdim;
            p1.y = 1.0 - (offset.y + (p1.y - m_region.bottomLeft.y) / maxdim);
            p2.y = 1.0 - (offset.y + (p2.y - m_region.bottomLeft.y) / maxdim);
            netfile << (i * 2 + 1) << " \"" << i << "a\" " << p1.x << " " << p1.y << std::endl;
            netfile << (i * 2 + 2) << " \"" << i << "b\" " << p2.x << " " << p2.y << std::endl;
        }
        netfile << "*Edges" << std::endl;
        for (size_t si = 0; si < m_shapes.size(); si++) {
            netfile << (si * 2 + 1) << " " << (si * 2 + 2) << " 2" << std::endl;
        }
        netfile << "*Arcs" << std::endl;
        // this makes an assumption about which is the "start" and which is the "end"
        // it works for an automatically converted axial map, I'm not sure it works for others...
        for (size_t j = 0; j < m_connectors.size(); j++) {
            const Connector &conn = m_connectors[j];
            for (auto &segconn : conn.forwardSegconns) {
                SegmentRef ref = segconn.first;
                float weight = segconn.second;
                netfile << (j * 2 + 1) << " " << (ref.ref * 2 + ((ref.dir == 1) ? 1 : 2)) << " "
                        << weight << std::endl;
            }
            for (auto &segconn : conn.backSegconns) {
                SegmentRef ref = segconn.first;
                float weight = segconn.second;
                netfile << (j * 2 + 2) << " " << (ref.ref * 2 + ((ref.dir == 1) ? 1 : 2)) << " "
                        << weight << std::endl;
            }
        }
    } else {
        netfile << "*Vertices " << m_shapes.size() << std::endl;
        int i = -1;
        for (const auto &shape : m_shapes) {
            i++;
            Point2f p = shape.second.getCentroid();
            p.x = offset.x + (p.x - m_region.bottomLeft.x) / maxdim;
            p.y = 1.0 - (offset.y + (p.y - m_region.bottomLeft.y) / maxdim);
            netfile << (i + 1) << " \"" << i << "\" " << p.x << " " << p.y << std::endl;
        }
        netfile << "*Edges" << std::endl;
        for (size_t j = 0; j < m_connectors.size(); j++) {
            const Connector &conn = m_connectors[j];
            for (size_t k = 0; k < conn.connections.size(); k++) {
                auto to = conn.connections[k];
                if (j < to) {
                    netfile << (j + 1) << " " << (to + 1) << " 1" << std::endl;
                }
            }
        }
    }
}

bool ShapeGraph::readShapeGraphData(std::istream &stream) {

    m_attributes->clear();
    m_connectors.clear();
    m_mapType = ShapeMap::EMPTYMAP;

    // note that keyvertexcount and keyvertices are different things! (length keyvertices not the
    // same as keyvertexcount!)
    stream.read(reinterpret_cast<char *>(&m_keyvertexcount), sizeof(m_keyvertexcount));
    int size;
    stream.read(reinterpret_cast<char *>(&size), sizeof(size));
    for (int i = 0; i < size; i++) {
        std::vector<int> tempVec;
        dXreadwrite::readIntoVector(stream, tempVec);
        m_keyvertices.push_back(std::set<int>(tempVec.begin(), tempVec.end()));
    }
    return true;
}

std::tuple<bool, bool, bool, int> ShapeGraph::read(std::istream &stream) {
    bool read = readShapeGraphData(stream);
    // now base class read:
    auto shapeMapReadResult = ShapeMap::read(stream);
    std::get<0>(shapeMapReadResult) = read && std::get<0>(shapeMapReadResult);

    return shapeMapReadResult;
}

bool ShapeGraph::writeShapeGraphData(std::ostream &stream) const {
    // note keyvertexcount and keyvertices are different things!  (length keyvertices not the same
    // as keyvertexcount!)
    stream.write(reinterpret_cast<const char *>(&m_keyvertexcount), sizeof(m_keyvertexcount));
    auto size = m_keyvertices.size();
    stream.write(reinterpret_cast<const char *>(&size), sizeof(static_cast<int>(size)));
    for (size_t i = 0; i < m_keyvertices.size(); i++) {
        dXreadwrite::writeVector(
            stream, std::vector<int>(m_keyvertices[i].begin(), m_keyvertices[i].end()));
    }
    return true;
}

bool ShapeGraph::write(std::ostream &stream, const std::tuple<bool, bool, int> &displayData) const {
    bool written = writeShapeGraphData(stream);
    // now simply run base class write:
    written = written & ShapeMap::write(stream, displayData);

    return written;
}

void ShapeGraph::writeAxialConnectionsAsDotGraph(std::ostream &stream) {
    const std::vector<Connector> &connectors = ShapeMap::getConnections();

    stream << "strict graph {" << std::endl;

    stream.precision(12);

    for (size_t i = 0; i < connectors.size(); i++) {
        const auto &connections = connectors[i].connections;
        for (auto connection : connections) {
            stream << "    " << i << " -- " << connection << std::endl;
        }
    }
    stream << "}" << std::endl;
}

void ShapeGraph::writeLinksUnlinksAsPairsCSV(std::ostream &stream, char delimiter) {
    stream.precision(12);

    stream << "refA" << delimiter << "refB" << delimiter << "link" << std::endl;

    for (auto &link : m_links) {
        stream << depthmapX::getMapAtIndex(m_shapes, static_cast<size_t>(link.a))->first
               << delimiter
               << depthmapX::getMapAtIndex(m_shapes, static_cast<size_t>(link.b))->first
               << delimiter << "1" << std::endl;
    }

    for (auto &unlink : m_unlinks) {
        stream << depthmapX::getMapAtIndex(m_shapes, static_cast<size_t>(unlink.a))->first
               << delimiter
               << depthmapX::getMapAtIndex(m_shapes, static_cast<size_t>(unlink.b))->first
               << delimiter << "0" << std::endl;
    }
}

void ShapeGraph::writeAxialConnectionsAsPairsCSV(std::ostream &stream) {
    const std::vector<Connector> &connectors = ShapeMap::getConnections();

    stream.precision(12);

    stream << "refA,refB" << std::endl;

    for (size_t i = 0; i < connectors.size(); i++) {
        auto &connections = connectors[i].connections;
        if (i != 0)
            stream << std::endl;
        for (auto iter = connections.begin(); iter != connections.end(); ++iter) {
            if (iter != connections.begin())
                stream << std::endl;
            stream << i << "," << *iter;
        }
    }
}

void ShapeGraph::writeSegmentConnectionsAsPairsCSV(std::ostream &stream) {
    const std::vector<Connector> &connectors = ShapeMap::getConnections();

    stream.precision(12);

    stream << "refA,refB,ss_weight,for_back,dir";

    // directed links
    for (size_t i = 0; i < connectors.size(); i++) {
        for (auto &segconn : connectors[i].forwardSegconns) {
            stream << std::endl;
            stream << i << "," << segconn.first.ref << "," << segconn.second << "," << 0 // forward
                   << "," << static_cast<int>(segconn.first.dir);
        }

        for (auto &segconn : connectors[i].backSegconns) {
            stream << std::endl;
            stream << i << "," << segconn.first.ref << "," << segconn.second << "," << 1 // back
                   << "," << static_cast<int>(segconn.first.dir);
        }
    }
}

void ShapeGraph::unlinkAtPoint(const Point2f &unlinkPoint) {
    std::vector<Point2f> closepoints;
    std::vector<std::pair<size_t, size_t>> intersections;
    PixelRef pix = pixelate(unlinkPoint);
    std::vector<ShapeRef> &pixShapes =
        m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    auto iter = pixShapes.begin();
    for (; iter != pixShapes.end(); ++iter) {
        for (auto jter = iter; jter != pixShapes.end(); ++jter) {
            auto aIter = m_shapes.find(static_cast<int>(iter->shapeRef));
            auto bIter = m_shapes.find(static_cast<int>(jter->shapeRef));
            auto a = static_cast<size_t>(std::distance(m_shapes.begin(), aIter));
            auto b = static_cast<size_t>(std::distance(m_shapes.begin(), bIter));
            auto &connections = m_connectors[static_cast<size_t>(a)].connections;
            if (aIter != m_shapes.end() && bIter != m_shapes.end() && aIter->second.isLine() &&
                bIter->second.isLine() &&
                std::find(connections.begin(), connections.end(), b) != connections.end()) {
                closepoints.push_back(aIter->second.getLine().intersection_point(
                    bIter->second.getLine(), TOLERANCE_A));
                intersections.push_back(std::make_pair(a, b));
            }
        }
    }
    double mindist = -1.0;
    int minpair = -1;
    int j = 0;
    for (auto &closepoint : closepoints) {
        if (minpair == -1 || unlinkPoint.dist(closepoint) < mindist) {
            mindist = unlinkPoint.dist(closepoint);
            minpair = j;
        }
        j++;
    }
    if (minpair != -1) {
        auto &intersection = intersections[static_cast<size_t>(minpair)];
        unlinkShapes(intersection.first, intersection.second);
    } else {
        std::cerr << "eek!";
    }
}
////////////////////////////////////////////////////////////////////////////

// this unlink options was originally excised on the version 7 recode
// however, it is *very specific* to axial maps, and so have been reincluded here

void ShapeGraph::unlinkFromShapeMap(const ShapeMap &shapemap) {
    // used to make a shape map from every axial intersection,

    // find lines in rough vincinity of unlink point, and check for the closest
    // pair to unlink:

    const std::map<int, SalaShape> &polygons = shapemap.getAllShapes();
    for (const auto &polygon : polygons) {
        // just use the points:
        if (polygon.second.isPoint()) {
            unlinkAtPoint(polygon.second.getPoint());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

// Two ways to make a segment map

// Method 1: direct linkage of endpoints where they touch

void ShapeGraph::makeNewSegMap(Communicator *comm) {
    // now make a connection set from the ends of lines:
    struct LineConnector {
        const Line4f &line;
        Connector &connector;
        int index;
        LineConnector(const Line4f &lineIn, Connector &connectorIn, int indexIn)
            : line(lineIn), connector(connectorIn), index(indexIn) {}
    };

    std::vector<Connector> connectionset;
    for (auto &shape : m_shapes) {
        if (shape.second.isLine()) {
            connectionset.emplace_back();
        }
    }
    std::map<size_t, LineConnector> lineConnectors;
    auto connectionIter = connectionset.begin();
    int connectionIdx = 0;
    for (auto &shape : m_shapes) {
        if (shape.second.isLine()) {
            lineConnectors.insert(
                std::make_pair(shape.first, LineConnector(shape.second.getLine(), *connectionIter,
                                                          connectionIdx)));
            connectionIter++;
            connectionIdx++;
        }
    }

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, lineConnectors.size());
    }

    double maxdim = std::max(m_region.width(), m_region.height());

    size_t count = 0;
    for (auto &lineConnectorA : lineConnectors) {
        Connector &connectionsetA = lineConnectorA.second.connector;
        const Line4f &lineA = lineConnectorA.second.line;
        int idxA = lineConnectorA.second.index;
        // n.b., vector() is based on t_start and t_end, so we must use t_start and t_end here and
        // throughout
        PixelRef pix1 = pixelate(lineA.t_start());
        std::vector<ShapeRef> &shapes1 =
            m_pixelShapes(static_cast<size_t>(pix1.y), static_cast<size_t>(pix1.x));
        for (auto &shape : shapes1) {
            auto lineConnectorB = lineConnectors.find(shape.shapeRef);
            if (lineConnectorB != lineConnectors.end() && idxA < lineConnectorB->second.index) {

                Connector &connectionsetB = lineConnectorB->second.connector;
                const Line4f &lineB = lineConnectorB->second.line;
                int idxB = lineConnectorB->second.index;

                Point2f alpha = lineA.vector();
                Point2f beta = lineB.vector();
                alpha.normalise();
                beta.normalise();
                if (lineA.t_start().approxeq(lineB.t_start(), (maxdim * TOLERANCE_B))) {
                    auto x = static_cast<float>(
                        2.0 * acos(std::min(std::max(-alpha.dot(beta), -1.0), 1.0)) / M_PI);
                    depthmapX::addIfNotExists(connectionsetA.backSegconns, SegmentRef(1, idxB), x);
                    depthmapX::addIfNotExists(connectionsetB.backSegconns, SegmentRef(1, idxA), x);
                }
                if (lineA.t_start().approxeq(lineB.t_end(), (maxdim * TOLERANCE_B))) {
                    auto x = static_cast<float>(
                        2.0 * acos(std::min(std::max(-alpha.dot(-beta), -1.0), 1.0)) / M_PI);
                    depthmapX::addIfNotExists(connectionsetA.backSegconns, SegmentRef(-1, idxB), x);
                    depthmapX::addIfNotExists(connectionsetB.forwardSegconns, SegmentRef(1, idxA),
                                              x);
                }
            }
        }

        PixelRef pix2 = pixelate(lineA.t_end());
        std::vector<ShapeRef> &shapes2 =
            m_pixelShapes(static_cast<size_t>(pix2.y), static_cast<size_t>(pix2.x));
        for (auto &shape : shapes2) {
            auto lineConnectorB = lineConnectors.find(shape.shapeRef);
            if (lineConnectorB != lineConnectors.end() && idxA < lineConnectorB->second.index) {

                Connector &connectionsetB = lineConnectorB->second.connector;
                const Line4f &lineB = lineConnectorB->second.line;
                int idxB = lineConnectorB->second.index;

                Point2f alpha = lineA.vector();
                Point2f beta = lineB.vector();

                alpha.normalise();
                beta.normalise();
                if (lineA.t_end().approxeq(lineB.t_start(), (maxdim * TOLERANCE_B))) {
                    auto x = static_cast<float>(
                        2.0 * acos(std::min(std::max(-(-alpha).dot(beta), -1.0), 1.0)) / M_PI);
                    depthmapX::addIfNotExists(connectionsetA.forwardSegconns, SegmentRef(1, idxB),
                                              x);
                    depthmapX::addIfNotExists(connectionsetB.backSegconns, SegmentRef(-1, idxA), x);
                }
                if (lineA.t_end().approxeq(lineB.t_end(), (maxdim * TOLERANCE_B))) {
                    auto x = static_cast<float>(
                        2.0 * acos(std::min(std::max(-(-alpha).dot(-beta), -1.0), 1.0)) / M_PI);
                    depthmapX::addIfNotExists(connectionsetA.forwardSegconns, SegmentRef(-1, idxB),
                                              x);
                    depthmapX::addIfNotExists(connectionsetB.forwardSegconns, SegmentRef(-1, idxA),
                                              x);
                }
            }
        }

        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
            }
        }
        count++;
    }

    // initialise attributes now separated from making the connections
    makeSegmentConnections(connectionset);
}

// Method 2: Making a segment map (in two stages)

// One: take the original axial map and split it up
// (note: you need to start from an axial map,
//  but the map could have been created from a road-centre-line
//  graph or equivalent -- reason is that you might want to
//  preserve unlinks in your angular mapping)

// A "linetest" is used in order to use the test component to
// identify the original axial line this line segment is
// associated with

void ShapeGraph::makeSegmentMap(std::vector<Line4f> &lines, std::vector<Connector> &connectors,
                                double stubremoval) {
    // the first (key) pair is the line / line intersection, second is the pair of associated
    // segments for the first line
    std::map<OrderedIntPair, std::pair<int, int>> segmentlist;

    // this code relies on the polygon order being the same as the connections

    auto iter = m_shapes.begin();
    for (size_t i = 0; i < m_connectors.size(); i++) {
        auto shape = iter->second;
        int axialRef = iter->first;
        iter++;
        if (!shape.isLine()) {
            continue;
        }
        const Line4f &line = shape.getLine();
        std::vector<std::pair<double, int>> breaks; // this is a vector instead of a map because the
                                                    // original code allowed for duplicate keys
        LineAxis axis = line.width() >= line.height() ? LineAxis::XAXIS : LineAxis::YAXIS;
        // we need the breaks ordered from start to end of the line
        // this is automatic for XAXIS, but on YAXIS, need to know
        // if the line is ascending or decending
        int parity = (axis == LineAxis::XAXIS) ? 1 : line.sign();

        auto &connections = m_connectors[i].connections;
        for (size_t j = 0; j < connections.size(); j++) {
            // find the intersection point and add...
            // note: more than one break at the same place allowed
            auto shapeJ =
                depthmapX::getMapAtIndex(m_shapes, static_cast<size_t>(connections[j]))->second;
            if (i != connections[j] && shapeJ.isLine()) {
                breaks.push_back(std::make_pair(
                    parity * line.intersection_point(shapeJ.getLine(), axis, TOLERANCE_A),
                    connections[j]));
            }
        }
        std::sort(breaks.begin(), breaks.end());
        // okay, now we have a list from one end of the other of lines this line connects with
        Point2f lastpoint = line.start();
        int segA = -1, segB = -1;
        double neardist;
        // TOLERANCE_C is introduced as of 01.08.2008 although it is a fix to a bug first
        // found in July 2006.  It has been set "high" deliberately (1e-6 = a millionth of the line
        // height / width) in order to catch small errors made by operators or floating point errors
        // in other systems when drawing, for example, three axial lines intersecting
        if (stubremoval == 0.0) {
            // if 0, convert to tolerance
            stubremoval = TOLERANCE_C;
        }
        neardist = (axis == LineAxis::XAXIS) ? (line.width() * stubremoval)
                                             : (line.height() * stubremoval);
        double overlapdist = (axis == LineAxis::XAXIS) ? (line.width() * TOLERANCE_C)
                                                       : (line.height() * TOLERANCE_C);
        //
        for (auto breaksIter = breaks.begin(); breaksIter != breaks.end();) {
            std::vector<int> keylist;
            if (segA == -1) {
                Point2f thispoint = line.point_on_line(parity * breaksIter->first, axis);
                if (fabs(parity * breaksIter->first - line.start()[axis]) < neardist) {
                    segA = -1;
                    lastpoint = thispoint;
                } else {
                    Line4f segmentA(line.start(), thispoint);
                    lines.push_back(segmentA);
                    connectors.push_back(Connector(axialRef));
                    segA = static_cast<int>(lines.size()) - 1;
                }
                lastpoint = thispoint;
            }
            //
            double here = parity * breaksIter->first;
            while (breaksIter != breaks.end() &&
                   fabs(parity * breaksIter->first - here) < overlapdist) {
                keylist.push_back(breaksIter->second);
                ++breaksIter;
            }
            //
            if (breaksIter == breaks.end() &&
                fabs(line.end()[axis] - parity * breaks.rbegin()->first) < neardist) {
                segB = -1;
            } else {
                Point2f thispoint;
                if (breaksIter != breaks.end()) {
                    thispoint = line.point_on_line(parity * breaksIter->first, axis);
                } else {
                    thispoint = line.end();
                }
                Line4f segmentB(lastpoint, thispoint);
                lines.push_back(segmentB);
                connectors.push_back(Connector(axialRef));
                segB = static_cast<int>(lines.size()) - 1;
                //
                lastpoint = thispoint;
            }
            //
            for (size_t j = 0; j < keylist.size(); j++) {
                //
                if (keylist[j] < static_cast<int>(i)) {
                    // other line already segmented, look up in segment list,
                    // and join segments together nicely
                    auto segIter =
                        segmentlist.find(OrderedIntPair(keylist[j], static_cast<int>(i)));

                    if (segIter !=
                        segmentlist.end()) { // <- if it isn't -1 something has gone badly wrong!
                        int seg1 = segIter->second.first;
                        int seg2 = segIter->second.second;
                        if (segA != -1) {
                            if (seg1 != -1) {
                                Point2f alpha = lines[static_cast<size_t>(segA)].start() -
                                                lines[static_cast<size_t>(segA)].end();
                                Point2f beta = lines[static_cast<size_t>(seg1)].start() -
                                               lines[static_cast<size_t>(seg1)].end();
                                alpha.normalise();
                                beta.normalise();
                                auto x = static_cast<float>(
                                    2.0 * acos(std::min(std::max(-alpha.dot(beta), -1.0), 1.0)) /
                                    M_PI);
                                depthmapX::addIfNotExists(
                                    connectors[static_cast<size_t>(segA)].forwardSegconns,
                                    SegmentRef(-1, seg1), x);
                                depthmapX::addIfNotExists(
                                    connectors[static_cast<size_t>(seg1)].forwardSegconns,
                                    SegmentRef(-1, segA), x);
                            }
                            if (seg2 != -1) {
                                Point2f alpha = lines[static_cast<size_t>(segA)].start() -
                                                lines[static_cast<size_t>(segA)].end();
                                Point2f beta = lines[static_cast<size_t>(seg2)].end() -
                                               lines[static_cast<size_t>(seg2)].start();
                                alpha.normalise();
                                beta.normalise();
                                auto x = static_cast<float>(
                                    2.0 * acos(std::min(std::max(-alpha.dot(beta), -1.0), 1.0)) /
                                    M_PI);
                                depthmapX::addIfNotExists(
                                    connectors[static_cast<size_t>(segA)].forwardSegconns,
                                    SegmentRef(1, seg2), x);
                                depthmapX::addIfNotExists(
                                    connectors[static_cast<size_t>(seg2)].backSegconns,
                                    SegmentRef(-1, segA), x);
                            }
                        }
                        if (segB != -1) {
                            if (seg1 != -1) {
                                Point2f alpha = lines[static_cast<size_t>(segB)].end() -
                                                lines[static_cast<size_t>(segB)].start();
                                Point2f beta = lines[static_cast<size_t>(seg1)].start() -
                                               lines[static_cast<size_t>(seg1)].end();
                                alpha.normalise();
                                beta.normalise();
                                auto x = static_cast<float>(
                                    2.0 * acos(std::min(std::max(-alpha.dot(beta), -1.0), 1.0)) /
                                    M_PI);
                                depthmapX::addIfNotExists(
                                    connectors[static_cast<size_t>(segB)].backSegconns,
                                    SegmentRef(-1, seg1), x);
                                depthmapX::addIfNotExists(
                                    connectors[static_cast<size_t>(seg1)].forwardSegconns,
                                    SegmentRef(1, segB), x);
                            }
                            if (seg2 != -1) {
                                Point2f alpha = lines[static_cast<size_t>(segB)].end() -
                                                lines[static_cast<size_t>(segB)].start();
                                Point2f beta = lines[static_cast<size_t>(seg2)].end() -
                                               lines[static_cast<size_t>(seg2)].start();
                                alpha.normalise();
                                beta.normalise();
                                auto x = static_cast<float>(
                                    2.0 * acos(std::min(std::max(-alpha.dot(beta), -1.0), 1.0)) /
                                    M_PI);
                                depthmapX::addIfNotExists(
                                    connectors[static_cast<size_t>(segB)].backSegconns,
                                    SegmentRef(1, seg2), x);
                                depthmapX::addIfNotExists(
                                    connectors[static_cast<size_t>(seg2)].backSegconns,
                                    SegmentRef(1, segB), x);
                            }
                        }
                    }
                } else {
                    // other line still to be segmented, add ourselves to segment list
                    // to be added later
                    segmentlist.insert(
                        std::make_pair(OrderedIntPair(static_cast<int>(i), keylist[j]),
                                       std::pair<int, int>(segA, segB)));
                }
            }
            if (segA != -1 && segB != -1) {
                depthmapX::addIfNotExists(connectors[static_cast<size_t>(segA)].forwardSegconns,
                                          SegmentRef(1, segB), 0.0f);
                depthmapX::addIfNotExists(connectors[static_cast<size_t>(segB)].backSegconns,
                                          SegmentRef(-1, segA), 0.0f);
            }
            segA = segB;
        }
    }
}

void ShapeGraph::initialiseAttributesSegment() {
    m_attributes->clear();

    // note, expects these in alphabetical order to preserve numbering:
    m_attributes->insertOrResetLockedColumn(Column::AXIAL_LINE_REF);
    m_attributes->insertOrResetLockedColumn(Column::SEGMENT_LENGTH);
}

// now segments and connections are listed separately...
// put them together in a new map

void ShapeGraph::makeSegmentConnections(std::vector<Connector> &connectionset) {
    m_connectors.clear();

    // note, expects these in alphabetical order to preserve numbering:
    auto wConnCol = m_attributes->getOrInsertColumn(Column::ANGULAR_CONNECTIVITY);
    auto uwConnCol = m_attributes->getOrInsertLockedColumn(Column::CONNECTIVITY);

    auto refCol = m_attributes->getColumnIndex(Column::AXIAL_LINE_REF);
    auto lengCol = m_attributes->getColumnIndex(Column::SEGMENT_LENGTH);

    int i = -1;
    for (const auto &shape : m_shapes) {
        i++;
        Connector &connector = connectionset[static_cast<size_t>(i)];
        AttributeRow &row = m_attributes->getRow(AttributeKey(shape.first));

        row.setValue(refCol, static_cast<float>(connector.segmentAxialref));
        row.setValue(lengCol, static_cast<float>(shape.second.getLine().length()));

        // all indices should match... (including lineset/connectionset versus m_shapes)
        m_connectors.push_back(connector);
        float totalWeight = 0.0f;
        for (auto iter = connector.forwardSegconns.begin(); iter != connector.forwardSegconns.end();
             ++iter) {
            totalWeight += iter->second;
        }
        for (auto iter = connector.backSegconns.begin(); iter != connector.backSegconns.end();
             ++iter) {
            totalWeight += iter->second;
        }
        row.setValue(wConnCol, static_cast<float>(totalWeight));
        row.setValue(uwConnCol, static_cast<float>(connector.forwardSegconns.size() +
                                                   connector.backSegconns.size()));

        // free up connectionset as we go along:
        connectionset[static_cast<size_t>(i)] = Connector();
    }
}

// this pushes axial map values to a segment map
// the segment map is 'this', the axial map is passed:

void ShapeGraph::pushAxialValues(ShapeGraph &axialmap) {
    if (!m_attributes->hasColumn(Column::AXIAL_LINE_REF)) {
        // this should never happen
        // AT: I am converting this to throw an error
        throw depthmapX::RuntimeException("Axial line ref does not exist");
    }

    std::vector<size_t> colindices;
    for (size_t i = 0; i < axialmap.m_attributes->getNumColumns(); i++) {
        std::string colname = std::string("Axial ") + axialmap.m_attributes->getColumnName(i);
        colindices.push_back(m_attributes->getOrInsertColumn(colname));
    }
    for (auto iter = m_attributes->begin(); iter != m_attributes->end(); iter++) {
        int axialref = static_cast<int>(iter->getRow().getValue(Column::AXIAL_LINE_REF));
        // P.K: The original code here got the index of the row, but the column
        // "Axial Line Ref" should actually contain keys, not indices
        AttributeRow &row = axialmap.m_attributes->getRow(AttributeKey(axialref));
        for (size_t k = 0; k < axialmap.m_attributes->getNumColumns(); k++) {
            float val = row.getValue(k);
            // need to look up the column index:
            iter->getRow().setValue(colindices[k], val);
        }
    }
}
