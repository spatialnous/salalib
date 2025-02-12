// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ngraph.hpp"

#include "pointmap.hpp"

#include "genlib/containerutils.hpp"
#include "genlib/readwritehelpers.hpp"

void Node::make(const PixelRef pix, PixelRefVector *bins, float *binFarDists, int qOctants) {
    m_pixel = pix;

    for (int i = 0; i < 32; i++) {

        if (qOctants != 0x00FF) {
            // now, an octant filter has been used... note that the exact q-octants
            // that will have been processed rely on adjacenies in the q_octants...
            if (!(qOctants & processoctant(i))) {
                continue;
            }
        }

        m_bins[i].m_distance = binFarDists[i];

        if (i == 4 || i == 20) {
            m_bins[i].make(bins[i], PixelRef::POSDIAGONAL);
        } else if (i == 12 || i == 28) {
            m_bins[i].make(bins[i], PixelRef::NEGDIAGONAL);
        } else if ((i > 4 && i < 12) || (i > 20 && i < 28)) {
            m_bins[i].make(bins[i], PixelRef::VERTICAL);
        } else {
            m_bins[i].make(bins[i], PixelRef::HORIZONTAL);
        }
        // Now clear the bin!
        bins[i].clear();
    }
}

// based on extract metric

bool Node::concaveConnected() {
    // not quite correct -- sometimes at corners you 'see through' the very first
    // connection but a useful approximation: to be concave connected, you need
    // less than 3 in a row somewhere:
    unsigned int test = 0;
    // note wraps around
    test |= (m_bins[0].count()) ? 0 : 0x101;
    test |= (m_bins[4].count()) ? 0 : 0x202;
    test |= (m_bins[8].count()) ? 0 : 0x404;
    test |= (m_bins[12].count()) ? 0 : 0x808;
    test |= (m_bins[16].count()) ? 0 : 0x010;
    test |= (m_bins[20].count()) ? 0 : 0x020;
    test |= (m_bins[24].count()) ? 0 : 0x040;
    test |= (m_bins[28].count()) ? 0 : 0x080;
    if (test != 0) {
        for (int i = 0; i < 8; i++) {
            if (((~test) & 1) && (test & 4) && ((~test) & 12)) { // less than 3 in a row test
                return true;
            }
            test >>= 1;
        }
    }
    return false;
}

bool Node::fullyConnected() {
    // not quite correct -- sometimes at corners you 'see through' the very first
    // connection
    return (m_bins[0].count() && m_bins[4].count() && m_bins[8].count() && m_bins[12].count() &&
            m_bins[16].count() && m_bins[20].count() && m_bins[24].count() && m_bins[28].count());
}

//////////////////////////////////////////////////////////////////////////////////

bool Node::containsPoint(const PixelRef pixel) const {
    bool found = false;
    int start, end;

    // This should really calculate which bin it ought to be in, but for now,
    // we'll reduce by quadrant:
    if (pixel.x > m_pixel.x) {
        if (pixel.y >= m_pixel.y) {
            start = 0;
            end = 7;
        } else {
            start = 25;
            end = 31;
        }
    } else {
        if (pixel.y > m_pixel.y) {
            start = 8;
            end = 15;
        } else {
            start = 16;
            end = 24;
        }
    }
    for (int i = start; i <= end; i++) {
        if (m_bins[i].containsPoint(pixel)) {
            found = true;
            break;
        }
    }
    return found;
}

//////////////////////////////////////////////////////////////////////////////////

void Node::first() const {
    m_curbin = 0;
    do {
        m_bins[m_curbin].first();
    } while (m_bins[m_curbin].is_tail() && ++m_curbin < 32);
}

void Node::next() const {
    m_bins[m_curbin].next();
    while (m_bins[m_curbin].is_tail() && ++m_curbin < 32) {
        m_bins[m_curbin].first();
    }
}

bool Node::is_tail() const { return m_curbin == 32; }

PixelRef Node::cursor() const { return m_bins[m_curbin].cursor(); }

void Node::contents(PixelRefVector &hood) const {
    first();
    while (!is_tail()) {
        depthmapX::addIfNotExists(hood, cursor());
        next();
    }
}

//////////////////////////////////////////////////////////////////////////////////

std::istream &Node::read(std::istream &stream) {
    int i;
    for (i = 0; i < 32; i++) {
        m_bins[i].read(stream);
    }

    for (i = 0; i < 32; i++) {
        dXreadwrite::readIntoVector(stream, occlusionBins[i]);
    }

    return stream;
}

std::ostream &Node::write(std::ostream &stream) {
    int i;
    for (i = 0; i < 32; i++) {
        m_bins[i].write(stream);
    }

    for (i = 0; i < 32; i++) {
        dXreadwrite::writeVector(stream, occlusionBins[i]);
    }
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const Node &node) {
    for (int i = 0; i < 32; i++) {
        if (node.m_bins[i].count()) {
            stream << "    " << node.m_bins[i] << std::endl;
        }
    }
    return stream;
}

///////////////////////////////////////////////////////////////////////////////////////

void Bin::make(const PixelRefVector &pixels, int8_t onDir) {
    pixelVecs.clear();
    m_nodeCount = 0;

    if (pixels.size()) {

        dir = onDir;

        if (dir & PixelRef::DIAGONAL) {

            PixelVec cur(pixels[0], pixels[0]);

            // Special, the diagonal should be pixels directly along the diagonal
            // Both posdiagonal and negdiagonal are positive in the x direction
            // Note that it is ordered anyway, so no need for anything too fancy:
            if (pixels.back().x < cur.start().x) {
                cur.setStart(pixels.back());
            }
            if (pixels.back().x > cur.end().x) {
                cur.setEnd(pixels.back());
            }

            pixelVecs.push_back(cur);
            m_nodeCount = static_cast<unsigned short>(pixels.size());
        } else {
            // Reorder the pixels:
            if (dir == PixelRef::HORIZONTAL) {
                std::set<PixelRefH> pixelsH;
                for (size_t i = 0; i < pixels.size(); i++) {
                    pixelsH.insert(PixelRefH(pixels[i]));
                }
                // this looks like a simple bubble sort
                auto curr = pixelsH.begin();
                pixelVecs.push_back(PixelVec(*curr, *curr));
                ++curr;
                auto prev = pixelsH.begin();
                for (; curr != pixelsH.end(); ++curr) {
                    if (prev->y != curr->y || prev->x + 1 != curr->x) {
                        pixelVecs.back().setEnd(*prev);
                        pixelVecs.push_back(PixelVec(*curr, *curr));
                    }
                    prev = curr;
                }
                pixelVecs.back().setEnd(*pixelsH.rbegin());
            }
            if (dir == PixelRef::VERTICAL) {
                std::set<PixelRefV> pixelsV;
                for (size_t i = 0; i < pixels.size(); i++) {
                    pixelsV.insert(PixelRefV(pixels[i]));
                }
                // this looks like a simple bubble sort
                auto curr = pixelsV.begin();
                pixelVecs.push_back(PixelVec(*curr, *curr));
                ++curr;
                auto prev = pixelsV.begin();
                for (; curr != pixelsV.end(); ++curr) {
                    if (prev->x != curr->x || prev->y + 1 != curr->y) {
                        pixelVecs.back().setEnd(*prev);
                        pixelVecs.push_back(PixelVec(*curr, *curr));
                    }
                    prev = curr;
                }
                pixelVecs.back().setEnd(*pixelsV.rbegin());
            }

            m_nodeCount = static_cast<unsigned short>(pixels.size());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////

bool Bin::containsPoint(const PixelRef p) const {
    for (auto pixVec : pixelVecs) {
        if (dir & PixelRef::DIAGONAL) {
            // note abs is only allowed if you have pre-checked you are in the right
            // quadrant!
            if (p.x >= pixVec.start().x && p.x <= pixVec.end().x &&
                abs(p.y - pixVec.start().y) == p.x - pixVec.start().x) {
                return true;
            }
        } else {
            if (p.row(dir) == pixVec.start().row(dir) && p.col(dir) >= pixVec.start().col(dir) &&
                p.col(dir) <= pixVec.end().col(dir)) {
                return true;
            }
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////

void Bin::first() const {
    m_curvec = 0;
    if (!pixelVecs.empty())
        m_curpix = pixelVecs[static_cast<size_t>(m_curvec)].start();
}

void Bin::next() const {
    if (m_curpix.move(dir).col(dir) > pixelVecs[static_cast<size_t>(m_curvec)].end().col(dir)) {
        m_curvec++;
        if (m_curvec < static_cast<int>(pixelVecs.size()))
            m_curpix = pixelVecs[static_cast<size_t>(m_curvec)].start();
    }
}

bool Bin::is_tail() const { return m_curvec >= static_cast<int>(pixelVecs.size()); }

PixelRef Bin::cursor() const { return static_cast<int>(m_curpix); }

///////////////////////////////////////////////////////////////////////////////////////

std::istream &Bin::read(std::istream &stream) {
    stream.read(reinterpret_cast<char *>(&dir), sizeof(dir));
    stream.read(reinterpret_cast<char *>(&m_nodeCount), sizeof(m_nodeCount));

    stream.read(reinterpret_cast<char *>(&m_distance), sizeof(m_distance));
    stream.read(reinterpret_cast<char *>(&m_occDistance), sizeof(m_occDistance));

    if (m_nodeCount) {
        if (dir & PixelRef::DIAGONAL) {
            pixelVecs = std::vector<PixelVec>(1);
            pixelVecs[0].read(stream, dir);
        } else {
            unsigned short length;
            stream.read(reinterpret_cast<char *>(&length), sizeof(length));
            pixelVecs = std::vector<PixelVec>(length);
            pixelVecs[0].read(stream, dir);
            for (size_t i = 1; i < length; i++) {
                pixelVecs[i].read(stream, dir, pixelVecs[i - 1]);
            }
        }
    }

    return stream;
}

std::ostream &Bin::write(std::ostream &stream) {
    stream.write(reinterpret_cast<const char *>(&dir), sizeof(dir));
    stream.write(reinterpret_cast<const char *>(&m_nodeCount), sizeof(m_nodeCount));

    stream.write(reinterpret_cast<const char *>(&m_distance), sizeof(m_distance));
    stream.write(reinterpret_cast<const char *>(&m_occDistance), sizeof(m_occDistance));

    if (m_nodeCount) {

        if (dir & PixelRef::DIAGONAL) {
            pixelVecs[0].write(stream, dir);
        } else {
            // TODO: Remove this limitation in the next version of the .graph format
            auto length = static_cast<unsigned short>(pixelVecs.size());
            stream.write(reinterpret_cast<const char *>(&length), sizeof(length));
            pixelVecs[0].write(stream, dir);
            for (size_t i = 1; i < length; i++) {
                pixelVecs[i].write(stream, dir, pixelVecs[i - 1]);
            }
        }
    }

    return stream;
}

std::ostream &operator<<(std::ostream &stream, const Bin &bin) {
    int c = 0;
    for (auto pixVec : bin.pixelVecs) {
        for (PixelRef p = pixVec.start(); p.col(bin.dir) <= pixVec.end().col(bin.dir);
             p.move(bin.dir)) {
            if (++c % 10 == 0) {
                stream << "\n    ";
            }
            stream << p << ",";
        }
    }
    return stream;
}

///////////////////////////////////////////////////////////////////////////////////////

std::istream &PixelVec::read(std::istream &stream, const int8_t dir) {
    unsigned short runlength;
    stream.read(reinterpret_cast<char *>(&m_start), sizeof(m_start));
    stream.read(reinterpret_cast<char *>(&runlength), sizeof(runlength));
    switch (dir) {
    case PixelRef::POSDIAGONAL:
        m_end.x = m_start.x + static_cast<short>(runlength);
        m_end.y = m_start.y + static_cast<short>(runlength);
        break;
    case PixelRef::NEGDIAGONAL:
        m_end.x = m_start.x + static_cast<short>(runlength);
        m_end.y = m_start.y - static_cast<short>(runlength);
        break;
    case PixelRef::HORIZONTAL:
        m_end.x = m_start.x + static_cast<short>(runlength);
        m_end.y = m_start.y;
        break;
    case PixelRef::VERTICAL:
        m_end.x = m_start.x;
        m_end.y = m_start.y + static_cast<short>(runlength);
        break;
    }
    return stream;
}

std::ostream &PixelVec::write(std::ostream &stream, const int8_t dir) {
    stream.write(reinterpret_cast<const char *>(&m_start), sizeof(m_start));
    unsigned short runlength;
    switch (dir) {
    case PixelRef::HORIZONTAL:
    case PixelRef::POSDIAGONAL:
    case PixelRef::NEGDIAGONAL:
        runlength = static_cast<unsigned short>(m_end.x - m_start.x);
        break;
    case PixelRef::VERTICAL:
        runlength = static_cast<unsigned short>(m_end.y - m_start.y);
        break;
    }
    stream.write(reinterpret_cast<const char *>(&runlength), sizeof(runlength));

    return stream;
}

struct ShiftLength {
    unsigned short shift : 4;
    unsigned short runlength : 12;
};

std::istream &PixelVec::read(std::istream &stream, const int8_t dir, const PixelVec &context) {
    short primary;
    ShiftLength shiftlength;
    stream.read(reinterpret_cast<char *>(&primary), sizeof(primary));
    stream.read(reinterpret_cast<char *>(&shiftlength), sizeof(shiftlength));
    switch (dir) {
    case PixelRef::HORIZONTAL:
        m_start.x = primary;
        m_start.y = context.m_start.y + shiftlength.shift;
        m_end.x = m_start.x + shiftlength.runlength;
        m_end.y = m_start.y;
        break;
    case PixelRef::VERTICAL:
        m_start.x = context.m_start.x + shiftlength.shift;
        m_start.y = primary;
        m_end.x = m_start.x;
        m_end.y = m_start.y + shiftlength.runlength;
        break;
    }

    return stream;
}

std::ostream &PixelVec::write(std::ostream &stream, const int8_t dir, const PixelVec &context) {
    ShiftLength shiftlength;
    switch (dir) {
    case PixelRef::HORIZONTAL:
        stream.write(reinterpret_cast<const char *>(&(m_start.x)), sizeof(m_start.x));
        shiftlength.runlength = static_cast<unsigned short>(m_end.x - m_start.x) & 0x0FFF;
        shiftlength.shift = static_cast<unsigned short>(m_start.y - context.m_start.y) & 0x0F;
        break;
    case PixelRef::VERTICAL:
        stream.write(reinterpret_cast<const char *>(&(m_start.y)), sizeof(m_start.y));
        shiftlength.runlength = static_cast<unsigned short>(m_end.y - m_start.y) & 0x0FFF;
        shiftlength.shift = static_cast<unsigned short>(m_start.x - context.m_start.x) & 0x0F;
        break;
    }
    stream.write(reinterpret_cast<const char *>(&shiftlength), sizeof(shiftlength));

    return stream;
}
