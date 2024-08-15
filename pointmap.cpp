// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Point data

#include "pointmap.h"

#include "attributetable.h"
#include "ngraph.h"
#include "parsers/mapinfodata.h" // for mapinfo interface
#include "salashape.h"
#include "shapemap.h"

#include "genlib/comm.h" // for communicator
#include "genlib/containerutils.h"
#include "genlib/pflipper.h"
#include "genlib/stringutils.h"

#include <cmath>
#include <numeric>
#include <unordered_set>

/////////////////////////////////////////////////////////////////////////////////

PointMap::PointMap(const QtRegion &parentRegion, const std::string &name)
    : AttributeMap(std::unique_ptr<AttributeTable>(new AttributeTable())),
      m_parentRegion(&parentRegion), m_points(0, 0) {
    m_name = name;

    m_cols = 0;
    m_rows = 0;
    m_filled_point_count = 0;

    m_spacing = 0.0;

    m_initialised = false;
    m_blockedlines = false;
    m_processed = false;
    m_boundarygraph = false;

    m_undocounter = 0;
}

void PointMap::copy(const PointMap &sourcemap, bool copypoints, bool copyattributes) {
    m_name = sourcemap.getName();
    m_region = sourcemap.getRegion();
    m_parentRegion = sourcemap.m_parentRegion;

    m_cols = sourcemap.getCols();
    m_rows = sourcemap.getRows();
    m_filled_point_count = sourcemap.getFilledPointCount();

    m_spacing = sourcemap.getSpacing();

    m_initialised = sourcemap.m_initialised;
    m_blockedlines = sourcemap.m_blockedlines;
    m_processed = sourcemap.m_processed;
    m_boundarygraph = sourcemap.m_boundarygraph;

    m_undocounter = sourcemap.m_undocounter;

    m_offset = sourcemap.m_offset;
    m_bottom_left = sourcemap.m_bottom_left;

    if (copypoints || copyattributes) {
        m_points = sourcemap.m_points;
    }
    if (copyattributes) {

        std::vector<size_t> indices(sourcemap.m_attributes->getNumColumns());
        std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            return sourcemap.m_attributes->getColumnName(a) <
                   sourcemap.m_attributes->getColumnName(b);
        });

        for (auto sourceIter = sourcemap.m_attributes->begin();
             sourceIter != sourcemap.m_attributes->end(); sourceIter++) {
            m_attributes->addRow(sourceIter->getKey());
        }
        for (auto idx : indices) {
            auto outcol =
                m_attributes->insertOrResetColumn(sourcemap.m_attributes->getColumnName(idx));
            // n.b. outcol not necessarily the same as incol, although row position in
            // table (j) should match

            auto targetIter = m_attributes->begin();
            for (auto sourceIter = sourcemap.m_attributes->begin();
                 sourceIter != sourcemap.m_attributes->end(); sourceIter++) {
                targetIter->getRow().setValue(outcol, sourceIter->getRow().getValue(idx));
                targetIter++;
            }
        }
    }
}

void PointMap::communicate(time_t &atime, Communicator *comm, size_t record) {
    if (comm) {
        if (qtimer(atime, 500)) {
            if (comm->IsCancelled()) {
                throw Communicator::CancelledException();
            }
            comm->CommPostMessage(Communicator::CURRENT_RECORD, record);
        }
    }
}

bool PointMap::setGrid(double spacing, const Point2f &offset) {
    m_spacing = spacing;
    // note, the internal offset is the offset from the bottom left
    double xoffset = fmod(m_parentRegion->bottom_left.x + offset.x, m_spacing);
    double yoffset = fmod(m_parentRegion->bottom_left.y + offset.y, m_spacing);
    if (xoffset < m_spacing / 2.0)
        xoffset += m_spacing;
    if (xoffset > m_spacing / 2.0)
        xoffset -= m_spacing;
    if (yoffset < m_spacing / 2.0)
        yoffset += m_spacing;
    if (yoffset > m_spacing / 2.0)
        yoffset -= m_spacing;

    m_offset = Point2f(-xoffset, -yoffset);

    if (m_points.size() != 0) {
        m_filled_point_count = 0;
    }
    m_undocounter = 0; // <- reset the undo counter... sorry... once you've done
                       // this you can't undo

    // A grid at the required spacing:
    m_cols = static_cast<size_t>(floor((xoffset + m_parentRegion->width()) / m_spacing + 0.5) + 1);
    m_rows = static_cast<size_t>(floor((yoffset + m_parentRegion->height()) / m_spacing + 0.5) + 1);

    m_bottom_left = Point2f(m_parentRegion->bottom_left.x + m_offset.x,
                            m_parentRegion->bottom_left.y + m_offset.y);

    m_region =
        QtRegion(Point2f(m_bottom_left.x - m_spacing / 2.0, m_bottom_left.y - m_spacing / 2.0),
                 Point2f(m_bottom_left.x + double(m_cols - 1) * m_spacing + m_spacing / 2.0,
                         m_bottom_left.y + double(m_rows - 1) * m_spacing + m_spacing / 2.0));

    m_points = depthmapX::ColumnMatrix<Point>(m_rows, m_cols);
    for (size_t j = 0; j < m_cols; j++) {
        for (size_t k = 0; k < m_rows; k++) {
            m_points(k, j).m_location =
                depixelate(PixelRef(static_cast<short>(j), static_cast<short>(k)));
        }
    }

    m_initialised = true;
    m_blockedlines = false;
    m_processed = false;
    m_boundarygraph = false;

    m_merge_lines.clear();

    return true;
}

bool PointMap::clearAllPoints() {
    for (auto &point : m_points) {
        if (point.filled()) {
            point.set(Point::EMPTY, m_undocounter);
        }
    }
    m_filled_point_count = 0;
    m_merge_lines.clear();
    return true;
}

bool PointMap::clearPointsInRange(PixelRef bl, PixelRef tr, std::set<int> &selSet) {
    if (!m_filled_point_count) {
        return false;
    }

    // This function is a bit messy...
    // each is a slight variation (saves a little time when there's a single
    // selection as opposed to a compound selection someday clean up

    m_undocounter++;

    m_undocounter++;
    for (auto i = bl.x; i <= tr.x; i++) {
        for (auto j = bl.y; j <= tr.y; j++) {
            PixelRef ref(j, i);
            Point &pnt = getPoint(ref);
            if (selSet.find(ref) != selSet.end() || (pnt.m_state & Point::FILLED)) {
                pnt.set(Point::EMPTY, m_undocounter);
                if (!pnt.m_merge.empty()) {
                    PixelRef p = pnt.m_merge;
                    auto &point = getPoint(p);
                    depthmapX::findAndErase(
                        m_merge_lines,
                        PixelRefPair(PixelRef(static_cast<short>(i), static_cast<short>(j)), p));
                    point.m_merge = NoPixel;
                    point.m_state &= ~Point::MERGED;
                }
                m_filled_point_count--;
            }
        }
    }

    return true;
}

bool PointMap::undoPoints() {
    if (!m_undocounter) {
        return false;
    }
    for (auto &p : m_points) {
        if (p.m_undoCounter == m_undocounter) {
            if (p.m_state & Point::FILLED) {
                p.m_state &= ~Point::FILLED;
                p.m_state |= Point::EMPTY;
                p.m_undoCounter = 0; // probably shouldn't set to 0 (can't undo)  Eventually
                                     // will implement 'redo' counter as well
                m_filled_point_count--;
            } else if (p.m_state & Point::EMPTY) {
                p.m_state |= Point::FILLED;
                p.m_state &= ~Point::EMPTY;
                p.m_undoCounter = 0; // probably shouldn't set to 0 (can't undo)  Eventually
                                     // will implement 'redo' counter as well
                m_filled_point_count++;
            }
        }
    }
    m_undocounter--; // reduce undo counter

    return true;
}

// constrain is used to constrain to existing rows / cols
// (not quite the same as constraining to bounding box due to spacing offsets)
PixelRef PointMap::pixelate(const Point2f &p, bool constrain, int scalefactor) const {
    PixelRef ref;

    double spacing = m_spacing / double(scalefactor);
    ref.x = static_cast<short>(floor((p.x - m_bottom_left.x + (m_spacing / 2.0)) / spacing));
    ref.y = static_cast<short>(floor((p.y - m_bottom_left.y + (m_spacing / 2.0)) / spacing));

    if (constrain) {
        if (ref.x < 0)
            ref.x = 0;
        else if (ref.x >= static_cast<short>(m_cols * scalefactor))
            ref.x = static_cast<short>(m_cols * scalefactor) - 1;
        if (ref.y < 0)
            ref.y = 0;
        else if (ref.y >= static_cast<short>(m_rows * scalefactor))
            ref.y = static_cast<short>(m_rows * scalefactor) - 1;
    }

    return ref;
}

void PointMap::addPointsInRegionToSet(const QtRegion &r, std::set<PixelRef> &selSet) {
    auto newSet = getPointsInRegion(r);
    selSet.insert(newSet.begin(), newSet.end());
}

std::set<PixelRef> PointMap::getPointsInRegion(const QtRegion &r) const {
    std::set<PixelRef> selSet;
    auto s_bl = pixelate(r.bottom_left, true);
    auto s_tr = pixelate(r.top_right, true);

    int mask = 0;
    mask |= Point::FILLED;

    for (auto i = s_bl.x; i <= s_tr.x; i++) {
        for (auto j = s_bl.y; j <= s_tr.y; j++) {
            PixelRef ref(i, j);
            if (getPoint(ref).getState() & mask) {
                selSet.insert(ref);
            }
        }
    }
    return selSet;
}

void PointMap::fillLine(const Line &li) {
    PixelRefVector pixels = pixelateLine(li, 1);
    for (size_t j = 0; j < pixels.size(); j++) {
        if (getPoint(pixels[j]).empty()) {
            getPoint(pixels[j]).set(Point::FILLED, m_undocounter);
            m_filled_point_count++;
        }
    }
}

bool PointMap::blockLines(std::vector<Line> &lines) {
    if (!m_initialised || m_points.size() == 0) {
        return false;
    }
    if (m_blockedlines) {
        return true;
    }
    // just ensure lines don't exist to start off with (e.g., if someone's been
    // playing with the visible layers)
    unblockLines();

    // This used to use a packed Linekey (file, layer, line), but
    // would require a key with (file, layer, shaperef, seg) when used with
    // shaperef, so just switched to an integer key:

    for (const auto &line : lines) {
        blockLine(Line(line.start(), line.end()));
    }

    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            Point &pt = getPoint(curs);
            QtRegion viewport = regionate(curs, 1e-10);
            std::vector<Line>::iterator iter = pt.m_lines.begin(), end = pt.m_lines.end();
            for (; iter != end;) {
                if (!iter->crop(viewport)) {
                    // the pixelation is fairly rough to make sure that no point is
                    // missed: this just clears up if any point has been added in error:
                    iter = pt.m_lines.erase(iter);
                    end = pt.m_lines.end();
                } else {
                    ++iter;
                }
            }
        }
    }

    m_blockedlines = true;

    return true;
}

void PointMap::blockLine(const Line &li) {
    std::vector<PixelRef> pixels = pixelateLineTouching(li, 1e-10);
    // touching is generally better for ensuring lines pixelated completely,
    // although it may catch extra points...
    for (size_t n = 0; n < pixels.size(); n++) {
        getPoint(pixels[n]).m_lines.push_back(li);
        getPoint(pixels[n]).setBlock(true);
    }
}

void PointMap::unblockLines(bool clearblockedflag) {
    // just ensure lines don't exist to start off with (e.g., if someone's been
    // playing with the visible layers)
    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            getPoint(curs).m_lines.clear();
            if (clearblockedflag) {
                getPoint(curs).setBlock(false);
            }
        }
    }
}

// still used through pencil tool

bool PointMap::fillPoint(const Point2f &p, bool add) {
    // "false" is do not constrain to bounding box, includes() must be used before
    // getPoint
    PixelRef pix = pixelate(p, false);
    if (!includes(pix)) {
        return false;
    }
    Point &pt = getPoint(pix);
    if (add && !pt.filled()) {
        m_filled_point_count++;
        pt.set(Point::FILLED, ++m_undocounter);
    } else if (!add && (pt.m_state & Point::FILLED)) {
        m_filled_point_count--;
        pt.set(Point::EMPTY, ++m_undocounter);
        if (pt.m_merge != NoPixel) {
            unmergePixel(pix);
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////

// NB --- I've returned to original

// AV TV // semifilled
bool PointMap::makePoints(const Point2f &seed, int fill_type, Communicator *comm) {
    if (!m_initialised || m_points.size() == 0) {
        return false;
    }
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_RECORDS, (m_rows * m_cols));
    }

    // Snap to existing grid
    // "false" is does not constrain: must use includes() before getPoint
    PixelRef seedref = pixelate(seed, false);

    if (!includes(seedref) || getPoint(seedref).filled()) {
        return false;
    }

    // check if seed point is actually visible from the centre of the cell
    std::vector<Line> &linesTouching = getPoint(seedref).m_lines;
    for (const auto &line : linesTouching) {
        if (intersect_line_no_touch(line, Line(seed, getPoint(seedref).m_location))) {
            return false;
        }
    }

    if (!m_blockedlines) {
        throw depthmapX::RuntimeException("blockLines() not called before makePoints");
    }

    m_undocounter++; // undo counter increased ready for fill...

    // AV TV
    // int filltype = fill_type ? Point::FILLED | Point::CONTEXTFILLED :
    // Point::FILLED;
    int filltype;
    if (fill_type == 0) // FULLFILL
        filltype = Point::FILLED;
    else if (fill_type == 1) // SEMIFILL
        filltype = Point::FILLED | Point::CONTEXTFILLED;
    else // AUGMENT
        filltype = Point::AUGMENTED;

    getPoint(seedref).set(filltype, m_undocounter);
    m_filled_point_count++;

    // Now... start making lines:
    pflipper<PixelRefVector> surface;

    surface.a().push_back(seedref);

    size_t added = 0;

    time_t atime = 0;
    qtimer(atime, 0);

    while (surface.a().size() > 0) {
        PixelRef &currpix = surface.a().back();
        int result = 0;
        result |= expand(currpix, currpix.up(), surface.b(), filltype);
        result |= expand(currpix, currpix.down(), surface.b(), filltype);
        result |= expand(currpix, currpix.left(), surface.b(), filltype);
        result |= expand(currpix, currpix.right(), surface.b(), filltype);
        result |= expand(currpix, currpix.up().left(), surface.b(), filltype);
        result |= expand(currpix, currpix.up().right(), surface.b(), filltype);
        result |= expand(currpix, currpix.down().left(), surface.b(), filltype);
        result |= expand(currpix, currpix.down().right(), surface.b(), filltype);
        // if there is a block, mark the currpix as an edge
        if ((result & 4) || getPoint(currpix).blocked()) {
            getPoint(currpix).setEdge();
        }
        //
        surface.a().pop_back();
        if (surface.a().size() == 0) {
            surface.flip();
        }
        added++;
        communicate(atime, comm, added);
    }

    return true;
}

int PointMap::expand(const PixelRef p1, const PixelRef p2, PixelRefVector &list, int filltype) {
    if (p2.x < 0 || p2.x >= static_cast<short>(m_cols) || p2.y < 0 ||
        p2.y >= static_cast<short>(m_rows)) {
        // 1 = off edge
        return 1;
    }
    if (getPoint(p2).getState() & Point::FILLED) {
        // 2 = already filled
        return 2;
    }
    Line l(depixelate(p1), depixelate(p2));
    for (auto &line : getPoint(p1).m_lines) {
        if (intersect_region(l, line, m_spacing * 1e-10) &&
            intersect_line(l, line, m_spacing * 1e-10)) {
            // 4 = blocked
            return 4;
        }
    }
    for (auto &line : getPoint(p2).m_lines) {
        if (intersect_region(l, line, m_spacing * 1e-10) &&
            intersect_line(l, line, m_spacing * 1e-10)) {
            // 4 = blocked
            return 4;
        }
    }
    getPoint(p2).set(filltype, m_undocounter);
    m_filled_point_count++;
    list.push_back(p2);

    // 8 = success
    return 8;
}

void PointMap::outputPoints(std::ostream &stream, char delim) {
    stream << "Ref" << delim << "x" << delim << "y" << std::endl;
    stream.precision(12);

    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {

            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));

            if (getPoint(curs).filled()) {

                Point2f p = depixelate(curs);
                stream << curs << delim << p.x << delim << p.y << std::endl;
            }
        }
    }
}

void PointMap::outputMergeLines(std::ostream &stream, char delim) {
    stream << "x1" << delim << "y1" << delim << "x2" << delim << "y2" << std::endl;

    stream.precision(12);
    for (size_t i = 0; i < m_merge_lines.size(); i++) {

        Line li(depixelate(m_merge_lines[i].a), depixelate(m_merge_lines[i].b));

        stream << li.start().x << delim << li.start().y << delim << li.end().x << delim
               << li.end().y << std::endl;
    }
}

/////////////////////////////////////////////////////////////////////////////////

void PointMap::outputSummary(std::ostream &myout, char delimiter) {
    myout << "Ref" << delimiter << "x" << delimiter << "y";

    // TODO: For compatibility write the columns in alphabetical order
    // but the physical columns in the order inserted

    std::vector<size_t> indices(m_attributes->getNumColumns());
    std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        return m_attributes->getColumnName(a) < m_attributes->getColumnName(b);
    });
    for (auto idx : indices) {
        myout << delimiter << m_attributes->getColumnName(idx);
    }

    myout << std::endl;
    myout.precision(8);

    for (auto iter = m_attributes->begin(); iter != m_attributes->end(); iter++) {
        PixelRef pix = iter->getKey().value;
        if (isObjectVisible(m_layers, iter->getRow())) {
            myout << pix << delimiter;
            Point2f p = depixelate(pix);
            myout << p.x << delimiter << p.y;
            for (auto idx : indices) {
                myout << delimiter << iter->getRow().getValue(idx);
            }
            myout << std::endl;
        }
    }
}

void PointMap::outputMif(std::ostream &miffile, std::ostream &midfile) {
    MapInfoData mapinfodata;
    mapinfodata.exportFile(miffile, midfile, *this);
}

void PointMap::outputNet(std::ostream &netfile) {
    // this is a bid of a faff, as we first have to get the point locations,
    // then the connections from a lookup table... ickity ick ick...
    std::map<PixelRef, PixelRefVector> graph;
    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {
            Point &pnt = m_points(j, i);
            if (pnt.filled() && pnt.m_node) {
                PixelRef pix(static_cast<short>(i), static_cast<short>(j));
                PixelRefVector connections;
                pnt.m_node->contents(connections);
                graph.insert(std::make_pair(pix, connections));
            }
        }
    }
    netfile << "*Vertices " << graph.size() << std::endl;
    double maxdim = fmax(m_region.width(), m_region.height());
    Point2f offset = Point2f((maxdim - m_region.width()) / (2.0 * maxdim),
                             (maxdim - m_region.height()) / (2.0 * maxdim));
    size_t j = 0;
    for (auto &iter : graph) {
        auto graphKey = iter.first;
        Point2f p = depixelate(graphKey);
        p.x = offset.x + (p.x - m_region.bottom_left.x) / maxdim;
        p.y = 1.0 - (offset.y + (p.y - m_region.bottom_left.y) / maxdim);
        netfile << (j + 1) << " \"" << graphKey << "\" " << p.x << " " << p.y << std::endl;
        j++;
    }
    netfile << "*Edges" << std::endl;
    size_t k = 0;
    for (auto &iter : graph) {
        PixelRefVector &list = iter.second;
        for (size_t m = 0; m < list.size(); m++) {
            size_t n = depthmapX::findIndexFromKey(graph, list[m]);
            if (static_cast<int>(n) != -1 && k < n) {
                netfile << (k + 1) << " " << (n + 1) << " 1" << std::endl;
            }
        }
    }
}

void PointMap::outputConnections(std::ostream &myout) {
    myout << "#graph v1.0" << std::endl;
    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {
            Point &pnt = m_points(j, i);
            if (pnt.filled() && pnt.m_node) {
                PixelRef pix(static_cast<short>(i), static_cast<short>(j));
                Point2f p = depixelate(pix);
                myout << "node {\n"
                      << "  ref    " << pix << "\n"
                      << "  origin " << p.x << " " << p.y << " " << 0.0 << "\n"
                      << "  connections [" << std::endl;
                myout << *(pnt.m_node);
                myout << "  ]\n}" << std::endl;
            }
        }
    }
}

void PointMap::outputConnectionsAsCSV(std::ostream &myout, std::string delim) {
    myout << "RefFrom" << delim << "RefTo";
    std::unordered_set<PixelRef, hashPixelRef> seenPix;
    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {
            Point &pnt = m_points(j, i);
            if (pnt.filled() && pnt.m_node) {
                PixelRef pix(static_cast<short>(i), static_cast<short>(j));
                seenPix.insert(pix);
                PixelRefVector hood;
                pnt.m_node->contents(hood);
                for (PixelRef &p : hood) {
                    if (!(std::find(seenPix.begin(), seenPix.end(), p) != seenPix.end()) &&
                        getPoint(p).filled()) {
                        myout << std::endl << pix << delim << p;
                    }
                }
            }
        }
    }
}

void PointMap::outputLinksAsCSV(std::ostream &myout, std::string delim) {
    myout << "RefFrom" << delim << "RefTo";
    std::unordered_set<PixelRef, hashPixelRef> seenPix;
    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {
            Point &pnt = m_points(j, i);
            if (pnt.filled() && pnt.m_node) {
                PixelRef mergePixelRef = pnt.getMergePixel();
                if (mergePixelRef != NoPixel) {
                    PixelRef pix(static_cast<short>(i), static_cast<short>(j));
                    if (seenPix.insert(pix).second) {
                        seenPix.insert(mergePixelRef);
                        myout << std::endl << pix << delim << mergePixelRef;
                    }
                }
            }
        }
    }
}

void PointMap::outputBinSummaries(std::ostream &myout) {
    myout << "cols " << m_cols << " rows " << m_rows << std::endl;

    myout << "x\ty";
    for (int i = 0; i < 32; i++) {
        myout << "\tbin" << i;
    }
    myout << std::endl;

    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {

            Point p = getPoint(PixelRef(static_cast<short>(i), static_cast<short>(j)));

            myout << i << "\t" << j;

            if (!p.filled()) {
                for (int k = 0; k < 32; k++) {
                    myout << "\t" << 0;
                }
            } else {
                for (int k = 0; k < 32; k++) {
                    myout << "\t" << p.m_node->bin(k).count();
                }
            }

            myout << std::endl;
        }
    }
}

// Helper function: is there a blocked point next to you?
// ...rather scruffily goes round the eight adjacent points...

// This is being phased out, with the new "edge" points (which are the filled
// edges of the graph)

bool PointMap::blockedAdjacent(const PixelRef p) const {
    bool ba = false;
    PixelRef temp = p.right();
    PixelRef bounds(static_cast<short>(m_cols), static_cast<short>(m_rows));

    if (bounds.encloses(temp) && getPoint(temp).blocked()) { // Right
        ba = true;
    } else {
        temp = temp.up();
        if (bounds.encloses(temp) && getPoint(temp).blocked()) { // Top right
            ba = true;
        } else {
            temp = temp.left();
            if (bounds.encloses(temp) && getPoint(temp).blocked()) { // Top
                ba = true;
            } else {
                temp = temp.left();
                if (bounds.encloses(temp) && getPoint(temp).blocked()) { // Top Left
                    ba = true;
                } else {
                    temp = temp.down();
                    if (bounds.encloses(temp) && getPoint(temp).blocked()) { // Left
                        ba = true;
                    } else {
                        temp = temp.down();
                        if (bounds.encloses(temp) && getPoint(temp).blocked()) { // Bottom Left
                            ba = true;
                        } else {
                            temp = temp.right();
                            if (bounds.encloses(temp) && getPoint(temp).blocked()) { // Bottom
                                ba = true;
                            } else {
                                temp = temp.right();
                                if (bounds.encloses(temp) &&
                                    getPoint(temp).blocked()) { // Bottom right
                                    ba = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return ba;
}

////////////////////////////////////////////////////////////////////////////////

bool PointMap::readMetadata(std::istream &stream) {
    m_name = dXstring::readString(stream);

    stream.read((char *)&m_spacing, sizeof(m_spacing));

    int rows, cols;
    stream.read(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.read(reinterpret_cast<char *>(&cols), sizeof(cols));
    m_rows = static_cast<size_t>(rows);
    m_cols = static_cast<size_t>(cols);

    stream.read((char *)&m_filled_point_count, sizeof(m_filled_point_count));

    stream.read((char *)&m_bottom_left, sizeof(m_bottom_left));

    m_region =
        QtRegion(Point2f(m_bottom_left.x - m_spacing / 2.0, m_bottom_left.y - m_spacing / 2.0),
                 Point2f(m_bottom_left.x + double(m_cols - 1) * m_spacing + m_spacing / 2.0,
                         m_bottom_left.y + double(m_rows - 1) * m_spacing + m_spacing / 2.0));
    return true;
}

bool PointMap::readPointsAndAttributes(std::istream &stream) {
    m_attributes->read(stream, m_layers);

    m_points = depthmapX::ColumnMatrix<Point>(m_rows, m_cols);

    for (size_t j = 0; j < m_cols; j++) {
        for (size_t k = 0; k < m_rows; k++) {
            m_points(k, j).read(stream);
        }

        for (size_t k = 0; k < m_rows; k++) {
            Point &pnt = m_points(k, j);
            // Old style point node reffing and also unselects selected nodes which
            // would otherwise be difficult

            // would soon be better simply to turn off the select flag....
            pnt.m_state &= (Point::EMPTY | Point::FILLED | Point::MERGED | Point::BLOCKED |
                            Point::CONTEXTFILLED | Point::EDGE);

            // Set the node pixel if it exists:
            if (pnt.m_node) {
                pnt.m_node->setPixel(PixelRef(static_cast<short>(j), static_cast<short>(k)));
            }
            // Add merge line if merged:
            if (!pnt.m_merge.empty()) {
                depthmapX::addIfNotExists(
                    m_merge_lines,
                    PixelRefPair(PixelRef(static_cast<short>(j), static_cast<short>(k)),
                                 pnt.m_merge));
            }
        }
    }

    m_initialised = true;
    m_blockedlines = false;

    stream.read((char *)&m_processed, sizeof(m_processed));
    stream.read((char *)&m_boundarygraph, sizeof(m_boundarygraph));
    return true;
}

std::tuple<bool, int> PointMap::read(std::istream &stream) {
    bool read = readMetadata(stream);

    // sala does not read or write display data by default any more.
    // instead construct another read/write function for a GUI using
    // the functions above and below
    int displayedAttribute;
    stream.read((char *)&displayedAttribute, sizeof(displayedAttribute));

    read = read && readPointsAndAttributes(stream);

    return std::make_tuple(read, displayedAttribute);
}

bool PointMap::writeMetadata(std::ostream &stream) const {
    dXstring::writeString(stream, m_name);

    stream.write((char *)&m_spacing, sizeof(m_spacing));

    int rows = static_cast<int>(m_rows);
    int cols = static_cast<int>(m_cols);
    stream.write(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.write(reinterpret_cast<char *>(&cols), sizeof(cols));

    stream.write((char *)&m_filled_point_count, sizeof(m_filled_point_count));

    stream.write((char *)&m_bottom_left, sizeof(m_bottom_left));
    return true;
}

bool PointMap::writePointsAndAttributes(std::ostream &stream) const {

    m_attributes->write(stream, m_layers);

    for (auto &point : m_points) {
        point.write(stream);
    }

    stream.write((char *)&m_processed, sizeof(m_processed));
    stream.write((char *)&m_boundarygraph, sizeof(m_boundarygraph));
    return true;
}

bool PointMap::write(std::ostream &stream, int displayedAttribute) const {
    bool written = writeMetadata(stream);

    stream.write((char *)&displayedAttribute, sizeof(displayedAttribute));

    written = written && writePointsAndAttributes(stream);
    return written;
}

////////////////////////////////////////////////////////////////////////////////

// Now what this class is actually for: making a visibility graph!

// Visibility graph construction constants

size_t PointMap::tagState(bool settag) {

    size_t count = 0;

    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {

            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));

            // First ensure only one of filled/empty/blocked is on:
            Point &pt = getPoint(curs);
            if (pt.filled()) {
                if (settag) {
                    //                    pt.m_misc = static_cast<int>(count);
                    pt.m_processflag = 0x00FF; // process all quadrants
                } else {
                    //                    pt.m_misc = 0;
                    pt.m_processflag = 0x0000; // reset process flag
                }
                count++;
            }
        }
    }
    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////

#include "sparksieve2.h"

// The fast way to generate graphs... attempt 2
// This uses the new points line segments to allow quick overlap comparisons
// The spark method uses a 1024 long bit string to check against

// writing the algo has thrown up something: it would be more appropriate to
// have lines between the grid points, rather than centred on the grid square,
// i.e.:
//
// .-.     ---
// |X| not |.| (dots are the grid points)
// .-.     ---
//
// Then wouldn't have to 'test twice' for the grid point being blocked...
// ...perhaps a tweak for a later date!

bool PointMap::sparkGraph2(Communicator *comm, bool boundarygraph, double maxdist) {
    // Note, graph must be fixed (i.e., having blocking pixels filled in)

    if (!m_blockedlines) {
        throw depthmapX::RuntimeException("blockLines() not called before makePoints");
    }

    if (boundarygraph) {
        for (size_t i = 0; i < m_cols; i++) {
            for (size_t j = 0; j < m_rows; j++) {
                PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
                if (getPoint(curs).filled() && !getPoint(curs).edge()) {
                    m_points(j, i).m_state &= ~Point::FILLED;
                    m_filled_point_count--;
                }
            }
        }
    }

    // attributes table set up
    // n.b. these must be entered in alphabetical order to preserve col indexing:
    m_attributes->insertOrResetLockedColumn(PointMap::Column::CONNECTIVITY);
    m_attributes->insertOrResetColumn(PointMap::Column::POINT_FIRST_MOMENT);
    m_attributes->insertOrResetColumn(PointMap::Column::POINT_SECOND_MOMENT);

    // pre-label --- allows faster node access later on
    auto count = tagState(true);

    // start the timer when you know the true count including fixed points

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, count);
    }

    count = 0;

    for (size_t i = 0; i < m_cols; i++) {

        for (size_t j = 0; j < m_rows; j++) {

            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));

            auto &point = getPoint(curs);
            if (point.getState() & Point::FILLED) {

                point.m_node = std::unique_ptr<Node>(new Node());
                m_attributes->addRow(AttributeKey(curs));

                sparkPixel2(curs, 1,
                            maxdist); // make flag of 1 suggests make this node, don't
                                      // set reciprocral process flags on those you can
                                      // see maxdist controls how far to see out to

                count++; // <- increment count

                if (comm) {
                    if (qtimer(atime, 500)) {
                        if (comm->IsCancelled()) {
                            tagState(false); // <- the state field has been used for tagging
                                             // visited nodes... set back to a state variable
                            // (well, actually, no it hasn't!)
                            // Should clear all nodes and attributes here:
                            // Clear nodes
                            // Clear attributes
                            m_attributes->clear();
                            //
                            throw Communicator::CancelledException();
                        }
                        comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
                    }
                } // if (comm)
            }     // if ( getPoint( curs ).getState() & Point::FILLED )
        }         // rows
    }             // cols

    tagState(false); // <- the state field has been used for tagging visited
                     // nodes... set back to a state variable

    // keeping lines blocked now is wasteful of memory... free the memory involved
    unblockLines(false);

    // and add grid connections
    // (this is easier than trying to work it out per pixel as we calculate
    // visibility)
    addGridConnections();

    // the graph is processed:
    m_processed = true;
    if (boundarygraph) {
        m_boundarygraph = true;
    }

    return true;
}

bool PointMap::unmake(bool removeLinks) {
    for (size_t i = 0; i < m_cols; i++) {
        for (size_t j = 0; j < m_rows; j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            Point &pnt = getPoint(curs);
            if (pnt.filled()) {
                if (removeLinks) {
                    pnt.m_merge = NoPixel;
                }
                pnt.m_grid_connections = 0;
                pnt.m_node = nullptr;
                pnt.m_lines.clear();
                pnt.setBlock(false);
            }
        }
    }

    m_blockedlines = false;

    if (removeLinks) {
        m_merge_lines.clear();
    }

    m_attributes->clear();

    m_processed = false;
    m_boundarygraph = false;

    return true;
}

// 'make' construct types are:
// 1 -- build this node
// 2 -- register the reciprocal q octant in nodes you can see as requiring
// processing

bool PointMap::sparkPixel2(PixelRef curs, int make, double maxdist) {
    static std::vector<PixelRef> bins_b[32];
    static float far_bin_dists[32];
    for (int i = 0; i < 32; i++) {
        far_bin_dists[i] = 0.0f;
    }
    int neighbourhood_size = 0;
    double total_dist = 0.0;
    double total_dist_sqr = 0.0;

    Point2f centre0 = depixelate(curs);

    for (int q = 0; q < 8; q++) {

        if (!((getPoint(curs).m_processflag) & (1 << q))) {
            continue;
        }

        sparkSieve2 sieve(centre0, maxdist);
        int depth = 0;

        // attempt 0 depth line tests by taken appropriate quadrant
        // from immediately around centre
        // note regionate border must be greater than tolerance squared used in
        // interection testing later
        double border = m_spacing * 1e-10;
        QtRegion viewport0 = regionate(curs, 1e-10);
        switch (q) {
        case 0:
            viewport0.top_right.x = centre0.x;
            viewport0.bottom_left.y = centre0.y - border;
            break;
        case 6:
            viewport0.top_right.x = centre0.x + border;
            viewport0.bottom_left.y = centre0.y;
            break;
        case 1:
            viewport0.bottom_left.x = centre0.x;
            viewport0.bottom_left.y = centre0.y - border;
            break;
        case 7:
            viewport0.bottom_left.x = centre0.x - border;
            viewport0.bottom_left.y = centre0.y;
            break;
        case 2:
            viewport0.top_right.x = centre0.x;
            viewport0.top_right.y = centre0.y + border;
            break;
        case 4:
            viewport0.top_right.x = centre0.x + border;
            viewport0.top_right.y = centre0.y;
            break;
        case 3:
            viewport0.bottom_left.x = centre0.x;
            viewport0.top_right.y = centre0.y + border;
            break;
        case 5:
            viewport0.bottom_left.x = centre0.x - border;
            viewport0.top_right.y = centre0.y;
            break;
        }
        std::vector<Line> lines0;
        for (const Line &line : getPoint(curs).m_lines) {
            Line l = line;
            if (l.crop(viewport0)) {
                lines0.push_back(l);
            }
        }
        sieve.block(lines0, q);
        sieve.collectgarbage();

        std::vector<PixelRef> addlist;

        for (depth = 1; sieve.hasGaps(); depth++) {

            addlist.clear();
            if (!sieve2(sieve, addlist, q, depth, curs)) {
                break;
            }

            for (size_t n = 0; n < addlist.size(); n++) {
                if (getPoint(addlist[n]).getState() & Point::FILLED) {
                    int bin = whichbin(depixelate(addlist[n]) - centre0);
                    if (make & 1) {
                        // the blocked cells shouldn't contribute to point stats
                        // note m_spacing is used to scale the moment of inertia
                        // appropriately
                        double this_dist = dist(addlist[n], curs) * m_spacing;
                        if (this_dist > far_bin_dists[bin]) {
                            far_bin_dists[bin] = (float)this_dist;
                        }
                        total_dist += this_dist;
                        total_dist_sqr += this_dist * this_dist;
                        neighbourhood_size++;

                        bins_b[bin].push_back(addlist[n]);
                    }
                    if (make & 2) {
                        getPoint(addlist[n]).m_processflag |= q_opposite(bin);
                    }
                }
            }

        } // <- for (depth = 1; sieve.hasgaps(); depth++)

    } // <- for (int q = 0; q < 8; q++)

    if (make & 1) {
        // The bins are cleared in the make function!
        Point &pt = getPoint(curs);
        pt.m_node->make(curs, bins_b, far_bin_dists,
                        pt.m_processflag); // note: make clears bins!
        AttributeRow &row = m_attributes->getRow(AttributeKey(curs));
        row.setValue(PointMap::Column::CONNECTIVITY, float(neighbourhood_size));
        row.setValue(PointMap::Column::POINT_FIRST_MOMENT, float(total_dist));
        row.setValue(PointMap::Column::POINT_SECOND_MOMENT, float(total_dist_sqr));
    } else {
        // Clear bins by hand if not using them to make
        for (int i = 0; i < 32; i++) {
            bins_b[i].clear();
        }
    }

    // reset process flag
    getPoint(curs).m_processflag = 0;

    return true;
}

bool PointMap::sieve2(sparkSieve2 &sieve, std::vector<PixelRef> &addlist, int q, int depth,
                      PixelRef curs) {
    bool hasgaps = false;
    int firstind = 0;

    for (auto iter = sieve.m_gaps.begin(); iter != sieve.m_gaps.end(); ++iter) {
        // this goes through all open points
        if (iter->remove) {
            continue;
        }
        for (int ind = (int)ceil(iter->start * (depth - 0.5) - 0.5);
             ind <= (int)floor(iter->end * (depth + 0.5) + 0.5); ind++) {
            if (ind < firstind) {
                continue;
            }
            if (ind > depth) {
                break;
            }
            // this did say first = ind + 1, but I needed to change it to cope with
            // vertical lines I have a feeling the ind + 1 was there for a reason! (if
            // there to cope with boundary graph, could easily simply use ind + 1 in
            // the specific check)
            firstind = ind;

            // x and y are calculated using Grad's whichbin q quadrants
            int x = (q >= 4 ? ind : depth);
            int y = (q >= 4 ? depth : ind);

            PixelRef here = PixelRef(static_cast<short>(curs.x + (q % 2 ? x : -x)),
                                     static_cast<short>(curs.y + (q <= 1 || q >= 6 ? y : -y)));

            if (includes(here)) {
                hasgaps = true;
                // centre gap checks to see if the point is blocked itself
                bool centregap =
                    (double(ind) >= (iter->start * depth) && double(ind) <= (iter->end * depth));

                if (centregap && (getPoint(here).m_state & Point::FILLED)) {
                    // don't repeat axes / diagonals
                    if ((ind != 0 || q == 0 || q == 1 || q == 5 || q == 6) &&
                        (ind != depth || q < 4)) {
                        // block test as usual [tested 31.10.04 -- MUST use 1e-10 for Gassin
                        // at 10 grid spacing]
                        if (!sieve.testblock(depixelate(here), getPoint(here).m_lines,
                                             m_spacing * 1e-10)) {
                            addlist.push_back(here);
                        }
                    }
                }
                sieve.block(getPoint(here).m_lines, q);
            }
        }
    }
    sieve.collectgarbage();

    return hasgaps;
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool PointMap::binDisplay(Communicator *, std::set<int> &selSet) {
    auto bindisplay_col = m_attributes->insertOrResetColumn("Node Bins");

    for (auto &sel : selSet) {
        Point &p = getPoint(sel);
        // Code for colouring pretty bins:
        for (int i = 0; i < 32; i++) {
            Bin &b = p.m_node->bin(i);
            b.first();
            while (!b.is_tail()) {
                // m_attributes->setValue( row, bindisplay_col, float((i % 8) + 1) );
                m_attributes->getRow(AttributeKey(b.cursor()))
                    .setValue(bindisplay_col, float(b.distance()));
                b.next();
            }
        }
    }

    return true;
}

bool PointMap::mergePoints(const Point2f &p, QtRegion &firstPointsBounds,
                           std::set<int> &firstPoints) {

    // note that in a multiple selection, the point p is adjusted by the selection
    // bounds
    PixelRef bl = pixelate(firstPointsBounds.bottom_left);
    PixelRef tr = pixelate(firstPointsBounds.top_right);
    //
    PixelRef offset = pixelate(p) - PixelRef(tr.x, bl.y);
    //
    for (auto &sel : firstPoints) {
        PixelRef a = sel;
        PixelRef b = ((PixelRef)sel) + offset;
        // check in limits:
        if (includes(b) && getPoint(b).filled()) {
            mergePixels(a, b);
        }
    }

    return true;
}

bool PointMap::unmergePoints(std::set<int> &firstPoints) {
    for (auto &sel : firstPoints) {
        PixelRef a = sel;
        Point p = getPoint(a);
        if (p.getMergePixel() != NoPixel) {
            unmergePixel(a);
        }
    }
    return true;
}

// Either of the pixels can be given here and the other will also be unmerged
bool PointMap::unmergePixel(PixelRef a) {
    PixelRef c = getPoint(a).m_merge;
    depthmapX::findAndErase(m_merge_lines, PixelRefPair(a, c));
    getPoint(c).m_merge = NoPixel;
    getPoint(c).m_state &= ~Point::MERGED;
    getPoint(a).m_merge = NoPixel;
    getPoint(a).m_state &= ~Point::MERGED;
    return true;
}

bool PointMap::mergePixels(PixelRef a, PixelRef b) {
    if (a == b && !getPoint(a).m_merge.empty()) {
        unmergePixel(a);
    }
    if (a != b && getPoint(a).m_merge != b) {
        if (!getPoint(a).m_merge.empty()) {
            PixelRef c = getPoint(a).m_merge;
            auto it = std::find(m_merge_lines.begin(), m_merge_lines.end(), PixelRefPair(a, c));
            if (it != m_merge_lines.end())
                m_merge_lines.erase(it);
            getPoint(c).m_merge = NoPixel;
            getPoint(c).m_state &= ~Point::MERGED;
        }
        if (!getPoint(b).m_merge.empty()) {
            PixelRef c = getPoint(b).m_merge;
            auto it = std::find(m_merge_lines.begin(), m_merge_lines.end(), PixelRefPair(b, c));
            if (it != m_merge_lines.end())
                m_merge_lines.erase(it);
            getPoint(c).m_merge = NoPixel;
            getPoint(c).m_state &= ~Point::MERGED;
        }
        getPoint(a).m_merge = b;
        getPoint(a).m_state |= Point::MERGED;
        getPoint(b).m_merge = a;
        getPoint(b).m_state |= Point::MERGED;
        m_merge_lines.push_back(PixelRefPair(a, b));
    }

    // actually this return now triggers redraw whatever
    // rather than passing back altered status (as a point must be deselected)
    return true;
}

void PointMap::mergeFromShapeMap(const ShapeMap &shapemap) {
    const std::map<int, SalaShape> &polygons = shapemap.getAllShapes();
    for (const auto &polygon : polygons) {
        const SalaShape &poly = polygon.second;
        if (poly.isLine()) {
            PixelRef a = pixelate(poly.getLine().start());
            PixelRef b = pixelate(poly.getLine().end());
            if (getPoint(a).filled() && getPoint(b).filled()) {
                mergePixels(a, b);
            }
        }
    }
}

bool PointMap::isPixelMerged(const PixelRef &a) { return !getPoint(a).m_merge.empty(); }

// -2 for point not in visibility graph, -1 for point has no data
double PointMap::getLocationValue(const Point2f &point, std::optional<size_t> columnIdx) {
    double val = -2;

    // "false" does not constrain to bounds
    PixelRef pix = pixelate(point, false);
    // quick check for outside row / col bounds:
    if (!includes(pix)) {
        return val;
    }

    if (!getPoint(pix).filled()) {
        val = -2;
    } else if (!columnIdx.has_value()) {
        val = static_cast<float>(pix);
    } else {
        val = m_attributes->getRow(AttributeKey(pix)).getValue(columnIdx.value());
    }

    return val;
}

/////////////////////////////////////////////////////////////////////////////////
// Update connections will load an old graph and add char information

void PointMap::addGridConnections() {
    for (auto iter = m_attributes->begin(); iter != m_attributes->end(); iter++) {
        PixelRef curs = iter->getKey().value;
        PixelRef node = curs.right();
        Point &point = getPoint(curs);
        point.m_grid_connections = 0;
        for (int i = 0; i < 32; i += 4) {
            Bin &bin = point.m_node->bin(i);
            bin.first();
            while (!bin.is_tail()) {
                if (node == bin.cursor()) {
                    point.m_grid_connections |= (1 << (i / 4));
                    break;
                }
                bin.next();
            }
            char dir = PixelRef::NODIR;
            if (i == 0) {
                dir = PixelRef::VERTICAL;
            } else if (i == 4 || i == 8) {
                dir = PixelRef::NEGHORIZONTAL;
            } else if (i == 12 || i == 16) {
                dir = PixelRef::NEGVERTICAL;
            } else if (i == 20 || i == 24) {
                dir = PixelRef::HORIZONTAL;
            }
            node.move(dir);
        }
    }
}

// value in range 0 to 1
PixelRef PointMap::pickPixel(double value) const {
    int which = static_cast<int>(ceil(value * static_cast<double>(m_rows * m_cols)) - 1);
    return PixelRef(static_cast<short>(static_cast<size_t>(which) % m_cols),
                    static_cast<short>(static_cast<size_t>(which) / m_cols));
}
