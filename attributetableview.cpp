// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "attributetableview.h"

AttributeTableView::AttributeTableView(const AttributeTable &table)
    : m_table(table), m_displayColumn(-1) {}

void AttributeTableView::setDisplayColIndex(int columnIndex) {
    if (columnIndex < -1) {
        m_displayColumn = -2;
        m_index.clear();
        return;
    }
    // recalculate the index even if it's the same column in case stuff has
    // changed
    m_index = makeAttributeIndex(m_table, columnIndex);
    m_displayColumn = columnIndex;
}

float AttributeTableView::getNormalisedValue(const AttributeKey &key,
                                             const AttributeRow &row) const {
    if (m_displayColumn < 0) {
        auto endIter = m_table.end();
        --endIter;
        return (float)key.value / (float)endIter->getKey().value;
    }
    return row.getNormalisedValue(m_displayColumn);
}

const DisplayParams &AttributeTableView::getDisplayParams() const {
    if (m_displayColumn < 0) {
        return m_table.getDisplayParams();
    }
    return m_table.getColumn(m_displayColumn).getDisplayParams();
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
int AttributeTableHandle::findInIndex(const AttributeKey &key) {

    auto iter = std::find_if(m_mutableIndex.begin(), m_mutableIndex.end(), index_item_key(key));
    if (iter != m_mutableIndex.end()) {
        return (std::distance(m_mutableIndex.begin(), iter));
    }
    return -1;
}
