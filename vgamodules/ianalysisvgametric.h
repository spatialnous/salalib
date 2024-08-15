// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"

class IAnalysisVGAMetric : IAnalysis {
  protected:
    struct AnalysisData {
        const Point &m_point;
        const PixelRef &m_ref;
        size_t m_attributeDataRow;
        int m_misc = 0;
        float m_dist = -1.0f;
        float m_cumangle = 0.0f;
        AnalysisData(const Point &point, const PixelRef &ref, size_t attributeDataRow, int misc,
                     float dist, float cumangle)
            : m_point(point), m_ref(ref), m_attributeDataRow(attributeDataRow), m_misc(misc),
              m_dist(dist), m_cumangle(cumangle) {}
    };
    // to allow a dist / PixelRef pair for easy sorting
    // (have to do comparison operation on both dist and PixelRef as
    // otherwise would have a duplicate key for pqmap / pqvector)

    struct MetricTriple {
        float dist;
        PixelRef pixel;
        PixelRef lastpixel;
        MetricTriple(float d = 0.0f, PixelRef p = NoPixel, PixelRef lp = NoPixel) {
            dist = d;
            pixel = p;
            lastpixel = lp;
        }
        bool operator==(const MetricTriple &mp2) const {
            return (dist == mp2.dist && pixel == mp2.pixel);
        }
        bool operator<(const MetricTriple &mp2) const {
            return (dist < mp2.dist) || (dist == mp2.dist && pixel < mp2.pixel);
        }
        bool operator>(const MetricTriple &mp2) const {
            return (dist > mp2.dist) || (dist == mp2.dist && pixel > mp2.pixel);
        }
        bool operator!=(const MetricTriple &mp2) const {
            return (dist != mp2.dist) || (pixel != mp2.pixel);
        }
    };
};
