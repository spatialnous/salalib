// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

struct OrderedSizeTPair;

struct OrderedIntPair {
    int a;
    int b;
    OrderedIntPair(int x = -1, int y = -1) {
        a = (int)x < y ? x : y;
        b = (int)x < y ? y : x;
    }
    OrderedIntPair(const OrderedSizeTPair &osp);
    // inlined at end of file
    friend bool operator==(const OrderedIntPair &x, const OrderedIntPair &y);
    friend bool operator!=(const OrderedIntPair &x, const OrderedIntPair &y);
    friend bool operator<(const OrderedIntPair &x, const OrderedIntPair &y);
    friend bool operator>(const OrderedIntPair &x, const OrderedIntPair &y);
};

struct OrderedSizeTPair {
    size_t a;
    size_t b;
    OrderedSizeTPair(size_t x = static_cast<size_t>(-1), size_t y = static_cast<size_t>(-1)) {
        a = (size_t)x < y ? x : y;
        b = (size_t)x < y ? y : x;
    }
    OrderedSizeTPair(const OrderedIntPair &oip);
    // inlined at end of file
    friend bool operator==(const OrderedSizeTPair &x, const OrderedSizeTPair &y);
    friend bool operator!=(const OrderedSizeTPair &x, const OrderedSizeTPair &y);
    friend bool operator<(const OrderedSizeTPair &x, const OrderedSizeTPair &y);
    friend bool operator>(const OrderedSizeTPair &x, const OrderedSizeTPair &y);
};

inline OrderedIntPair::OrderedIntPair(const OrderedSizeTPair &osp)
    : OrderedIntPair(static_cast<int>(osp.a), static_cast<int>(osp.b)) {}

inline OrderedSizeTPair::OrderedSizeTPair(const OrderedIntPair &oip)
    : OrderedSizeTPair(static_cast<size_t>(oip.a), static_cast<size_t>(oip.b)) {}

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

// note: these are made with a is always less than b
inline bool operator==(const OrderedSizeTPair &x, const OrderedSizeTPair &y) {
    return (x.a == y.a && x.b == y.b);
}
inline bool operator!=(const OrderedSizeTPair &x, const OrderedSizeTPair &y) {
    return (x.a != y.a || x.b != y.b);
}
inline bool operator<(const OrderedSizeTPair &x, const OrderedSizeTPair &y) {
    return ((x.a == y.a) ? x.b < y.b : x.a < y.a);
}
inline bool operator>(const OrderedSizeTPair &x, const OrderedSizeTPair &y) {
    return ((x.a == y.a) ? x.b > y.b : x.a > y.a);
}
