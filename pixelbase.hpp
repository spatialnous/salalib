// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2018-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is my code to make a set of axial lines from a set of boundary lines

#pragma once

#include "pixelref.hpp"

#include "genlib/line4f.hpp"

#include <string>

class PixelBase {
  protected:
    size_t m_rows;
    size_t m_cols;
    Region4f m_region;
    std::string m_name = "Default";

  public:
    PixelBase(const std::string &name = std::string("Default"))
        : m_rows(), m_cols(), m_region(), m_name(name) {}
    virtual ~PixelBase() {}
    // constrain is constrain to bounding box (i.e., in row / col bounds)
    virtual PixelRef pixelate(const Point2f &, bool constrain = true,
                              int scalefactor = 1) const = 0;
    PixelRefVector pixelateLine(Line4f l, int scalefactor = 1) const;
    PixelRefVector pixelateLineTouching(Line4f l, double tolerance) const;
    PixelRefVector quickPixelateLine(PixelRef p, PixelRef q) const;
    bool includes(const PixelRef pix) const {
        return (pix.x >= 0 && pix.x < static_cast<short>(m_cols) && pix.y >= 0 &&
                pix.y < static_cast<short>(m_rows));
    }
    size_t getCols() const { return m_cols; }
    size_t getRows() const { return m_rows; }
    const Region4f &getRegion() const { return m_region; }
    void setName(const std::string &name) { m_name = name; }
    std::string getName() { return m_name; }
};
