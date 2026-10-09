// SPDX-FileCopyrightText: 2018-2026 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../genlib/containerutils.hpp"
#include "../pixelref.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace VGATypes {
    using ADRowIndex = uint32_t;
    static_assert(sizeof(PixelRef) <= sizeof(ADRowIndex),
                  "ADRowIndex must be wide enough to index any lattice position");
    struct ADRef {
        ADRowIndex idx;
        int bin;
    };
    using ADRefVector = std::vector<ADRef>;

    class RefIndex {
        std::vector<PixelRef> m_refs;

      public:
        explicit RefIndex(std::vector<PixelRef> refs) : m_refs(std::move(refs)) {
            for (size_t i = 1; i < m_refs.size(); i++)
                if (!(m_refs[i - 1] < m_refs[i]))
                    throw std::logic_error("RefIndex: refs must be strictly ascending");
        }
        const std::vector<PixelRef> getRefs() const { return m_refs; }
        size_t size() const { return m_refs.size(); }
        PixelRef operator[](size_t i) const { return m_refs[i]; }
        ADRowIndex idx(PixelRef ref) const {
            auto it = genlib::findBinary(m_refs, ref);
            if (it == m_refs.end())
                throw std::out_of_range("Ref " + std::to_string(ref) + " not in refs");
            return static_cast<ADRowIndex>(std::distance(m_refs.begin(), it));
        }
        std::optional<size_t> idxOptional(PixelRef ref) const { // body here, not in a .cpp
            auto it = genlib::findBinary(m_refs, ref);
            if (it == m_refs.end())
                return std::nullopt;
            return std::make_optional(static_cast<size_t>(std::distance(m_refs.begin(), it)));
        }
    };
} // namespace VGATypes
