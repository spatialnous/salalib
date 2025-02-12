// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "geometrygenerators.hpp"

std::vector<Point2f> GeometryGenerators::generateDiskTriangles(size_t sides, float radius,
                                                               Point2f position) {
    std::vector<Point2f> diskTriangles;
    for (size_t i = 0; i < sides; i++) {
        diskTriangles.push_back(Point2f(position.x, position.y));
        diskTriangles.push_back(Point2f(
            position.x +
                radius * sin(2 * M_PI * static_cast<double>(i + 1) / static_cast<double>(sides)),
            position.y +
                radius * cos(2 * M_PI * static_cast<double>(i + 1) / static_cast<double>(sides))));
        diskTriangles.push_back(Point2f(
            position.x +
                radius * sin(2 * M_PI * static_cast<double>(i) / static_cast<double>(sides)),
            position.y +
                radius * cos(2 * M_PI * static_cast<double>(i) / static_cast<double>(sides))));
    }
    return diskTriangles;
}

std::vector<Point2f>
GeometryGenerators::generateMultipleDiskTriangles(size_t sides, float radius,
                                                  std::vector<Point2f> positions) {
    std::vector<Point2f> diskTriangles = generateDiskTriangles(sides, radius);

    std::vector<Point2f> mulitpleDiskTriangles;

    std::vector<Point2f>::const_iterator iter = positions.begin(), end = positions.end();
    for (; iter != end; ++iter) {
        Point2f position = *iter;
        std::vector<Point2f>::const_iterator iterDiskVertices = diskTriangles.begin(),
                                             endDiskPoints = diskTriangles.end();
        for (; iterDiskVertices != endDiskPoints; ++iterDiskVertices) {
            Point2f vertex = *iterDiskVertices;
            mulitpleDiskTriangles.push_back(Point2f(position.x + vertex.x, position.y + vertex.y));
        }
    }
    return mulitpleDiskTriangles;
}

std::vector<SimpleLine> GeometryGenerators::generateCircleLines(size_t sides, float radius,
                                                                Point2f position) {
    std::vector<SimpleLine> cirleLines;
    for (size_t i = 0; i < sides; i++) {
        cirleLines.push_back(
            SimpleLine(Point2f(position.x + radius * sin(2 * M_PI * static_cast<double>(i + 1) /
                                                         static_cast<double>(sides)),
                               position.y + radius * cos(2 * M_PI * static_cast<double>(i + 1) /
                                                         static_cast<double>(sides))),
                       Point2f(position.x + radius * sin(2 * M_PI * static_cast<double>(i) /
                                                         static_cast<double>(sides)),
                               position.y + radius * cos(2 * M_PI * static_cast<double>(i) /
                                                         static_cast<double>(sides)))));
    }
    return cirleLines;
}

std::vector<SimpleLine>
GeometryGenerators::generateMultipleCircleLines(size_t sides, float radius,
                                                std::vector<Point2f> positions) {
    std::vector<SimpleLine> circleLines = generateCircleLines(sides, radius);

    std::vector<SimpleLine> mulitpleCircleLines;

    std::vector<Point2f>::const_iterator iter = positions.begin(), end = positions.end();
    for (; iter != end; ++iter) {
        Point2f position = *iter;
        std::vector<SimpleLine>::const_iterator iterCircleLines = circleLines.begin(),
                                                endCircleLines = circleLines.end();
        for (; iterCircleLines != endCircleLines; ++iterCircleLines) {
            SimpleLine line = *iterCircleLines;
            mulitpleCircleLines.push_back(
                SimpleLine(Point2f(position.x + line.start().x, position.y + line.start().y),
                           Point2f(position.x + line.end().x, position.y + line.end().y)));
        }
    }
    return mulitpleCircleLines;
}
