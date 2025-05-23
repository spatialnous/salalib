// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "salashape.hpp"

#include "genlib/readwritehelpers.hpp"

bool SalaShape::read(std::istream &stream) {
    // defaults
    m_draworder = -1;

    stream.read(reinterpret_cast<char *>(&m_type), sizeof(m_type));

    stream.read(reinterpret_cast<char *>(&m_region), sizeof(m_region));

    stream.read(reinterpret_cast<char *>(&m_centroid), sizeof(m_centroid));

    stream.read(reinterpret_cast<char *>(&m_area), sizeof(m_area));
    stream.read(reinterpret_cast<char *>(&m_perimeter), sizeof(m_perimeter));

    dXreadwrite::readIntoVector(stream, points);

    return true;
}

bool SalaShape::write(std::ostream &stream) const {
    stream.write(reinterpret_cast<const char *>(&m_type), sizeof(m_type));
    stream.write(reinterpret_cast<const char *>(&m_region), sizeof(m_region));
    stream.write(reinterpret_cast<const char *>(&m_centroid), sizeof(m_centroid));
    stream.write(reinterpret_cast<const char *>(&m_area), sizeof(m_area));
    stream.write(reinterpret_cast<const char *>(&m_perimeter), sizeof(m_perimeter));
    dXreadwrite::writeVector(stream, points);
    return true;
}

void SalaShape::setCentroidAreaPerim() {
    m_area = 0.0;
    m_perimeter = 0.0;
    m_centroid = Point2f(0, 0);
    for (size_t i = 0; i < points.size(); i++) {
        Point2f &p1 = points[i];
        Point2f &p2 = points[(i + 1) % points.size()];
        double aI = (p1.x * p2.y - p2.x * p1.y) / 2.0;
        m_area += aI;
        aI /= 6.0;
        m_centroid.x += (p1.x + p2.x) * aI;
        m_centroid.y += (p1.y + p2.y) * aI;
        Point2f side = p2 - p1;
        m_perimeter += side.length();
    }
    m_type &= ~SHAPE_CCW;
    if (pafmath::sgn(m_area) == 1) {
        m_type |= SHAPE_CCW;
    }
    if (m_area == 0) {
        m_centroid.scale(0);
    } else {
        m_centroid.scale(2.0 / m_area); // note, *not* fabs(m_area) as it is then
                                        // confused by clockwise ordered shapes
    }
    m_area = fabs(m_area);
    if (isOpen() && points.size() > 1) {
        // take off the automatically collected final side
        Point2f side = points.back() - points.front();
        m_perimeter -= side.length();
    }
}

// allows override of the above (used for isovists)
void SalaShape::setCentroid(const Point2f &p) { m_centroid = p; }

// get the angular deviation along the length of a poly line:
double SalaShape::getAngDev() const {
    double dev = 0.0;
    for (size_t i = 1; i < points.size() - 1; i++) {
        double ang = points[i - 1].angle(points[i], points[i + 1]);

        // Quick mod - TV
#if defined(_MSC_VER)
        dev += abs(M_PI - ang);
#else
        (M_PI - ang) < 0.0 ? dev += (ang - M_PI) : dev += (M_PI - ang);
#endif
    }
    // convert to Iida Hillier units (0 to 2):
    dev /= M_PI * 0.5;
    return dev;
}

std::vector<SalaEdgeU> SalaShape::getClippingSet(Region4f &clipframe) const {
    std::vector<SalaEdgeU> edgeset;
    bool lastInside = (clipframe.contains_touch(points[0])) ? true : false;
    bool foundInside = lastInside;
    for (size_t i = 1; i < points.size(); i++) {
        bool nextInside = (clipframe.contains_touch(points[i])) ? true : false;
        foundInside |= nextInside;
        if (lastInside != nextInside) {
            if (lastInside) {
                EdgeU eu = clipframe.getCutEdgeU(points[i - 1], points[i]);
                edgeset.push_back(SalaEdgeU(static_cast<int>(i), false, eu));
            } else {
                EdgeU eu = clipframe.getCutEdgeU(points[i], points[i - 1]);
                edgeset.push_back(SalaEdgeU(static_cast<int>(i - 1), true, eu));
            }
        }
        lastInside = nextInside;
    }
    if (!foundInside) {
        // note: deliberately add a single empty SalaEdgeU if this polygon is never
        // inside the frame
        edgeset.push_back(SalaEdgeU());
    }
    return edgeset;
}
