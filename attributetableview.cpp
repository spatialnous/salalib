// Copyright (C) 2017 Christian Sailer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
