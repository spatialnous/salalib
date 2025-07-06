// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2018-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is my code to make a set of axial lines from a set of boundary lines

#pragma once

#include "pafcolor.hpp"
#include "pixelbase.hpp"

#include "genlib/comm.hpp"
#include "genlib/line4f.hpp"
#include "genlib/simplematrix.hpp"

#include <map>
#include <string>

// couple of quick helpers

struct LineTest {
    Line4f line;
    unsigned int test;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    LineTest(const Line4f &l = Line4f(), int t = -1)
        : line(l), test(static_cast<unsigned int>(t)), _padding0(0) {

        // TODO: Shouldn't be casting an int with a known
        // default value of -1
    }
    // operator Line() {return line;}
};

struct LineKey {
    unsigned int file : 4;
    unsigned int layer : 6;
    unsigned int lineref : 20;

  private:
    [[maybe_unused]] unsigned char _padding0 : 2;

  public:
    operator int() { return *reinterpret_cast<int *>(this); }
    LineKey(int value = 0) : file(), layer(), lineref(), _padding0(0) {
        *reinterpret_cast<int *>(this) = value;
    }
    friend bool operator<(LineKey a, LineKey b);
    friend bool operator>(LineKey a, LineKey b);
    friend bool operator==(LineKey a, LineKey b);
};
inline bool operator<(LineKey a, LineKey b) { return int(a) < int(b); }
inline bool operator>(LineKey a, LineKey b) { return int(a) > int(b); }
inline bool operator==(LineKey a, LineKey b) { return int(a) == int(b); }

/////////////////////////////////////////////

class SpacePixel : public PixelBase {
    friend class PointMap;
    friend class AxialMaps;
    friend class AxialPolygons;
    friend class ShapeMap; // for transfer to everything being ShapeMaps
  protected:
    bool m_lock;
    mutable bool m_newline;

  protected:
    bool m_show;
    bool m_edit;
    PafColor m_color;
    int m_ref;
    int m_style; // allows for bold / dotted lines etc
    genlib::RowMatrix<std::vector<int>> m_pixelLines;

    std::map<int, LineTest> m_lines;
    //
    // for screen drawing
    mutable std::vector<int> m_displayLines;
    mutable int m_current;
    //
    // for line testing
    mutable unsigned int m_test;
    //
  public:
    SpacePixel(const std::string &name = std::string("Default"));
    //
    SpacePixel(const SpacePixel &spacepixel);
    SpacePixel &operator=(const SpacePixel &spacepixel);
    void construct(const SpacePixel &spacepixel);
    //
    PixelRef pixelate(const Point2f &p, bool constrain = true, int = 1) const override;
    //   PixelRefVector pixelate( const Line& l ) const;
    //
    void initLines(int size, const Point2f &min, const Point2f &max, double density = 1.0);
    void reinitLines(double density); // just reinitialises pixel lines, keeps lines, current ref
                                      // and test setting
    //
    void addLine(const Line4f &l);
    void sortPixelLines();
    //
    int addLineDynamic(const Line4f &l);

    bool intersect(const Line4f &l, double tolerance = 0.0);
    bool intersect_exclude(const Line4f &l, double tolerance = 0.0);

    void cutLine(Line4f &l, short dir, Communicator *comm = nullptr);

    const Region4f &getRegion() const { return static_cast<const Region4f &>(m_region); }

    void setRegion(Region4f &region) { m_region = region; }
    //
    const std::map<int, LineTest> &
    getAllLines() const // Danger! Use solely to look at the raw line data
    {
        return m_lines;
    }
    //
    // For easy layer manipulation:
    void setShow(bool show = true) { m_show = show; }
    bool isShown() const { return m_show; }
    void setEditable(bool edit = true) { m_edit = edit; }
    bool isEditable() const { return m_edit; }

  public:
    virtual bool read(std::istream &stream);
    virtual bool write(std::ofstream &stream);
    friend bool operator==(const SpacePixel &a, const SpacePixel &b);
};

// simply check they are the same name... useful for findindex from the group
inline bool operator==(const SpacePixel &a, const SpacePixel &b) { return a.m_name == b.m_name; }
