// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "line4f.h"

// plain 2-point line without regions
struct SimpleLine {
  public:
    SimpleLine(const Line4f &line) {
        m_start.x = line.t_start().x;
        m_start.y = line.t_start().y;
        m_end.x = line.t_end().x;
        m_end.y = line.t_end().y;
    }
    SimpleLine(const Point2f &a, const Point2f &b) {
        m_start.x = a.x;
        m_start.y = a.y;
        m_end.x = b.x;
        m_end.y = b.y;
    }
    SimpleLine(double x1, double y1, double x2, double y2) {
        m_start.x = x1;
        m_start.y = y1;
        m_end.x = x2;
        m_end.y = y2;
    }
    const Point2f &start() const { return m_start; }
    const Point2f &end() const { return m_end; }

  private:
    Point2f m_start;
    Point2f m_end;
};
