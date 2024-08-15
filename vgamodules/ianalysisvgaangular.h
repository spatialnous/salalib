//// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
////
//// SPDX-License-Identifier: GPL-3.0-or-later

//#pragma once

//#include "salalib/ianalysis.h"

// class IAnalysisVGAAngular : IAnalysis {
//   protected:
//     struct AnalysisData {
//         const Point &m_point;
//         const PixelRef &m_ref;
//         size_t m_attributeDataRow;
//         int m_misc = 0;
//         float m_dist = 0.0f;
//         float m_cumangle = -1.0f;
//         AnalysisData(const Point &point, const PixelRef &ref, size_t attributeDataRow, int misc,
//                      float dist, float cumangle)
//             : m_point(point), m_ref(ref), m_attributeDataRow(attributeDataRow), m_misc(misc),
//               m_dist(dist), m_cumangle(cumangle) {}
//     };

//    struct AngularTriple {
//        float angle;
//        PixelRef pixel;
//        PixelRef lastpixel;
//        AngularTriple(float a = 0.0f, PixelRef p = NoPixel, PixelRef lp = NoPixel) {
//            angle = a;
//            pixel = p;
//            lastpixel = lp;
//        }
//        bool operator==(const AngularTriple &mp2) const {
//            return (angle == mp2.angle && pixel == mp2.pixel);
//        }
//        bool operator<(const AngularTriple &mp2) const {
//            return (angle < mp2.angle) || (angle == mp2.angle && pixel < mp2.pixel);
//        }
//        bool operator>(const AngularTriple &mp2) const {
//            return (angle > mp2.angle) || (angle == mp2.angle && pixel > mp2.pixel);
//        }
//        bool operator!=(const AngularTriple &mp2) const {
//            return (angle != mp2.angle) || (pixel != mp2.pixel);
//        }
//    };
//};
