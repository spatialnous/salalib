// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "pointmap.hpp"
#include "shapegraph.hpp"
#include "shapemap.hpp"

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
        PushValueError(std::string message) : BaseException(std::move(message)) {}
    };

    void pushValue(double &val, int &count, double thisval, Func pushFunc);

    std::tuple<std::optional<size_t>, size_t, std::optional<size_t>>
    getColumnIndices(const AttributeTable &sourceAttr,
                     const std::optional<const std::string> &colIn, AttributeTable &destAttr,
                     const std::string &colOut, const std::optional<const std::string> &countCol);
    std::tuple<size_t, size_t, std::optional<size_t>>
    getColumnIndices(const AttributeTable &sourceAttr, const std::string &colIn,
                     AttributeTable &destAttr, const std::string &colOut,
                     const std::optional<const std::string> &countCol);

    void shapeToPoint(const ShapeMap &sourceMap, const std::string &colIn, PointMap &destMap,
                      const std::string &colOut, Func pushFunc,
                      const std::optional<const std::string> &colCount = std::nullopt);
    void shapeToAxial(ShapeMap &sourceMap, const std::optional<const std::string> &colIn,
                      ShapeGraph &destMap, const std::string &colOut, Func pushFunc,
                      const std::optional<const std::string> &countCol = std::nullopt);
    void shapeToShape(ShapeMap &sourceMap, const std::optional<const std::string> &colIn,
                      ShapeMap &destMap, const std::string &colOut, Func pushFunc,
                      const std::optional<const std::string> &countCol = std::nullopt);
    void pointToShape(const PointMap &sourceMap, const std::optional<const std::string> &colIn,
                      ShapeMap &destMap, const std::string &colOut, Func pushFunc,
                      const std::optional<const std::string> &countCol = std::nullopt);
    void pointToAxial(const PointMap &sourceMap, const std::optional<const std::string> &colIn,
                      ShapeGraph &destMap, const std::string colOut, Func pushFunc,
                      const std::optional<const std::string> &countCol = std::nullopt);
    void axialToShape(const ShapeGraph &sourceMap, const std::optional<const std::string> &colIn,
                      ShapeMap &destMap, const std::string colOut, Func pushFunc,
                      const std::optional<const std::string> countCol = std::nullopt);
    void axialToAxial(const ShapeGraph &sourceMap, const std::optional<const std::string> &colIn,
                      ShapeGraph &destMap, const std::string colOut, Func pushFunc,
                      const std::optional<const std::string> &countCol = std::nullopt);

} // namespace PushValues
