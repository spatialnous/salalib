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

#pragma once
#include "attributetable.h"
#include "attributetableview.h"
#include "mgraph_consts.h"
#include "pafcolor.h"

namespace dXreimpl {
    inline void pushSelectionToLayer(AttributeTable &table, LayerManager &layerManager,
                                     const std::string &layerName) {
        size_t layerIndex = layerManager.addLayer(layerName);
        LayerManager::KeyType layerKey = layerManager.getKey(layerIndex);
        for (auto &item : table) {
            auto &row = item.getRow();
            if (isObjectVisible(layerManager, row) && row.isSelected()) {
                addLayerToObject(item.getRow(), layerKey);
            }
        }

        layerManager.setLayerVisible(layerIndex);
    }

    inline PafColor getDisplayColor(const AttributeKey &key, const AttributeRow &row,
                                    const AttributeTableView &tableView,
                                    bool checkSelectionStatus = false) {
        if (checkSelectionStatus && row.isSelected()) {
            return PafColor(SALA_SELECTED_COLOR);
        }

        PafColor color;
        return color.makeColor(tableView.getNormalisedValue(key, row),
                               tableView.getDisplayParams());
    }

} // namespace dXreimpl

struct OrderedIntPair {
    int a;
    int b;
    OrderedIntPair(int x = -1, int y = -1) {
        a = (int)x < y ? x : y;
        b = (int)x < y ? y : x;
    }
    // inlined at end of file
    friend bool operator==(const OrderedIntPair &x, const OrderedIntPair &y);
    friend bool operator!=(const OrderedIntPair &x, const OrderedIntPair &y);
    friend bool operator<(const OrderedIntPair &x, const OrderedIntPair &y);
    friend bool operator>(const OrderedIntPair &x, const OrderedIntPair &y);
};

// note: these are made with a is always less than b
inline bool operator==(const OrderedIntPair &x, const OrderedIntPair &y) {
    return (x.a == y.a && x.b == y.b);
}
inline bool operator!=(const OrderedIntPair &x, const OrderedIntPair &y) {
    return (x.a != y.a || x.b != y.b);
}
inline bool operator<(const OrderedIntPair &x, const OrderedIntPair &y) {
    return ((x.a == y.a) ? x.b < y.b : x.a < y.a);
}
inline bool operator>(const OrderedIntPair &x, const OrderedIntPair &y) {
    return ((x.a == y.a) ? x.b > y.b : x.a > y.a);
}
