// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "attributetable.hpp"
#include "attributetableindex.hpp"

#include <functional>

class AttributeTableView {
  public:
    AttributeTableView(const AttributeTable &table);

    const AttributeTable &table;

    // columnIndex < 0 -> not set
    virtual void setDisplayColIndex(int columnIndex);
    int getDisplayColIndex() const { return m_displayColumn; }

    float getNormalisedValue(const AttributeKey &key, const AttributeRow &row) const;
    const DisplayParams &getDisplayParams() const;

    typedef std::vector<ConstAttributeIndexItem> ConstIndex;
    const ConstIndex &getConstTableIndex() const { return m_index; }

    const AttributeColumn &getDisplayedColumn() const;

  private:
    ConstIndex m_index;
    int m_displayColumn;

    [[maybe_unused]] unsigned _padding0 : 4 * 8;
};

class AttributeTableHandle : public AttributeTableView {
  public:
    AttributeTableHandle(AttributeTable &tablIn)
        : AttributeTableView(tablIn), m_mutableTable(tablIn), m_mutableIndex() {}
    virtual ~AttributeTableHandle() {}
    typedef std::vector<AttributeIndexItem> Index;
    const Index &getTableIndex() const { return m_mutableIndex; }
    void setDisplayColIndex(int columnIndex) override;
    Index::iterator::difference_type findInIndex(const AttributeKey &key);

  private:
    AttributeTable &m_mutableTable;
    Index m_mutableIndex;
};

struct index_item_key : public std::function<bool(AttributeKey)> {
    explicit index_item_key(const AttributeKey &baselineIn) : baseline(baselineIn) {}
    bool operator()(const AttributeIndexItem &arg) { return arg.key.value == baseline.value; }
    const AttributeKey &baseline;
};
