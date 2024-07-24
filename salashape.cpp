// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "salashape.h"

#include "genlib/readwritehelpers.h"

bool SalaShape::read(std::istream &stream) {
    // defaults
    m_draworder = -1;

    stream.read((char *)&m_type, sizeof(m_type));

    stream.read((char *)&m_region, sizeof(m_region));

    stream.read((char *)&m_centroid, sizeof(m_centroid));

    stream.read((char *)&m_area, sizeof(m_area));
    stream.read((char *)&m_perimeter, sizeof(m_perimeter));

    dXreadwrite::readIntoVector(stream, m_points);

    return true;
}

bool SalaShape::write(std::ostream &stream) const {
    stream.write((char *)&m_type, sizeof(m_type));
    stream.write((char *)&m_region, sizeof(m_region));
    stream.write((char *)&m_centroid, sizeof(m_centroid));
    stream.write((char *)&m_area, sizeof(m_area));
    stream.write((char *)&m_perimeter, sizeof(m_perimeter));
    dXreadwrite::writeVector(stream, m_points);
    return true;
}

void SalaShape::setCentroidAreaPerim() {
    m_area = 0.0;
    m_perimeter = 0.0;
    m_centroid = Point2f(0, 0);
    for (size_t i = 0; i < m_points.size(); i++) {
        Point2f &p1 = m_points[i];
        Point2f &p2 = m_points[(i + 1) % m_points.size()];
        double a_i = (p1.x * p2.y - p2.x * p1.y) / 2.0;
        m_area += a_i;
        a_i /= 6.0;
        m_centroid.x += (p1.x + p2.x) * a_i;
        m_centroid.y += (p1.y + p2.y) * a_i;
        Point2f side = p2 - p1;
        m_perimeter += side.length();
    }
    m_type &= ~SHAPE_CCW;
    if (pafmath::sgn(m_area) == 1) {
        m_type |= SHAPE_CCW;
    }
    m_centroid.scale(2.0 / m_area); // note, *not* fabs(m_area) as it is then
                                    // confused by clockwise ordered shapes
    m_area = fabs(m_area);
    if (isOpen()) {
        // take off the automatically collected final side
        Point2f side = m_points.back() - m_points.front();
        m_perimeter -= side.length();
    }
}

// allows override of the above (used for isovists)
void SalaShape::setCentroid(const Point2f &p) { m_centroid = p; }

// get the angular deviation along the length of a poly line:
double SalaShape::getAngDev() const {
    double dev = 0.0;
    for (size_t i = 1; i < m_points.size() - 1; i++) {
        double ang = angle(m_points[i - 1], m_points[i], m_points[i + 1]);

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

std::vector<SalaEdgeU> SalaShape::getClippingSet(QtRegion &clipframe) const {
    std::vector<SalaEdgeU> edgeset;
    bool last_inside = (clipframe.contains_touch(m_points[0])) ? true : false;
    bool found_inside = last_inside;
    for (size_t i = 1; i < m_points.size(); i++) {
        bool next_inside = (clipframe.contains_touch(m_points[i])) ? true : false;
        found_inside |= next_inside;
        if (last_inside != next_inside) {
            if (last_inside) {
                EdgeU eu = clipframe.getCutEdgeU(m_points[i - 1], m_points[i]);
                edgeset.push_back(SalaEdgeU(i, false, eu));
            } else {
                EdgeU eu = clipframe.getCutEdgeU(m_points[i], m_points[i - 1]);
                edgeset.push_back(SalaEdgeU(i - 1, true, eu));
            }
        }
        last_inside = next_inside;
    }
    if (!found_inside) {
        // note: deliberately add a single empty SalaEdgeU if this polygon is never
        // inside the frame
        edgeset.push_back(SalaEdgeU());
    }
    return edgeset;
}
