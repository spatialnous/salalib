// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "axialpolygons.hpp"

#include "tidylines.hpp"
#include "tolerances.hpp"

#include "genlib/containerutils.hpp"

AxialVertex AxialPolygons::makeVertex(const AxialVertexKey &vertexkey, const Point2f &openspace) {
    auto vertPossIter =
        depthmapX::getMapAtIndex(vertexPossibles, static_cast<size_t>(vertexkey.refKey));
    AxialVertex av(vertexkey, vertPossIter->first, openspace);

    // n.b., at this point, vertex key m_a and m_b are unfixed
    std::vector<Point2f> &pointlist = vertPossIter->second;
    if (pointlist.size() < 2) {
        return av;
    }

    Point2f o = av.point - av.openspace;

    // using an anglemap means that there are now no anti-clockwise vertices...
    // TODO: (CS) Double as key is problematic - books have been written about
    // double equality...
    std::map<double, int> anglemap;
    for (size_t i = 0; i < pointlist.size(); ++i) {
        anglemap.insert(
            std::make_pair(openspace.angle(av.point, pointlist[i]), static_cast<int>(i)));
    }

    av.refA = static_cast<short>(anglemap.begin()->second);
    // TODO: is this supposed to be av.m_ref_b?
    av.refA = static_cast<short>(anglemap.rbegin()->second);
    Point2f a = av.point - pointlist[static_cast<size_t>(anglemap.begin()->second)];
    Point2f b = pointlist[static_cast<size_t>(anglemap.rbegin()->second)] - av.point;
    av.a = a;
    av.b = b;
    a.normalise();
    b.normalise();

    double oa = o.det(a);
    double ob = o.det(b);
    double ab = a.det(b);

    // can't handle these cases
    if (fabs(oa) < TOLERANCE_A || fabs(ob) < TOLERANCE_A || fabs(ab) < TOLERANCE_A) {
        // although note that if ab == 0 and you've already checked intersection, it
        // can't be convex
        return av;
    }

    // ADDED 4-Nov-04 -- In order to stop too many lines being generated, don't
    // include points that do not change surface direction:  -- notice: will
    // create problems with circles
    if (fabs(a.dot(b)) > 0.999) {
        return av;
    }

    if (pafmath::sgn(oa) == pafmath::sgn(ob)) {
        // headon collision
        if (pafmath::sgn(oa) == 1) {
            if (pafmath::sgn(ab) == 1) {
                // convex clockwise
                av.convex = true;
                av.clockwise = true;
                av.axial = true;
            } else {
                // n.b., these are turned away for axial formation
                // concave clockwise
                av.convex = false;
                av.clockwise = true;
                av.axial = false;
            }
        }
    } else {
        // glancing blow
        // concave clockwise
        av.convex = false;
        av.clockwise = true;
        av.axial = true;
    }

    av.initialised = true;

    return av;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void AxialPolygons::clear() {
    // clear any existing data
    vertexPossibles.clear();
    m_vertexPolys.clear();
    handledList.clear();
    m_pixelPolys.reset(0, 0);
}

void AxialPolygons::init(std::vector<Line4f> &lines, const Region4f &region) {
    // init pixelbase members
    m_region = region;

    // now tidy
    TidyLines tidier;
    tidier.tidy(lines, m_region);

    // for easier debugging, the axial code is reused to make segments
    ShapeGraph firstpass;
    firstpass.init(lines.size(), m_region); // used to be double density
    firstpass.initialiseAttributesAxial();
    size_t i;
    for (i = 0; i < lines.size(); i++) {
        firstpass.makeLineShape(lines[i]);
    }
    firstpass.makeConnections();

    lines.clear();
    std::vector<Connector> connectionset;

    // interesting... 1.0 may or may not work as intended
    firstpass.makeSegmentMap(lines, connectionset, 1.0);

    // now we have a set of lines and a set of connections...
    // ...for the second pass, a bit of retro fitting to my original code is
    // required
    makeVertexPossibles(lines, connectionset);

    initLines(static_cast<int>(lines.size()), m_region.bottomLeft, m_region.topRight, 2);
    // need to init before making pixel polys...
    makePixelPolys();
    // now also add lines
    for (auto &vertexPoss : vertexPossibles) {
        for (auto poss : vertexPoss.second) {
            addLine(Line4f(vertexPoss.first, poss));
        }
    }
    sortPixelLines();
}

void AxialPolygons::makeVertexPossibles(const std::vector<Line4f> &lines,
                                        const std::vector<Connector> &connectionset) {
    vertexPossibles.clear();
    m_vertexPolys.clear();

    size_t i = 0;

    // TODO: (CS) these should be vectors, not raw pointers.
    depthmapX::RowMatrix<int> found(2, lines.size());
    for (i = 0; i < lines.size(); i++) {
        found(0, i) = -1;
        found(1, i) = -1;
    }
    std::vector<Point2f> pointlookup;
    // three pass operation: (1) stack the lines
    for (i = 0; i < lines.size(); i++) {
        if (found(0, i) == -1) {
            pointlookup.push_back(lines[i].start());
            vertexPossibles.insert(std::make_pair(pointlookup.back(), std::vector<Point2f>()));
            m_vertexPolys.push_back(
                -1); // <- n.b., dummy entry for now, maintain with vertex possibles
            found(0, i) = static_cast<int>(pointlookup.size() - 1);
            for (auto &segconn : connectionset[i].backSegconns) {
                int forwback = (segconn.first.dir == 1) ? 0 : 1;
                found(static_cast<size_t>(forwback), static_cast<size_t>(segconn.first.ref)) =
                    found(0, i);
            }
        }
        if (found(1, i) == -1) {
            pointlookup.push_back(lines[i].end());
            vertexPossibles.insert(std::make_pair(pointlookup.back(), std::vector<Point2f>()));
            m_vertexPolys.push_back(
                -1); // <- n.b., dummy entry for now, maintain with vertex possibles
            found(1, i) = static_cast<int>(pointlookup.size() - 1);
            for (auto &segconn : connectionset[i].forwardSegconns) {
                int forwback = (segconn.first.dir == 1) ? 0 : 1;
                found(static_cast<size_t>(forwback), static_cast<size_t>(segconn.first.ref)) =
                    found(1, i);
            }
        }
    }
    // three pass operation: (2) connect up vertex possibles
    for (i = 0; i < lines.size(); i++) {
        if (found(0, i) == -1 || found(1, i) == -1) {
            // TODO: (CS) What are these integers being thrown?!
            throw 1;
        }
        auto index0 = vertexPossibles.find(pointlookup[static_cast<size_t>(found(0, i))]);
        auto index1 = vertexPossibles.find(pointlookup[static_cast<size_t>(found(1, i))]);
        if (index0 == vertexPossibles.end() || index1 == vertexPossibles.end()) {
            // TODO: (CS) What are these integers being thrown?!
            throw 2;
        }

        index0->second.push_back(pointlookup[static_cast<size_t>(found(1, i))]);
        index1->second.push_back(pointlookup[static_cast<size_t>(found(0, i))]);
    }
    for (auto &possible : vertexPossibles) {
        sort(possible.second.begin(), possible.second.end());
        possible.second.erase(unique(possible.second.begin(), possible.second.end()),
                              possible.second.end());
    }

    // three pass operation: (3) create vertex poly entries
    int currentPoly = -1;
    for (i = 0; i < vertexPossibles.size(); i++) {
        if (m_vertexPolys[i] == -1) {
            currentPoly++;
            std::vector<int> addlist;
            addlist.push_back(static_cast<int>(i));
            while (addlist.size()) {
                m_vertexPolys[static_cast<size_t>(addlist.back())] = currentPoly;
                std::vector<Point2f> &connections =
                    depthmapX::getMapAtIndex(vertexPossibles, static_cast<size_t>(addlist.back()))
                        ->second;
                addlist.pop_back();
                for (size_t j = 0; j < connections.size(); j++) {
                    auto index = depthmapX::findIndexFromKey(vertexPossibles, connections[j]);
                    if (index == -1) {
                        throw 3;
                    }
                    if (m_vertexPolys[static_cast<size_t>(index)] == -1) {
                        addlist.push_back(static_cast<int>(index));
                    }
                }
            }
        }
    }
}

void AxialPolygons::makePixelPolys() {
    // record all of this onto the pixel polygons

    m_pixelPolys = depthmapX::ColumnMatrix<std::vector<int>>(m_rows, m_cols);
    // now register the vertices in each pixel...
    int j = -1;
    for (const auto &vertPoss : vertexPossibles) {
        j++;
        PixelRef pix = pixelate(vertPoss.first);
        m_pixelPolys(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x)).push_back(j);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

// almost identical to original!

AxialVertexKey AxialPolygons::seedVertex(const Point2f &seed) {
    AxialVertexKey seedvertex = NoVertex;
    PixelRef seedref = pixelate(seed);
    bool foundvertex = false;
    // for spiralling outwards to find a vertex:
    int dir = PixelRef::HORIZONTAL;
    int sidelength = 1;
    int runlength = 0;
    int allboundaries = 0;

    while (!foundvertex) {
        for (int vertexref :
             m_pixelPolys(static_cast<size_t>(seedref.y), static_cast<size_t>(seedref.x))) {
            const Point2f &trialpoint =
                depthmapX::getMapAtIndex(vertexPossibles, static_cast<size_t>(vertexref))->first;
            if (!intersect_exclude(Line4f(seed, trialpoint))) {
                // yay... ...but wait... we need to see if it's a proper polygon vertex
                // first...
                seedvertex = vertexref;
                foundvertex = true;
            }
        }
        if (!foundvertex) {
            seedref = seedref.move(static_cast<int8_t>(dir));
            // spiral outwards:
            if (++runlength == sidelength) {
                switch (dir) {
                case PixelRef::HORIZONTAL:
                    dir = PixelRef::VERTICAL;
                    runlength = 0;
                    break;
                case PixelRef::VERTICAL:
                    dir = PixelRef::NEGHORIZONTAL;
                    runlength = 0;
                    sidelength++;
                    break;
                case PixelRef::NEGHORIZONTAL:
                    dir = PixelRef::NEGVERTICAL;
                    runlength = 0;
                    break;
                case PixelRef::NEGVERTICAL:
                    dir = PixelRef::HORIZONTAL;
                    runlength = 0;
                    sidelength++;
                    break;
                }
            }
            // check to make sure not off edge of system:
            if (seedref.x < 0) {
                allboundaries |= 0x01;
                seedref.x = 0;
            }
            if (seedref.y < 0) {
                allboundaries |= 0x02;
                seedref.y = 0;
            }
            if (seedref.x >= static_cast<short>(m_cols)) {
                allboundaries |= 0x04;
                seedref.x = static_cast<short>(m_cols - 1);
            }
            if (seedref.y >= static_cast<short>(m_rows)) {
                allboundaries |= 0x08;
                seedref.y = static_cast<short>(m_rows - 1);
            }
            if (allboundaries == 0x0f) {
                return NoVertex;
            }
        }
    }
    return seedvertex;
}

// adds any axial lines from this point to the list of lines, adds any unhandled
// visible vertices it finds to the openvertices list axial lines themselves are
// added to the lines list - the axial line is only there to record the key
// vertices that comprise the line
void AxialPolygons::makeAxialLines(std::set<AxialVertex> &openvertices, std::vector<Line4f> &lines,
                                   KeyVertices &keyvertices,
                                   std::vector<PolyConnector> &polyConnections,
                                   std::vector<RadialLine> &radialLines) {
    auto it = openvertices.rbegin();
    AxialVertex vertex = *it;
    openvertices.erase(std::next(it).base());

    handledList.insert(vertex);

    int i = -1;
    for (const auto &vertPoss : vertexPossibles) {
        i++;
        if (i == vertex.refKey) {
            continue;
        }
        bool possible = false, stubpossible = false;
        Point2f p = vertPoss.first - vertex.point;
        if (vertex.convex) {
            if (vertex.a.det(p) > 0 && vertex.b.det(p) > 0) {
                possible = true;
            }
        } else {
            // left of b and right of a or left of a and right of b
            if (p.det(vertex.a) * p.det(vertex.b) < 0) {
                possible = true;
            } else if (p.det(vertex.a) < TOLERANCE_A && p.det(vertex.b) < TOLERANCE_A) {
                stubpossible = true;
            }
        }
        if (possible || stubpossible) {
            Line4f line(vertPoss.first, vertex.point);
            if (!intersect_exclude(line)) {
                AxialVertex nextVertex = makeVertex(AxialVertexKey(i), vertex.point);
                if (nextVertex.initialised && std::find(handledList.begin(), handledList.end(),
                                                        nextVertex) == handledList.end()) {
                    openvertices.insert(nextVertex); // <- note, add ignores duplicate adds (each
                                                     // vertex tends to be added multiple times
                                                     // before this vertex is handled itself)
                    bool shortlineSegend = false;
                    Line4f shortline = line;
                    if (!vertex.convex && possible) {
                        Line4f ext(line.t_end(), line.t_end() + (line.t_end() - line.t_start()));
                        ext.ray(1, m_region);
                        cutLine(ext, 1);
                        line = Line4f(line.t_start(), ext.t_end());
                        // for radial line segend calc:
                        if ((-p).det(vertex.b) < 0) {
                            shortlineSegend = true;
                        }
                    }
                    if (m_vertexPolys[static_cast<size_t>(vertex.refKey)] !=
                        m_vertexPolys[static_cast<size_t>(
                            nextVertex.refKey)]) { // must be on separate
                                                   // polygons
                        // radial line(s) (for new point)
                        RadialLine radialshort(nextVertex, shortlineSegend, vertex.point,
                                               nextVertex.point, nextVertex.point + nextVertex.b);
                        polyConnections.push_back(
                            PolyConnector(shortline, static_cast<RadialKey>(radialshort)));
                        radialLines.push_back(radialshort);
                        if (!vertex.convex && possible) {
                            Line4f longline = Line4f(vertPoss.first, line.t_end());
                            RadialLine radiallong(radialshort);
                            radiallong.segend = shortlineSegend ? 0 : 1;
                            polyConnections.push_back(
                                PolyConnector(longline, static_cast<RadialKey>(radiallong)));
                            radialLines.push_back(radiallong);
                        }
                    }
                    shortlineSegend = false;
                    if (!nextVertex.convex && nextVertex.axial) {
                        Line4f ext(line.t_start() - (line.t_end() - line.t_start()),
                                   line.t_start());
                        ext.ray(0, m_region);
                        cutLine(ext, 0);
                        line = Line4f(ext.t_start(), line.t_end());
                        // for radial line segend calc:
                        if (p.det(nextVertex.b) < 0) {
                            shortlineSegend = true;
                        }
                    }
                    if (m_vertexPolys[static_cast<size_t>(vertex.refKey)] !=
                        m_vertexPolys[static_cast<size_t>(
                            nextVertex.refKey)]) { // must be on separate
                                                   // polygons
                        // radial line(s) (for original point)
                        RadialLine radialshort(vertex, shortlineSegend, nextVertex.point,
                                               vertex.point, vertex.point + vertex.b);
                        polyConnections.push_back(
                            PolyConnector(shortline, static_cast<RadialKey>(radialshort)));
                        radialLines.push_back(radialshort);
                        if (!nextVertex.convex && nextVertex.axial) {
                            Line4f longline = Line4f(line.t_start(), vertex.point);
                            RadialLine radiallong(radialshort);
                            radiallong.segend = shortlineSegend ? 0 : 1;
                            polyConnections.push_back(
                                PolyConnector(longline, static_cast<RadialKey>(radiallong)));
                            radialLines.push_back(radiallong);
                        }
                    }
                    if (possible && nextVertex.axial) {
                        // axial line
                        lines.push_back(line);
                        keyvertices.push_back(std::set<int>());
                        if (vertex.convex) {
                            keyvertices.back().insert(vertex.refKey);
                        }
                        if (nextVertex.convex) {
                            keyvertices.back().insert(nextVertex.refKey);
                        }
                    }
                }
            }
        }
    }
    std::sort(radialLines.begin(), radialLines.end());
    radialLines.erase(std::unique(radialLines.begin(), radialLines.end()), radialLines.end());
}

///////////////////////////////////////////////////////////////////////////////////////////

// not really used as yet, a feature to make all the polygons from the vertex
// possibles list

void AxialPolygons::makePolygons(std::vector<std::vector<Point2f>> &polygons) {
    std::vector<std::vector<int>> newHandledList;
    for (size_t j = 0; j < vertexPossibles.size(); j++) {
        newHandledList.push_back(std::vector<int>());
    }

    int i = -1;
    for (const auto &vertPoss : vertexPossibles) {
        i++;
        std::vector<int> &currList = newHandledList[static_cast<size_t>(i)];
        if (vertPoss.second.size() == 1) {
            continue;
        }
        for (size_t j = 0; j < vertPoss.second.size(); j++) {
            if (std::find(currList.begin(), currList.end(), static_cast<int>(j)) !=
                currList.end()) {
                continue;
            }
            currList.push_back(static_cast<int>(j));
            const Point2f &key = vertPoss.first;
            std::vector<Point2f> polygon;
            polygon.push_back(key);
            Point2f curr = vertPoss.second.at(j);
            Point2f last = key;
            while (curr != key) {
                auto vertPossIter = vertexPossibles.find(curr);
                if (vertPossIter == vertexPossibles.end()) {
                    throw depthmapX::RuntimeException("Point " + std::to_string(curr.x) + ", " +
                                                      std::to_string(curr.y) +
                                                      " not found when making polygons");
                }
                polygon.push_back(curr);
                // hunt down left most
                int winner = -1, wayback = -1;
                double minangle = 2 * M_PI;
                for (size_t k = 0; k < vertPossIter->second.size(); k++) {
                    Point2f next = vertPossIter->second.at(k);
                    if (last != next) {
                        double thisangle = last.angle(curr, next);
                        if (thisangle < minangle) {
                            // check not going to a dead end:
                            if (vertexPossibles.find(vertPossIter->second.at(k))->second.size() >
                                1) {
                                minangle = thisangle;
                                winner = static_cast<int>(k);
                            }
                        }
                    } else {
                        wayback = static_cast<int>(k);
                    }
                }
                if (winner == -1) {
                    // this happens when you follow a false trail -- go back the way you
                    // came!
                    if (wayback < 0) {
                        throw new depthmapX::RuntimeException(
                            "Error traversing when making polygons");
                    }
                    winner = wayback;
                }
                newHandledList[static_cast<size_t>(
                                   std::distance(vertexPossibles.begin(), vertPossIter))]
                    .push_back(winner);
                last = curr;
                curr = vertPossIter->second.at(static_cast<size_t>(winner));
            }
            polygons.push_back(std::move(polygon));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

bool RadialLine::cuts(const Line4f &l) const {
    if (fabs((l.end() - keyvertex).det(l.end() - l.start())) < TOLERANCE_A) {
        // point on line, check that openspace and next vertex are on opposite sides
        // of the line
        Point2f x = l.end() - keyvertex;
        Point2f y = nextvertex - keyvertex;
        Point2f z = openspace - keyvertex;
        x.normalise();
        y.normalise();
        z.normalise();
        if (pafmath::sgn(x.det(y)) == pafmath::sgn(x.det(z)) && fabs(x.det(z)) > TOLERANCE_A) {
            return false;
        }
    }
    // keyvertex not on line... the line's been cut:
    return true;
}
