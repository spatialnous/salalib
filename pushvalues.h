// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/pointmap.h"
#include "salalib/shapegraph.h"
#include "salalib/shapemap.h"

namespace PushValues {
    enum class Func {
        MAX, // = 0
        MIN, // = 1
        AVG, // = 2
        TOT, // = 3
        NONE // = -1
    };

    class PushValueError : public depthmapX::BaseException {
      public:
        PushValueError(std::string message) : BaseException(message) {}
    };

    void pushValue(double &val, int &count, double thisval, Func pushFunc);

    std::tuple<std::optional<size_t>, size_t, std::optional<size_t>>
    getColumnIndices(const AttributeTable &sourceAttr, std::optional<std::string> colIn,
                     AttributeTable &destAttr, std::string colOut,
                     std::optional<std::string> countCol);
    std::tuple<size_t, size_t, std::optional<size_t>>
    getColumnIndices(const AttributeTable &sourceAttr, std::string colIn, AttributeTable &destAttr,
                     std::string colOut, std::optional<std::string> countCol);

    void shapeToPoint(const ShapeMap &sourceMap, std::string colIn, PointMap &destMap,
                      std::string colOut, Func pushFunc,
                      std::optional<std::string> colCount = std::nullopt);
    void shapeToAxial(ShapeMap &sourceMap, std::optional<std::string> colIn, ShapeGraph &destMap,
                      std::string colOut, Func pushFunc,
                      std::optional<std::string> countCol = std::nullopt);
    void shapeToShape(ShapeMap &sourceMap, std::optional<std::string> colIn, ShapeMap &destMap,
                      std::string colOut, Func pushFunc,
                      std::optional<std::string> countCol = std::nullopt);
    void pointToShape(const PointMap &sourceMap, std::optional<std::string> colIn,
                      ShapeMap &destMap, std::string colOut, Func pushFunc,
                      std::optional<std::string> countCol = std::nullopt);
    void pointToAxial(const PointMap &sourceMap, std::optional<std::string> colIn,
                      ShapeGraph &destMap, std::string colOut, Func pushFunc,
                      std::optional<std::string> countCol = std::nullopt);
    void axialToShape(const ShapeGraph &sourceMap, std::optional<std::string> colIn,
                      ShapeMap &destMap, std::string colOut, Func pushFunc,
                      std::optional<std::string> countCol = std::nullopt);
    void axialToAxial(const ShapeGraph &sourceMap, std::optional<std::string> colIn,
                      ShapeGraph &destMap, std::string colOut, Func pushFunc,
                      std::optional<std::string> countCol = std::nullopt);

} // namespace PushValues
