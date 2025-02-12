// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "point2f.hpp"

// an event is a point plus time (as in spacetime technical language)
class Event2f : public Point2f {
  public:
    double t; // time in seconds
    Event2f() : Point2f(), t(0.0) {}
    Event2f(double xIn, double yIn, double tIn) : Point2f(xIn, yIn), t(tIn) {}
    Event2f(Point2f &p) : Point2f(p), t(0.0) {}
    Event2f(Point2f &p, double tIn) : Point2f(p), t(tIn) {}
};
