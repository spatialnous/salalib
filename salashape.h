// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salaedgeu.h"

#include "genlib/p2dpoly.h"

#include <istream>
#include <vector>

class SalaShape {
  public:
    std::vector<Point2f> points;
    enum {
        SHAPE_POINT = 0x01,
        SHAPE_LINE = 0x02,
        SHAPE_POLY = 0x04,
        SHAPE_CIRCLE = 0x08,
        SHAPE_TYPE = 0x0f,
        SHAPE_CLOSED = 0x40,
        SHAPE_CCW = 0x80
    };
    friend class ShapeMap;

  protected:
    unsigned char m_type;
    Point2f m_centroid; // centre of mass, but also used as for point if object is a point
    Line m_region;      // bounding box, but also used as a line if object is a line, hence type
    double m_area;
    double m_perimeter;
    // these are all temporary data which are recalculated on reload
    //    mutable bool m_selected;
    mutable float m_color = 0;
    mutable int m_draworder = -1;

  public:
    SalaShape(unsigned char type = 0)
        : m_type(type), m_area(0.0), m_perimeter(0.0), m_draworder(-1) {}
    SalaShape(const Point2f &point)
        : m_type(SHAPE_POINT), m_centroid(point), m_area(0.0), m_perimeter(0.0), m_draworder(-1) {
        m_region = Line(point, point);
    }
    SalaShape(const Line &line)
        : m_type(SHAPE_LINE), m_region(line), m_area(0.0), m_perimeter(m_region.length()),
          m_draworder(-1) {
        m_centroid = m_region.getCentre();
    }
    bool operator==(const SalaShape &other) const {
        return                                     //
            m_type == other.getType() &&           //
            m_centroid == other.getCentroid() &&   //
            m_draworder == other.getDrawOrder() && //
            m_region == other.getLine() &&         //
            m_area == other.getArea() &&           //
            m_perimeter == other.getPerimeter();
    }
    unsigned char getType() const { return m_type; }
    int getDrawOrder() const { return m_draworder; }

    bool isOpen() const { return (m_type & SHAPE_CLOSED) == 0; }
    bool isClosed() const { return (m_type & SHAPE_CLOSED) == SHAPE_CLOSED; }
    bool isPoint() const { return (m_type == SHAPE_POINT); }
    bool isLine() const { return (m_type == SHAPE_LINE); }
    bool isPolyLine() const { return (m_type & (SHAPE_POLY | SHAPE_CLOSED)) == SHAPE_POLY; }
    bool isPolygon() const {
        return (m_type & (SHAPE_POLY | SHAPE_CLOSED)) == (SHAPE_POLY | SHAPE_CLOSED);
    }
    bool isCCW() const { return (m_type & SHAPE_CCW) == SHAPE_CCW; }
    //
    const Point2f &getPoint() const { return m_centroid; }
    const Line &getLine() const { return m_region; }
    const QtRegion &getBoundingBox() const { return m_region; }
    //
    double getArea() const { return m_area; }
    double getPerimeter() const { return m_perimeter; }
    // duplicate function, but easier to understand naming convention
    double getLength() const { return m_perimeter; }
    //
    void setCentroidAreaPerim();
    void setCentroid(const Point2f &p);
    // duplicate function, but easier to understand naming convention
    const Point2f &getCentroid() const { return m_centroid; }
    //
    double getAngDev() const;
    //    bool isSelected() const { return m_selected; }
    //    void setSelected(bool selected) { m_selected = selected; }
    //
    std::vector<SalaEdgeU> getClippingSet(QtRegion &clipframe) const;
    //
    bool read(std::istream &stream);
    bool write(std::ostream &stream) const;

    std::vector<Line> getAsLines() const {
        std::vector<Line> lines;
        if (isLine()) {
            lines.push_back(getLine());
        } else if (isPolyLine() || isPolygon()) {
            for (size_t j = 0; j < points.size() - 1; j++) {
                lines.push_back(Line(points[j], points[j + 1]));
            }
            if (isClosed()) {
                lines.push_back(Line(points[points.size() - 1], points[0]));
            }
        }
        return lines;
    }
};
