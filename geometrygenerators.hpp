// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/simpleline.hpp"

#include <vector>

class GeometryGenerators {
  public:
    static std::vector<Point2f> generateDiskTriangles(size_t sides, float radius,
                                                      Point2f position = Point2f(0, 0));
    static std::vector<Point2f> generateMultipleDiskTriangles(size_t sides, float radius,
                                                              std::vector<Point2f> positions);

    static std::vector<SimpleLine> generateCircleLines(size_t sides, float radius,
                                                       Point2f position = Point2f(0, 0));
    static std::vector<SimpleLine> generateMultipleCircleLines(size_t sides, float radius,
                                                               std::vector<Point2f> positions);
};
