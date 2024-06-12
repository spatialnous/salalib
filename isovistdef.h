// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/p2dpoly.h"

class IsovistDefinition {
  public:
    IsovistDefinition(double x, double y, double angle, double viewAngle)
        : mLocation(x, y), mAngle(angle), mViewAngle(viewAngle) {
        if (viewAngle >= 2 * M_PI) {
            mAngle = 0.0;
            mViewAngle = 0.0;
        }
    }

    IsovistDefinition(double x, double y) : mLocation(x, y), mAngle(0), mViewAngle(0) {}

    const Point2f &getLocation() const { return mLocation; }
    double getAngle() const { return mAngle; }
    double getViewAngle() const { return mViewAngle; }
    double getLeftAngle() const {
        double leftAngle = mAngle - 0.5 * mViewAngle;
        if (leftAngle < 0) {
            leftAngle += 2 * M_PI;
        }
        return leftAngle;
    }

    double getRightAngle() const {
        double rightAngle = mAngle + 0.5 * mViewAngle;
        if (rightAngle > 2 * M_PI) {
            rightAngle -= 2 * M_PI;
        }
        return rightAngle;
    }

  private:
    Point2f mLocation;
    double mAngle;
    double mViewAngle;
};
