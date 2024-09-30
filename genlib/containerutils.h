// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <map>
#include <vector>

namespace depthmapX {

    template <typename T> void findAndErase(std::vector<T> &vec, T element) {
        auto it = std::find(vec.begin(), vec.end(), element);
        if (it != vec.end())
            vec.erase(it);
    }

    template <typename T> bool addIfNotExists(std::vector<T> &vec, T element) {
        auto it = std::find(vec.begin(), vec.end(), element);
        if (it == vec.end()) {
            vec.push_back(element);
            return true;
        }
        return false;
    }

    template <typename K, typename V>
    bool addIfNotExists(std::map<K, V> &map, const K &key, const V &value) {
        auto it = map.find(key);
        if (it == map.end()) {
            map[key] = value;
            return true;
        }
        return false;
    }

    template <typename K, typename V>
    typename std::map<K, V>::const_iterator getMapAtIndex(const std::map<K, V> &m, size_t idx) {
        auto iter = m.begin();
        std::advance(iter, idx);
        return iter;
    }

    template <typename K, typename V>
    typename std::map<K, V>::iterator getMapAtIndex(std::map<K, V> &m, size_t idx) {
        auto iter = m.begin();
        std::advance(iter, idx);
        return iter;
    }

    template <typename K, typename V> int findIndexFromKey(const std::map<K, V> &m, K key) {
        auto iter = m.find(key);
        return iter == m.end() ? -1 : std::distance(m.begin(), iter);
    }

    template <typename TContainer, typename TValue>
    typename TContainer::iterator findBinary(TContainer &container, const TValue val) {
        auto res = std::lower_bound(container.begin(), container.end(), val);
        if (res == container.end() || val < *res) {
            return container.end();
        }
        return res;
    }

    template <typename TContainer, typename TValue>
    typename TContainer::const_iterator findBinary(const TContainer &container, const TValue val) {
        auto res = std::lower_bound(container.begin(), container.end(), val);
        if (res == container.end() || val < *res) {
            return container.end();
        }
        return res;
    }

    template <typename T>
    typename std::vector<T>::iterator insert_sorted(std::vector<T> &vec, T const &item) {
        return vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
    }

} // namespace depthmapX
