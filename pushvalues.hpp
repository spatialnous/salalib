// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024-2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "latticemap.hpp"
#include "shapegraph.hpp"
#include "shapemap.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace PushValues {
    enum class Func {
        MAX, // = 0
        MIN, // = 1
        AVG, // = 2
        TOT, // = 3
        NONE // = -1
    };

    class PushValueError : public genlib::BaseException {
      public:
        PushValueError(std::string message) : BaseException(std::move(message)) {}
    };

    void pushValue(double &val, int &count, double thisval, Func pushFunc);

    std::tuple<std::optional<size_t>, size_t, std::optional<size_t>>
    getColumnIndices(const AttributeTable &sourceAttr,
                     const std::optional<const std::string_view> colIn, AttributeTable &destAttr,
                     const std::string_view colOut,
                     const std::optional<const std::string_view> countCol);
    std::tuple<size_t, size_t, std::optional<size_t>>
    getColumnIndices(const AttributeTable &sourceAttr, const std::string_view colIn,
                     AttributeTable &destAttr, const std::string_view colOut,
                     const std::optional<const std::string_view> countCol);

    void shapeToPoint(const ShapeMap &sourceMap, const std::string_view colIn, LatticeMap &destMap,
                      const std::string_view colOut, Func pushFunc,
                      const std::optional<const std::string_view> &colCount = std::nullopt);
    void shapeToAxial(ShapeMap &sourceMap, const std::optional<const std::string_view> colIn,
                      ShapeGraph &destMap, const std::string_view colOut, Func pushFunc,
                      const std::optional<const std::string_view> countCol = std::nullopt);
    void shapeToShape(ShapeMap &sourceMap, const std::optional<const std::string_view> colIn,
                      ShapeMap &destMap, const std::string_view colOut, Func pushFunc,
                      const std::optional<const std::string_view> countCol = std::nullopt);
    void pointToShape(const LatticeMap &sourceMap,
                      const std::optional<const std::string_view> colIn, ShapeMap &destMap,
                      const std::string_view colOut, Func pushFunc,
                      const std::optional<const std::string_view> countCol = std::nullopt);
    void pointToAxial(const LatticeMap &sourceMap,
                      const std::optional<const std::string_view> colIn, ShapeGraph &destMap,
                      const std::string_view colOut, Func pushFunc,
                      const std::optional<const std::string_view> countCol = std::nullopt);
    void axialToShape(const ShapeGraph &sourceMap,
                      const std::optional<const std::string_view> colIn, ShapeMap &destMap,
                      const std::string_view colOut, Func pushFunc,
                      const std::optional<const std::string> countCol = std::nullopt);
    void axialToAxial(const ShapeGraph &sourceMap,
                      const std::optional<const std::string_view> colIn, ShapeGraph &destMap,
                      const std::string_view colOut, Func pushFunc,
                      const std::optional<const std::string_view> countCol = std::nullopt);

} // namespace PushValues
