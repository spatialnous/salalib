// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/attributetable.h"
#include "salalib/attributetableview.h"
#include "salalib/layermanagerimpl.h"
#include "salalib/spacepix.h"

class AttributeMap : public PixelBase {

  protected:
    // private: // members
    std::unique_ptr<AttributeTable> m_attributes;
    std::unique_ptr<AttributeTableHandle> m_attribHandle;
    LayerManagerImpl m_layers;

  public:
    AttributeMap(std::unique_ptr<AttributeTable> attributes,
                 std::unique_ptr<AttributeTableHandle> attribHandle, LayerManagerImpl layers)
        : m_attributes(std::move(attributes)), m_attribHandle(std::move(attribHandle)),
          m_layers(std::move(layers)) {}
    AttributeMap(std::unique_ptr<AttributeTable> attributes)
        : m_attributes(std::move(attributes)),
          m_attribHandle(new AttributeTableHandle(*m_attributes)) {}
    virtual ~AttributeMap() {}

  public:
    size_t addAttribute(const std::string &name) { return m_attributes->insertOrResetColumn(name); }
    void removeAttribute(size_t col) { m_attributes->removeColumn(col); }
    // I don't want to do this, but every so often you will need to update this table
    // use const version by preference
    AttributeTable &getAttributeTable() { return *m_attributes.get(); }
    const AttributeTable &getAttributeTable() const { return *m_attributes.get(); }
    LayerManagerImpl &getLayers() { return m_layers; }
    const LayerManagerImpl &getLayers() const { return m_layers; }
    AttributeTableHandle &getAttributeTableHandle() { return *m_attribHandle.get(); }
    const AttributeTableHandle &getAttributeTableHandle() const { return *m_attribHandle.get(); }

  public:
    double getDisplayMinValue(size_t attributeIdx) const {
        return m_attributes->getColumn(attributeIdx).getStats().min;
    }

    double getDisplayMaxValue(size_t attributeIdx) const {
        return m_attributes->getColumn(attributeIdx).getStats().max;
    }

    const DisplayParams &getDisplayParams(size_t attributeIdx) const {
        return m_attributes->getColumn(attributeIdx).getDisplayParams();
    }
    // make a local copy of the display params for access speed:
    void setDisplayParams(const DisplayParams &dp, size_t attributeIdx, bool apply_to_all = false) {
        if (apply_to_all)
            m_attributes->setDisplayParams(dp);
        else
            m_attributes->getColumn(attributeIdx).setDisplayParams(dp);
    }
    //
  public:
    float getDisplayedSelectedAvg(size_t attributeIdx, std::set<int> &selSet) const {
        return (m_attributes->getSelAvg(attributeIdx, selSet));
    }
};
