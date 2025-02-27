// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "displayparams.hpp"

#include "genlib/point2f.hpp"

#include <cstdint>

// For my colour scheme... some parameters to pass, and my own colour class

// Converts everything to safe HTML colours

struct PafColor {
    unsigned int color;
    auto redb() const { return static_cast<uint8_t>(color >> 16); }
    auto greenb() const { return static_cast<uint8_t>(color >> 8); }
    auto blueb() const { return static_cast<uint8_t>(color); }
    auto alphab() const { return static_cast<uint8_t>(color >> 24); }
    // Quick mod - TV
    void setr(unsigned char r) {
        color &= 0xff00ffff;
        color |= (static_cast<unsigned int>(r) << 16);
    }
    // Quick mod - TV
    void setg(unsigned char g) {
        color &= 0xffff00ff;
        color |= (static_cast<unsigned int>(g) << 8);
    }
    // Quick mod - TV
    void setb(unsigned char b) {
        color &= 0xffffff00;
        color |= static_cast<unsigned int>(b);
    }
    float redf() const { return static_cast<float>(redb()) / 255.0f; }
    float greenf() const { return static_cast<float>(greenb()) / 255.0f; }
    float bluef() const { return static_cast<float>(blueb()) / 255.0f; }
    PafColor() : color(0x00000000) {}
    PafColor(unsigned int rgb)
        : color(0xff000000 | rgb) // color in 0x00rrggbb format
    {}
    PafColor(double r, double g, double b, double a = 1.0)
        : color(static_cast<unsigned int>(                      //
              0x00000000                                        //
              | ((static_cast<unsigned char>(a * 255.0)) << 24) //
              | ((static_cast<unsigned char>(r * 255.0)) << 16) //
              | ((static_cast<unsigned char>(g * 255.0)) << 8)  //
              | ((static_cast<unsigned char>(b * 255.0))))) {}

    PafColor(const Point2f &vec, double a = 1.0) : color(fromVecA(vec, a)) {}

    operator unsigned int() { return color & 0x00ffffff; }
    friend bool operator==(const PafColor &a, const PafColor &b);
    friend bool operator!=(const PafColor &a, const PafColor &b);
    PafColor &makeAxmanesque(double field);
    PafColor &makeHueOnlyAxmanesque(double field);
    PafColor &makePurpleOrange(double field);
    PafColor &makeBlueRed(double field);
    PafColor &makeGreyScale(double field);
    PafColor &makeMonochrome(double field);
    PafColor &makeDepthmapClassic(double field, double blue, double red);
    PafColor &makeColor(double field, DisplayParams dp); // <- note, make copy to play around with
  private:
    unsigned int fromVecA(const Point2f &vec, double a = 1.0) {
        return static_cast<unsigned int>(                     //
            0x00000000                                        //
            | ((static_cast<unsigned char>(a * 255.0)) << 24) //
            | ((static_cast<unsigned char>(                   //
                   vec.dot(Point2f(1.0, 0.0)) * 255.0))
               << 16)                       //
            | ((static_cast<unsigned char>( //
                   vec.dot(Point2f(-0.5, 0.86602540378443864676372317075294)) * 255.0))
               << 8)                        //
            | ((static_cast<unsigned char>( //
                  vec.dot(Point2f(-0.5, -0.86602540378443864676372317075294)) * 255.0))));
    }
};
inline bool operator==(const PafColor &a, const PafColor &b) { return (a.color == b.color); }
inline bool operator!=(const PafColor &a, const PafColor &b) { return (a.color != b.color); }
