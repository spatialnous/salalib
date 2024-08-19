// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "layermanager.h"

#include <map>
#include <vector>

class LayerManagerImpl : public LayerManager {

    // LayerManager interface
  public:
    LayerManagerImpl();
    size_t addLayer(const std::string &layerName) override;
    const std::string &getLayerName(size_t index) const override;
    size_t getLayerIndex(const std::string &layerName) const override;
    void setLayerVisible(size_t layerIndex, bool visible) override;
    bool isLayerVisible(size_t layerIndex) const override;
    size_t getNumLayers() const override { return m_layers.size(); }

    KeyType getKey(size_t layerIndex) const override;
    bool isVisible(const KeyType &key) const override;

    void read(std::istream &stream) override;
    void write(std::ostream &stream) const override;

  private:
    void checkIndex(size_t index) const;

  private:
    int64_t m_visibleLayers;
    std::vector<std::string> m_layers;
    std::map<std::string, size_t> m_layerLookup;
};
