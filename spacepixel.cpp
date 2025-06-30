// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is my code to make a set of axial lines from a set of boundary lines

// spatial data

#include "spacepixel.hpp"

#include "genlib/readwritehelpers.hpp"
#include "genlib/stringutils.hpp"

#include <cmath>
#include <fstream>
#include <set>

SpacePixel::SpacePixel(const std::string &name)
    : PixelBase(name), m_lock(), m_newline(false), m_show(true), m_edit(false), m_color(),
      m_ref(-1), m_style(0), m_pixelLines(0, 0), m_lines(), m_displayLines(), m_current(0),
      m_test(0) {

    m_cols = 0;
    m_rows = 0;

    m_color = 0;
}

SpacePixel::SpacePixel(const SpacePixel &spacepixel)
    : PixelBase(spacepixel.m_name), m_lock(), m_newline(), m_show(), m_edit(), m_color(), m_ref(),
      m_style(), m_pixelLines(spacepixel.m_pixelLines.rows(), spacepixel.m_pixelLines.columns()),
      m_lines(), m_displayLines(), m_current(), m_test() {
    // n.b., not strictly allowed
    construct(spacepixel);
}

SpacePixel &SpacePixel::operator=(const SpacePixel &spacepixel) {
    if (this != &spacepixel) {
        construct(spacepixel);
    }
    return *this;
}

void SpacePixel::construct(const SpacePixel &spacepixel) {
    m_name = spacepixel.m_name;
    m_show = spacepixel.m_show;
    m_edit = spacepixel.m_edit;

    m_rows = spacepixel.m_rows;
    m_cols = spacepixel.m_cols;

    m_region = spacepixel.m_region;

    m_ref = spacepixel.m_ref;
    m_test = spacepixel.m_test;
    m_lines = spacepixel.m_lines;
    m_newline = true;

    if (!m_rows || !m_cols) {
        m_displayLines.clear();
        return;
    }

    m_pixelLines = spacepixel.m_pixelLines;

    m_color = spacepixel.m_color;
    m_style = spacepixel.m_style;

    // m_pixel_height = spacepixel.m_pixel_height;
    // m_pixel_width  = spacepixel.m_pixel_width;
}

PixelRef SpacePixel::pixelate(const Point2f &p, bool constrain, int) const {
    PixelRef r;

    Point2f p1 = p;
    p1.normalScale(m_region.bottomLeft, m_region.width(), m_region.height());

    r.x = static_cast<short>(p1.x * static_cast<double>(static_cast<double>(m_cols) - 1e-9));
    if (constrain) {
        if (r.x >= static_cast<short>(m_cols))
            r.x = static_cast<short>(m_cols) - 1;
        else if (r.x < 0)
            r.x = 0;
    }
    r.y = static_cast<short>(p1.y * static_cast<double>(static_cast<double>(m_rows) - 1e-9));
    if (constrain) {
        if (r.y >= static_cast<short>(m_rows))
            r.y = static_cast<short>(m_rows) - 1;
        else if (r.y < 0)
            r.y = 0;
    }

    return r;
}

void SpacePixel::initLines(int size, const Point2f &min, const Point2f &max, double density) {
    m_displayLines.clear();
    m_lines.clear();
    m_ref = -1;
    m_test = 0;

    // work out extents...
    m_region = Region4f(min, max);

    if (m_region.height() == 0) {
        m_rows = 1;
    } else {
        double whRatio = m_region.width() / m_region.height();
        m_rows = static_cast<size_t>(sqrt(static_cast<double>(size) * whRatio * density));
        if (m_rows < 1)
            m_rows = 1;
    }

    if (m_region.width() == 0) {
        m_cols = 1;
    } else {
        double hwRatio = m_region.height() / m_region.width();
        m_cols = static_cast<size_t>(sqrt(static_cast<double>(size) * hwRatio * density));
        if (m_cols < 1)
            m_cols = 1;
    }
    // could work these two out on the fly, but it's easier to have them stored:
    // m_pixel_height = m_region.height() / static_cast<double>(m_rows);
    // m_pixel_width  = m_region.width()  / static_cast<double>(m_cols);

    m_pixelLines = depthmapX::RowMatrix<std::vector<int>>(static_cast<size_t>(m_rows),
                                                          static_cast<size_t>(m_cols));
}

void SpacePixel::reinitLines(double density) {
    m_displayLines.clear();

    double whRatio = m_region.width() / m_region.height();
    double hwRatio = m_region.height() / m_region.width();

    m_rows = static_cast<size_t>(sqrt(static_cast<double>(m_lines.size()) * whRatio * density));
    m_cols = static_cast<size_t>(sqrt(static_cast<double>(m_lines.size()) * hwRatio * density));

    if (m_rows < 1)
        m_rows = 1;
    if (m_cols < 1)
        m_cols = 1;

    m_pixelLines = depthmapX::RowMatrix<std::vector<int>>(static_cast<size_t>(m_rows),
                                                          static_cast<size_t>(m_cols));

    // now re-add the lines:
    for (const auto &line : m_lines) {
        PixelRefVector list = pixelateLine(line.second.line);
        for (size_t j = 0; j < list.size(); j++) {
            // note: m_pixelLines will be reordered by sortPixelLines
            m_pixelLines(static_cast<size_t>(list[j].y), static_cast<size_t>(list[j].x))
                .push_back(line.first);
        }
    }

    // and finally sort:
    sortPixelLines();

    // flag as newline just in case:
    m_newline = true;
}

// Add line: pixelate the line

void SpacePixel::addLine(const Line4f &line) {
    // Fairly simple: just pixelates the line!
    m_ref++; // need unique keys for the lines so they can be added / removed at
             // any time
    m_lines.insert(std::make_pair(m_ref, LineTest(line, 0)));
    m_newline = true;

    PixelRefVector list = pixelateLine(line);

    for (size_t i = 0; i < list.size(); i++) {
        // note: m_pixelLines will be reordered by sortPixelLines
        m_pixelLines(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x))
            .push_back(m_ref);
    }
}

int SpacePixel::addLineDynamic(const Line4f &line) {
    m_ref++; // need unique keys for the lines so they can be added / removed at
             // any time
    m_lines.insert(std::make_pair(m_ref, LineTest(line, 0)));
    m_newline = true;

    PixelRefVector list = pixelateLine(line);

    for (size_t i = 0; i < list.size(); i++) {
        // note: dynamic lines could be dodgy... only pixelate bits that fall in
        // range
        if (list[i].x >= 0 && list[i].y >= 0 && static_cast<size_t>(list[i].x) < m_cols &&
            static_cast<size_t>(list[i].y) < m_rows) {
            // note, this probably won't be reordered on dynamic
            m_pixelLines(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x))
                .push_back(m_ref);
        }
    }

    return m_ref;
}

void SpacePixel::sortPixelLines() {
    for (size_t i = 0; i < static_cast<size_t>(m_cols); i++) {
        for (size_t j = 0; j < static_cast<size_t>(m_rows); j++) {
            std::vector<int> &pixelLines = m_pixelLines(j, i);
            // tidy up in case of removal
            for (auto revIter = pixelLines.rbegin(); revIter != pixelLines.rend(); ++revIter) {
                if (m_lines.find(*revIter) == m_lines.end()) {
                    pixelLines.erase(std::next(revIter).base());
                }
            }
            std::sort(pixelLines.begin(), pixelLines.end());
        }
    }
}

bool SpacePixel::intersect(const Line4f &l, double tolerance) {
    m_test++; // note loops! (but vary rarely: inevitabley, lines will have been
              // marked before it loops)

    PixelRefVector list = pixelateLine(l);

    for (size_t i = 0; i < list.size(); i++) {
        auto &pixelLines =
            m_pixelLines(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
        for (int lineref : pixelLines) {
            const auto &lineIt = m_lines.find(lineref);
            if (lineIt == m_lines.end()) {
                throw depthmapX::RuntimeException("Line " + std::to_string(lineref) +
                                                  " not found when looking for intersections");
            }
            LineTest &linetest = lineIt->second;
            if (linetest.test != m_test) {
                if (linetest.line.Region4f::intersects(l, tolerance)) {
                    if (linetest.line.Line4f::intersects(l, tolerance)) {
                        return true;
                    }
                }
                linetest.test = m_test;
            }
        }
    }

    return false;
}

bool SpacePixel::intersect_exclude(const Line4f &l, double tolerance) {
    m_test++; // note loops! (but vary rarely: inevitabley, lines will have been
              // marked before it loops)

    PixelRefVector list = pixelateLine(l);

    for (size_t i = 0; i < list.size(); i++) {
        auto &pixelLines =
            m_pixelLines(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
        for (int lineref : pixelLines) {
            const auto &lineIt = m_lines.find(lineref);
            if (lineIt == m_lines.end()) {
                throw depthmapX::RuntimeException("Line " + std::to_string(lineref) +
                                                  " not found when looking for intersections");
            }
            LineTest &linetest = lineIt->second;
            if (linetest.test != m_test) {
                if (linetest.line.Region4f::intersects(l, tolerance)) {
                    if (linetest.line.Line4f::intersects(l, tolerance)) {
                        if (linetest.line.start() != l.start() &&
                            linetest.line.start() != l.end() && linetest.line.end() != l.start() &&
                            linetest.line.end() != l.end()) {
                            return true;
                        }
                    }
                }
                linetest.test = m_test;
            }
        }
    }

    return false;
}

void SpacePixel::cutLine(Line4f &l, short dir, Communicator *comm) {
    m_test++;

    double tolerance = l.length() * 1e-9;

    std::set<double> loc;
    PixelRefVector vec = pixelateLine(l);

    LineAxis axis;
    if (l.width() >= l.height()) {
        axis = LineAxis::XAXIS;
    } else {
        axis = LineAxis::YAXIS;
    }
    Point2f truestart = (dir == l.direction()) ? l.start() : l.end();
    Point2f trueend = (dir == l.direction()) ? l.end() : l.start();

    bool found = false;
    std::vector<Line4f> touchingLines;

    for (size_t i = 0; i < vec.size() && !found; i++) {
        // depending on direction of line either move head to tail or tail to head
        PixelRef pix = (dir == l.direction()) ? vec[i] : vec[vec.size() - 1 - i];
        auto &pixelLines = m_pixelLines(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
        for (int lineref : pixelLines) {
            const auto &lineIt = m_lines.find(lineref);
            if (lineIt == m_lines.end()) {
                // the lineref may have been deleted -- this is supposed to be tidied up
                // just ignore...
                if (comm)
                    comm->logWarning("cut line exception -- missing line?");
            }
            LineTest &linetest = lineIt->second;
            if (linetest.test != m_test) {
                if (linetest.line.Region4f::intersects(l, tolerance * linetest.line.length())) {
                    switch (linetest.line.Line4f::intersects_distinguish(
                        l, tolerance * linetest.line.length())) {
                    case 0:
                        break;
                    case 2: {
                        loc.insert(l.intersection_point(linetest.line, axis));
                    } break;
                    case 1:
                        if (truestart != linetest.line.start() &&
                            truestart != linetest.line.end()) {
                            if (!touchingLines.size()) {
                                touchingLines.push_back(linetest.line);
                            } else {
                                Point2f a, b;
                                int pair = -1;
                                // if there may be more than one touches in the same pixel, we
                                // have to build a list of possibles...
                                for (size_t k = 0; k < touchingLines.size() && pair == -1; k++) {
                                    if (linetest.line.start() == touchingLines[k].start() ||
                                        linetest.line.end() == touchingLines[k].end()) {
                                        a = linetest.line.end() - linetest.line.start();
                                        pair = static_cast<int>(k);
                                    } else if (linetest.line.start() == touchingLines[k].end() ||
                                               linetest.line.end() == touchingLines[k].start()) {
                                        a = linetest.line.start() - linetest.line.end();
                                        pair = static_cast<int>(k);
                                    }
                                    if (pair != -1) {
                                        b = touchingLines[static_cast<size_t>(pair)].end() -
                                            touchingLines[static_cast<size_t>(pair)].start();
                                        Point2f p = trueend - truestart;
                                        double oa = p.det(a);
                                        double ob = p.det(b);
                                        if (pafmath::sgn(oa) != pafmath::sgn(ob) ||
                                            fabs(oa) < tolerance * linetest.line.length() ||
                                            fabs(ob) < tolerance * linetest.line.length()) {
                                            // crossed
                                            if (fabs(oa) >
                                                tolerance *
                                                    linetest.line
                                                        .length()) { // checks not parallel...
                                                loc.insert(
                                                    l.intersection_point(linetest.line, axis));
                                            } else if (fabs(ob) >
                                                       tolerance * linetest.line.length()) {
                                                loc.insert(l.intersection_point(
                                                    touchingLines[static_cast<size_t>(pair)],
                                                    axis));
                                            } else {
                                                // parallel with both lines ... this shouldn't
                                                // happen...
                                                if (comm)
                                                    comm->logWarning("couldn't chop at boundary");
                                            }
                                        }
                                    }
                                    pair = -1;
                                }
                                touchingLines.push_back(linetest.line);
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }
                linetest.test = m_test;
            }
        }
        if (loc.size()) {
            // there's no guarantee the loc actually happened in this pixel...
            // check the first loc actually occurred in this pixel...
            if ((dir == l.direction() && (axis == LineAxis::XAXIS || l.sign() == 1)) ||
                (dir != l.direction() && (axis == LineAxis::YAXIS && l.sign() == -1))) {
                if (pix == pixelate(l.point_on_line(*loc.begin(), axis))) {
                    found = true;
                }
            } else {
                if (pix == pixelate(l.point_on_line(*loc.rbegin(), axis))) {
                    found = true;
                }
            }
        }
    }

    if (loc.size()) {
        // it intersected...
        double pos;
        if (dir == l.direction()) {
            if (axis == LineAxis::XAXIS) {
                pos = *loc.begin();
                l.by() = l.ay() + l.sign() * l.height() * (pos - l.ax()) / l.width();
                l.bx() = pos;
            } else if (l.sign() == 1) {
                pos = *loc.begin();
                l.bx() = l.ax() + l.width() * (pos - l.ay()) / l.height();
                l.by() = pos;
            } else {
                pos = *loc.rbegin();
                l.bx() = l.ax() + l.width() * (l.ay() - pos) / l.height();
                l.by() = pos;
            }
        } else {
            if (axis == LineAxis::XAXIS) {
                pos = *loc.rbegin();
                l.ay() = l.by() - l.sign() * l.height() * (l.bx() - pos) / l.width();
                l.ax() = pos;
            } else if (l.sign() == 1) {
                pos = *loc.rbegin();
                l.ax() = l.bx() - l.width() * (l.by() - pos) / l.height();
                l.ay() = pos;
            } else {
                pos = *loc.begin();
                l.ax() = l.bx() - l.width() * (pos - l.by()) / l.height();
                l.ay() = pos;
            }
        }
    }
}

bool SpacePixel::read(std::istream &stream) {
    // clear anything that was there:
    m_displayLines.clear();
    m_lines.clear();

    // read name:

    m_name = dXstring::readString(stream);
    stream.read(reinterpret_cast<char *>(&m_show), sizeof(m_show));

    if (m_name.empty()) {
        m_name = "<unknown>";
    }

    m_edit = false; // <- just default to not editable on read

    stream.read(reinterpret_cast<char *>(&m_color), sizeof(m_color));

    // read extents:
    stream.read(reinterpret_cast<char *>(&m_region), sizeof(m_region));

    // read rows / cols
    int rows, cols;
    stream.read(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.read(reinterpret_cast<char *>(&cols), sizeof(cols));
    m_rows = static_cast<size_t>(rows);
    m_cols = static_cast<size_t>(cols);

    // could work these two out on the fly, but it's easier to have them stored:
    // m_pixel_height = m_region.height() / static_cast<double>(m_rows);
    // m_pixel_width  = m_region.width()  / static_cast<double>(m_cols);

    // prepare loader:
    m_pixelLines = depthmapX::RowMatrix<std::vector<int>>(static_cast<size_t>(m_rows),
                                                          static_cast<size_t>(m_cols));

    stream.read(reinterpret_cast<char *>(&m_ref), sizeof(m_ref));
    dXreadwrite::readIntoMap(stream, m_lines);
    // now load into structure:
    int n = -1;
    for (const auto &line : m_lines) {
        n++;

        PixelRefVector list = pixelateLine(line.second.line);

        for (size_t m = 0; m < list.size(); m++) {
            // note: m_pixelLines is an *ordered* list! --- used by other ops.
            m_pixelLines(static_cast<size_t>(list[m].y), static_cast<size_t>(list[m].x))
                .push_back(n);
        }
    }

    return true;
}

bool SpacePixel::write(std::ofstream &stream) {
    // write name:
    dXstring::writeString(stream, m_name);
    stream.write(reinterpret_cast<const char *>(&m_show), sizeof(m_show));
    stream.write(reinterpret_cast<const char *>(&m_color), sizeof(m_color));

    // write extents:
    stream.write(reinterpret_cast<const char *>(&m_region), sizeof(m_region));

    // write rows / cols
    int rows = static_cast<int>(m_rows);
    int cols = static_cast<int>(m_cols);
    stream.write(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.write(reinterpret_cast<char *>(&cols), sizeof(cols));

    // write lines:
    stream.write(reinterpret_cast<const char *>(&m_ref), sizeof(m_ref));

    dXreadwrite::writeMap(stream, m_lines);

    return true;
}
