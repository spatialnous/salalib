// SPDX-FileCopyrightText: 2018 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "attributetableindex.h"

std::vector<ConstAttributeIndexItem> makeAttributeIndex(const AttributeTable &table, int colIndex) {
    std::vector<ConstAttributeIndexItem> index;
    size_t numRows = table.getNumRows();
    if (numRows == 0) {
        return index;
    }
    index.reserve(numRows);
    // perturb the values to be sorted by so same values will be in order of
    // appearence in the map
    size_t idx = 0;
    if (colIndex == -1) {
        double perturbationFactor = 1e-9 / numRows;
        for (auto &item : table) {
            double value = (double)item.getKey().value;
            value += idx * perturbationFactor;

            index.push_back(ConstAttributeIndexItem(item.getKey(), value, item.getRow()));
            ++idx;
        }
    } else if (colIndex >= 0) {
        double perturbationFactor = table.getColumn(colIndex).getStats().max * 1e-9 / numRows;
        for (auto &item : table) {
            double value = item.getRow().getValue(colIndex);
            value += idx * perturbationFactor;

            index.push_back(ConstAttributeIndexItem(item.getKey(), value, item.getRow()));
            ++idx;
        }
    } else {
        throw std::out_of_range("Column index out of range");
    }
    std::sort(index.begin(), index.end());
    return index;
}

std::vector<AttributeIndexItem> makeAttributeIndex(AttributeTable &table, int colIndex) {
    std::vector<AttributeIndexItem> index;
    size_t numRows = table.getNumRows();
    if (numRows == 0) {
        return index;
    }
    index.reserve(numRows);
    // perturb the values to be sorted by so same values will be in order of
    // appearence in the map
    size_t idx = 0;
    if (colIndex == -1) {
        double perturbationFactor = 1e-9 / numRows;
        for (auto &item : table) {
            double value = (double)item.getKey().value;
            value += idx * perturbationFactor;

            index.push_back(AttributeIndexItem(item.getKey(), value, item.getRow()));
            ++idx;
        }
    } else if (colIndex >= 0) {
        double perturbationFactor = table.getColumn(colIndex).getStats().max * 1e-9 / numRows;
        for (auto &item : table) {
            double value = item.getRow().getValue(colIndex);
            value += idx * perturbationFactor;

            index.push_back(AttributeIndexItem(item.getKey(), value, item.getRow()));
            ++idx;
        }
    } else {
        throw std::out_of_range("Column index out of range");
    }
    std::sort(index.begin(), index.end());
    return index;
}

std::pair<std::vector<AttributeIndexItem>::iterator, std::vector<AttributeIndexItem>::iterator>
getIndexItemsInValueRange(std::vector<AttributeIndexItem> &index, AttributeTable &table,
                          float fromValue, float toValue) {
    AttributeKey dummykey(-1);
    AttributeRowImpl dummyrow(table);
    return std::pair<std::vector<AttributeIndexItem>::iterator,
                     std::vector<AttributeIndexItem>::iterator>(
        std::lower_bound(index.begin(), index.end(),
                         AttributeIndexItem(dummykey, fromValue, dummyrow)),
        std::upper_bound(index.begin(), index.end(),
                         AttributeIndexItem(dummykey, toValue, dummyrow)));
}
