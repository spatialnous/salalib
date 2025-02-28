// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "layermanagerimpl.hpp"

#include "genlib/stringutils.hpp"

LayerManagerImpl::LayerManagerImpl() : m_visibleLayers(1), m_layers(), m_layerLookup() {
    m_layers.push_back("Everything");
    m_layerLookup["Everything"] = 0;
}

size_t LayerManagerImpl::addLayer(const std::string &layerName) {
    size_t newLayerIndex = m_layers.size();
    if (newLayerIndex > 63) {
        throw OutOfLayersException();
    }
    auto result = m_layerLookup.insert(std::make_pair(layerName, newLayerIndex));
    if (!result.second) {
        throw DuplicateKeyException(std::string("Trying to insert duplicate key: ") + layerName);
    }
    m_layers.push_back(layerName);
    return newLayerIndex;
}

const std::string &LayerManagerImpl::getLayerName(size_t index) const {
    checkIndex(index);
    return m_layers[index];
}

size_t LayerManagerImpl::getLayerIndex(const std::string &layerName) const {
    auto iter = m_layerLookup.find(layerName);
    if (iter == m_layerLookup.end()) {
        throw std::out_of_range("Unknown layer name");
    }
    return iter->second;
}

void LayerManagerImpl::setLayerVisible(size_t layerIndex, bool visible) {
    checkIndex(layerIndex);
    if (layerIndex == 0) {
        // this it the everything layer - if switching on just show everything, else
        // switch everything off
        m_visibleLayers = visible ? 1 : 0;
        return;
    }
    int64_t layerValue = (static_cast<KeyType>(1)) << layerIndex;

    // if visible, switch on this layer and switch everything layer off, else just
    // switch off this layer
    if (visible) {
        m_visibleLayers = (m_visibleLayers | layerValue) & (~0x1);
    } else {
        m_visibleLayers &= (~layerValue);
    }
}

bool LayerManagerImpl::isLayerVisible(size_t layerIndex) const {
    checkIndex(layerIndex);
    return isVisible(getKey(layerIndex));
}

bool LayerManagerImpl::isVisible(const KeyType &key) const { return (m_visibleLayers & key) != 0; }

void LayerManagerImpl::read(std::istream &stream) {
    m_layerLookup.clear();
    m_layers.clear();
    KeyType dummy;
    stream.read(reinterpret_cast<char *>(&dummy), sizeof(dummy));
    stream.read(reinterpret_cast<char *>(&m_visibleLayers), sizeof(m_visibleLayers));
    int count;
    stream.read(reinterpret_cast<char *>(&count), sizeof(int));
    for (size_t i = 0; i < static_cast<size_t>(count); ++i) {
        stream.read(reinterpret_cast<char *>(&dummy), sizeof(dummy));
        m_layers.push_back(dXstring::readString(stream));
        m_layerLookup[m_layers.back()] = i;
    }
}

void LayerManagerImpl::write(std::ostream &stream) const {
    //    KeyType availableLayers = 0;
    //    for (size_t i = m_layers.size(); i < 64; ++i)
    //    {
    //        availableLayers |= ((KeyType)1) << i;
    //    }

    // TODO: (PK) While the above seems to me like the sane solution and
    // potentially what the intention was in the original implementation, the one
    // in the old attributes table seems to be messed up because of its starting
    // value. Therefore, for temporary binary compatibility at least until the new
    // attributes table is in place the original solution is used here as found
    // in AttributeTable::selectionToLayer():

    int64_t availableLayers = 0xffffffff << (32 + 0xfffffffe);
    // should have been:
    // int64_t availableLayers = (int64_t(0xffffffff) << 32) + 0xfffffffe;

    for (size_t i = 1; (i < 64) & (i < m_layers.size()); ++i) {
        int loc = 1;
        while (loc < 64 && ((availableLayers >> loc) & 0x1) == 0) {
            loc++;
        }
        if (loc == 64) {
            // too many layers -- maximum 64
            throw OutOfLayersException();
        }
        int64_t newlayer = static_cast<int64_t>(0x1) << static_cast<int64_t>(loc);
        // now layer has been found, eliminate from available layers
        // and add a lookup for the name
        availableLayers = (availableLayers & (~newlayer));
    }

    stream.write(reinterpret_cast<const char *>(&availableLayers), sizeof(KeyType));
    stream.write(reinterpret_cast<const char *>(&m_visibleLayers), sizeof(KeyType));
    auto sizeAsInt = static_cast<int>(m_layers.size());
    stream.write(reinterpret_cast<const char *>(&sizeAsInt), sizeof(int));

    availableLayers = 0xffffffff << (32 + 0xfffffffe);
    int64_t newlayer = 0x1;
    stream.write(reinterpret_cast<const char *>(&newlayer), sizeof(KeyType));
    dXstring::writeString(stream, m_layers[0]);
    for (size_t i = 1; i < m_layers.size(); ++i) {
        // again keeping binary comatibility
        //        KeyType key = ((KeyType)1) << i;
        //        stream.write((const char *)&key, sizeof(KeyType));
        //        dXstring::writeString(stream,m_layers[i]);

        int loc = 1;
        while (loc < 64 && ((availableLayers >> loc) & 0x1) == 0) {
            loc++;
        }
        if (loc == 64) {
            // too many layers -- maximum 64
            throw OutOfLayersException();
        }
        newlayer = static_cast<int64_t>(0x1) << static_cast<int64_t>(loc);
        stream.write(reinterpret_cast<const char *>(&newlayer), sizeof(KeyType));
        dXstring::writeString(stream, m_layers[i]);
    }
}

LayerManager::KeyType LayerManagerImpl::getKey(size_t layerIndex) const {
    return (static_cast<int64_t>(1)) << layerIndex;
}

void LayerManagerImpl::checkIndex(size_t index) const {
    if (index >= m_layers.size()) {
        throw std::out_of_range("Invalid layer index");
    }
}
