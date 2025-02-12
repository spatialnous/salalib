// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "exportutils.hpp"

void exportUtils::writeMapShapesAsCat(ShapeMap &map, std::ostream &stream) {
    stream << "CAT" << std::endl;
    for (auto &refShape : map.getAllShapes()) {
        SalaShape &shape = refShape.second;
        if (shape.isPolyLine() || shape.isPolygon()) {
            stream << "Begin " << (shape.isPolyLine() ? "Polyline" : "Polygon") << std::endl;
            for (Point2f p : shape.points) {
                stream << p.x << " " << p.y << std::endl;
            }
            stream << "End " << (shape.isPolyLine() ? "Polyline" : "Polygon") << std::endl;
        } else if (shape.isLine()) {
            stream << "Begin Polyline" << std::endl;
            stream << shape.getLine().ax() << " " << shape.getLine().ay() << std::endl;
            stream << shape.getLine().bx() << " " << shape.getLine().by() << std::endl;
            stream << "End Polyline" << std::endl;
        }
    }
}
