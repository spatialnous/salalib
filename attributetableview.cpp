// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "attributetableview.hpp"

AttributeTableView::AttributeTableView(const AttributeTable &tableIn)
    : table(tableIn), m_index(), m_displayColumn(-1), _padding0(0) {}

void AttributeTableView::setDisplayColIndex(int columnIndex) {
    if (columnIndex < -1) {
        m_displayColumn = -2;
        m_index.clear();
        return;
    }
    // recalculate the index even if it's the same column in case stuff has
    // changed
    m_index = makeAttributeIndex(table, columnIndex);
    m_displayColumn = columnIndex;
}

float AttributeTableView::getNormalisedValue(const AttributeKey &key,
                                             const AttributeRow &row) const {
    if (m_displayColumn < 0) {
        auto endIter = table.end();
        --endIter;
        return static_cast<float>(key.value) / static_cast<float>(endIter->getKey().value);
    }
    return row.getNormalisedValue(static_cast<size_t>(m_displayColumn));
}

const DisplayParams &AttributeTableView::getDisplayParams() const {
    if (m_displayColumn < 0) {
        return table.getDisplayParams();
    }
    return table.getColumn(static_cast<size_t>(m_displayColumn)).getDisplayParams();
}

void AttributeTableHandle::setDisplayColIndex(int columnIndex) {
    if (columnIndex < -1) {
        m_mutableIndex.clear();
    } else {
        // recalculate the index even if it's the same column in case stuff has
        // changed
        m_mutableIndex = makeAttributeIndex(m_mutableTable, columnIndex);
    }
    AttributeTableView::setDisplayColIndex(columnIndex);
}
AttributeTableHandle::Index::iterator::difference_type
AttributeTableHandle::findInIndex(const AttributeKey &key) {

    auto iter = std::find_if(m_mutableIndex.begin(), m_mutableIndex.end(), index_item_key(key));
    if (iter != m_mutableIndex.end()) {
        return (std::distance(m_mutableIndex.begin(), iter));
    }
    return -1;
}
