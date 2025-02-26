// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "attributetable.hpp"

#include <algorithm>

class ConstAttributeIndexItem {
  public:
    ConstAttributeIndexItem(const AttributeKey &k, double v, const AttributeRow &r)
        : key(k), _padding0(0), value(v), row(&r) {}

    ConstAttributeIndexItem(const ConstAttributeIndexItem &other)
        : key(other.key), _padding0(0), value(other.value), row(other.row) {}
    ConstAttributeIndexItem &operator=(const ConstAttributeIndexItem &other) {
        if (this == &other) {
            return *this;
        }
        key = other.key;
        value = other.value;
        row = other.row;
        return *this;
    }

    AttributeKey key;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8; // padding

  public:
    double value;
    const AttributeRow *row;
};

class AttributeIndexItem : public ConstAttributeIndexItem {
  public:
    AttributeIndexItem(const AttributeKey &k, double v, AttributeRow &r)
        : ConstAttributeIndexItem(k, v, r), mutableRow(&r) {}
    AttributeIndexItem(const AttributeIndexItem &other)
        : ConstAttributeIndexItem(other), mutableRow(other.mutableRow) {}
    AttributeIndexItem &operator=(const AttributeIndexItem &other) {
        if (this == &other) {
            return *this;
        }
        ConstAttributeIndexItem::operator=(other);
        mutableRow = other.mutableRow;
        return *this;
    }

    AttributeRow *mutableRow;
};

inline bool operator<(const ConstAttributeIndexItem &lhs, const ConstAttributeIndexItem &rhs) {
    return lhs.value < rhs.value;
}

std::vector<ConstAttributeIndexItem> makeAttributeIndex(const AttributeTable &table, int colIndex);
std::vector<AttributeIndexItem> makeAttributeIndex(AttributeTable &table, int colIndex);
std::pair<std::vector<AttributeIndexItem>::iterator, std::vector<AttributeIndexItem>::iterator>
getIndexItemsInValueRange(std::vector<AttributeIndexItem> &index, AttributeTable &table,
                          float fromValue, float toValue);
