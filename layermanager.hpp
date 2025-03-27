// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/exceptions.hpp"

#include <cstdint>
#include <string>

class LayerManager {
  public:
    typedef int64_t KeyType;
    virtual size_t addLayer(const std::string &layerName) = 0;
    virtual const std::string &getLayerName(size_t index) const = 0;
    virtual size_t getLayerIndex(const std::string &layerName) const = 0;
    virtual void setLayerVisible(size_t layerIndex, bool visible = true) = 0;
    virtual bool isLayerVisible(size_t layerIndex) const = 0;
    virtual size_t getNumLayers() const = 0;

    virtual KeyType getKey(size_t layerIndex) const = 0;
    virtual bool isVisible(const KeyType &key) const = 0;

    virtual void read(std::istream &stream) = 0;
    virtual void write(std::ostream &stream) const = 0;

    virtual ~LayerManager() {}

  public:
    class OutOfLayersException : public depthmapX::BaseException {
      public:
        OutOfLayersException() {}
        OutOfLayersException(const std::string &message)
            : depthmapX::BaseException(message.c_str()) {}
    };

    class DuplicateKeyException : public depthmapX::BaseException {
      public:
        DuplicateKeyException() {}
        DuplicateKeyException(const std::string &message)
            : depthmapX::BaseException(message.c_str()) {}
    };
};

class LayerAware {
  public:
    virtual void setLayerKey(const LayerManager::KeyType &key) { m_layerKey = key; }

    virtual const LayerManager::KeyType &getLayerKey() const { return m_layerKey; }

    virtual ~LayerAware() {}

  protected:
    LayerManager::KeyType m_layerKey;
    LayerAware() : m_layerKey() {}
};

inline bool isObjectVisible(const LayerManager &manager, const LayerAware &object) {
    return manager.isVisible(object.getLayerKey());
}

inline void addLayerToObject(LayerAware &object, const LayerManager::KeyType &layerKey) {
    object.setLayerKey(object.getLayerKey() | layerKey);
}
