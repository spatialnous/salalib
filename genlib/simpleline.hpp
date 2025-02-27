// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "line4f.hpp"

// plain 2-point line without regions
struct SimpleLine {
  public:
    SimpleLine(const Line4f &line)
        : m_start(line.t_start().x, line.t_start().y), m_end(line.t_end().x, line.t_end().y) {}
    SimpleLine(const Point2f &a, const Point2f &b) : m_start(a.x, a.y), m_end(b.x, b.y) {}
    SimpleLine(double x1, double y1, double x2, double y2) : m_start(x1, y1), m_end(x2, y2) {}
    const Point2f &start() const { return m_start; }
    const Point2f &end() const { return m_end; }

  private:
    Point2f m_start;
    Point2f m_end;
};
