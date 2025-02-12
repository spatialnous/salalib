// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/point2f.hpp"

#include <cmath>

class IsovistDefinition {
  public:
    IsovistDefinition(double x, double y, double angle, double viewAngle)
        : m_location(x, y), m_angle(angle), m_viewAngle(viewAngle) {
        if (viewAngle >= 2 * M_PI) {
            m_angle = 0.0;
            m_viewAngle = 0.0;
        }
    }

    IsovistDefinition(double x, double y) : m_location(x, y), m_angle(0), m_viewAngle(0) {}

    const Point2f &getLocation() const { return m_location; }
    double getAngle() const { return m_angle; }
    double getViewAngle() const { return m_viewAngle; }
    double getLeftAngle() const {
        double leftAngle = m_angle - 0.5 * m_viewAngle;
        if (leftAngle < 0) {
            leftAngle += 2 * M_PI;
        }
        return leftAngle;
    }

    double getRightAngle() const {
        double rightAngle = m_angle + 0.5 * m_viewAngle;
        if (rightAngle > 2 * M_PI) {
            rightAngle -= 2 * M_PI;
        }
        return rightAngle;
    }

  private:
    Point2f m_location;
    double m_angle;
    double m_viewAngle;
};
