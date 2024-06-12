// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

class GridProperties {
  public:
    GridProperties(double maxDimension);
    double getMin() const { return m_min; }
    double getMax() const { return m_max; }
    double getDefault() const { return m_default; }

  private:
    double m_max;
    double m_min;
    double m_default;
};
