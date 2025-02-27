// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "displayparams.hpp"
#include "layermanager.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

///
/// Namespace to hold known attributes
///
namespace AttributeName {
    const char *const REF = "Ref";
}

///
/// Interface to an attribute row
///
class AttributeRow : public LayerAware {
  public:
    virtual float getValue(const std::string &column) const = 0;
    virtual float getValue(size_t index) const = 0;
    virtual float getNormalisedValue(size_t index) const = 0;
    virtual AttributeRow &setValue(const std::string &column, float value) = 0;
    virtual AttributeRow &setValue(size_t index, float value) = 0;
    virtual AttributeRow &incrValue(size_t index, float value = 1.0f) = 0;
    virtual AttributeRow &incrValue(const std::string &colName, float value = 1.0f) = 0;

    ~AttributeRow() override {}
};

///
/// Container for attribute column stats - really just POD to pass them around
///
struct AttributeColumnStats {
    AttributeColumnStats(double minimum, double maximum, double tot, double vTot, double vMin,
                         double vMax)
        : min(minimum), max(maximum), total(tot), visibleTotal(vTot), visibleMin(vMin),
          visibleMax(vMax) {}

    AttributeColumnStats() : AttributeColumnStats(-1.0, -1.0, -1.0, -1.0, -1.0, -1.0) {}

    double min;
    double max;
    double total;
    double visibleTotal;
    double visibleMin;
    double visibleMax;
};

///
/// Interface to an attribute column
///
class AttributeColumn {
  public:
    virtual const std::string &getName() const = 0;
    virtual bool isLocked() const = 0;
    virtual void setLock(bool lock) = 0;
    virtual bool isHidden() const = 0;
    virtual void setHidden(bool hidden) = 0;
    virtual void setDisplayParams(const DisplayParams &params) = 0;
    virtual const DisplayParams &getDisplayParams() const = 0;

    virtual const std::string &getFormula() const = 0;
    virtual void setFormula(std::string newFormula) = 0;

    virtual const AttributeColumnStats &getStats() const = 0;

    // stats are mutable - we need to be able to update them all the time,
    // even when not allowed to modify the column settings
    virtual void updateStats(float val, float oldVal = 0.0f) const = 0;
    virtual void setStats(const AttributeColumnStats &stats) const = 0;

    virtual ~AttributeColumn() {}
};

///
/// Interface to an Attribute Column Manager
/// This handles the mapping from column name to index and actually storing the column
/// implementations Implemented by AttributeTable
///
class AttributeColumnManager {
  public:
    virtual size_t getNumColumns() const = 0;
    virtual size_t getColumnIndex(const std::string &name) const = 0;
    virtual std::optional<size_t> getColumnIndexOptional(const std::string &name) const = 0;
    virtual const AttributeColumn &getColumn(size_t index) const = 0;
    virtual const std::string &getColumnName(size_t index) const = 0;
    virtual bool hasColumn(const std::string &name) const = 0;
};

// Implementation of AttributeColumn

class AttributeColumnImpl : public AttributeColumn, AttributeColumnStats {
    // AttributeColumn interface
  public:
    AttributeColumnImpl(const std::string &name, const std::string &formula = std::string())
        : stats(), m_name(name), m_displayParams(), m_locked(false), m_hidden(false), _padding0(0),
          m_formula(formula) {}

    AttributeColumnImpl()
        : stats(), m_name(), m_displayParams(), m_locked(false), m_hidden(false), _padding0(0),
          m_formula() {}
    const std::string &getName() const override;
    bool isLocked() const override;
    void setLock(bool lock) override;
    bool isHidden() const override;
    void setHidden(bool hidden) override;
    const std::string &getFormula() const override;
    void setFormula(std::string newFormula) override;
    const AttributeColumnStats &getStats() const override;
    void setDisplayParams(const DisplayParams &params) override { m_displayParams = params; }
    const DisplayParams &getDisplayParams() const override { return m_displayParams; }

    void updateStats(float val, float oldVal = 0.0f) const override;
    void setStats(const AttributeColumnStats &stats) const override;

  public:
    // stats are mutable - we need to be able to update them all the time,
    // even when not allowed to modify the column settings
    mutable AttributeColumnStats stats;

    void setName(const std::string &name);
    // returns the physical column for comaptibility with the old attribute table
    size_t read(std::istream &stream);
    void write(std::ostream &stream, int physicalCol);

  private:
    std::string m_name;
    DisplayParams m_displayParams;
    bool m_locked;
    bool m_hidden;

    [[maybe_unused]] unsigned _padding0 : 2 * 8;

    std::string m_formula;
};

// Implementation of AttributeColumn that actually links to the keys of the table

class KeyColumn : public AttributeColumnImpl {
  public:
    KeyColumn() : AttributeColumnImpl() { setName(AttributeName::REF); }
};

// Implementation of AttributeRow
class AttributeRowImpl : public AttributeRow {
  public:
    AttributeRowImpl(const AttributeColumnManager &colManager)
        : m_data(colManager.getNumColumns(), -1.0), m_colManager(colManager) {
        m_layerKey = 1;
    }

    // AttributeRow interface
  public:
    float getValue(const std::string &column) const override;
    float getValue(size_t index) const override;
    float getNormalisedValue(size_t index) const override;
    AttributeRow &setValue(const std::string &column, float value) override;
    AttributeRow &setValue(size_t index, float value) override;
    AttributeRow &incrValue(const std::string &column, float value) override;
    AttributeRow &incrValue(size_t index, float value) override;

    void addColumn();
    void removeColumn(size_t index);

    void read(std::istream &stream);
    void write(std::ostream &stream);

  private:
    std::vector<float> m_data;
    const AttributeColumnManager &m_colManager;

    void checkIndex(size_t index) const;
};

///
/// \brief Small struct to make an attribute key distinguishable from an int
/// PixelRefs are serialised into an int (2 bytes x, 2 bytes y) for historic reason. This seems
/// dangerous and confusing as these are by no means indices, but look the same to the compiler and
/// the reader. This struct should disambiguate this...
///
struct AttributeKey {
    explicit AttributeKey(int val) : value(val) {}
    int value;

    bool operator<(const AttributeKey &other) const { return value < other.value; }

    void write(std::ostream &stream) const {
        stream.write(reinterpret_cast<const char *>(&value), sizeof(int));
    }

    void read(std::istream &stream) { stream.read(reinterpret_cast<char *>(&value), sizeof(int)); }
};

///
/// AttributeTable
///
class AttributeTable : public AttributeColumnManager {
    // AttributeTable "interface" - the actual table handling
  public:
    AttributeTable()
        : m_rows(), m_columnMapping(), m_columns(), m_keyColumn(), m_displayParams(), _padding0(0) {
    }
    virtual ~AttributeTable() {}
    AttributeTable(AttributeTable &&) = default;
    AttributeTable &operator=(AttributeTable &&) = default;
    AttributeTable(const AttributeTable &) = delete;
    AttributeTable &operator=(const AttributeTable &) = delete;

    ///
    /// \brief Get a row reference
    /// \param key of the row
    /// \return reference to row, throws if key not found
    ///
    AttributeRow &getRow(const AttributeKey &key);

    ///
    /// \brief Get a row const reference
    /// \param key of the row
    /// \return const reference to row, throws if key not found
    ///
    const AttributeRow &getRow(const AttributeKey &key) const;

    ///
    /// \brief Get a row pointer
    /// \param key of the row
    /// \return pointer to row, null if key not found
    ///
    AttributeRow *getRowPtr(const AttributeKey &key);

    ///
    /// \brief Get a row const pointer
    /// \param key of the row
    /// \return const pointer to row, null if key not found
    ///
    const AttributeRow *getRowPtr(const AttributeKey &key) const;
    size_t getRowIdx(const AttributeKey &key) const;
    AttributeRow &addRow(const AttributeKey &key);
    AttributeColumn &getColumn(size_t index);
    size_t insertOrResetColumn(const std::string &columnName,
                               const std::string &formula = std::string());
    size_t insertOrResetLockedColumn(const std::string &columnName,
                                     const std::string &formula = std::string());
    size_t getOrInsertColumn(const std::string &columnName,
                             const std::string &formula = std::string());
    size_t getOrInsertLockedColumn(const std::string &columnName,
                                   const std::string &formula = std::string());
    void removeRow(const AttributeKey &key);
    void removeColumn(size_t colIndex);
    void renameColumn(const std::string &oldName, const std::string &newName);
    size_t getNumRows() const { return m_rows.size(); }
    const DisplayParams &getDisplayParams() const { return m_displayParams; }
    void setDisplayParams(const DisplayParams &params) { m_displayParams = params; }
    void setDisplayParamsForAllAttributes(const DisplayParams &params);
    void read(std::istream &stream, LayerManager &layerManager);
    void write(std::ostream &stream, const LayerManager &layerManager);
    void clear();
    float getSelAvg(size_t columnIndex, std::set<int> &selSet) {
        float selTotal = 0;
        int selNum = 0;
        for (auto &pair : m_rows) {
            if (selSet.find(pair.first.value) != selSet.end()) {
                selTotal += pair.second->getValue(columnIndex);
                selNum++;
            }
        }
        if (selNum == 0) {
            return (-1);
        }
        return (selTotal / static_cast<float>(selNum));
    }

    // interface AttributeColumnManager
  public:
    size_t getColumnIndex(const std::string &name) const override;
    std::optional<size_t> getColumnIndexOptional(const std::string &name) const override;
    const AttributeColumn &getColumn(size_t index) const override;
    const std::string &getColumnName(size_t index) const override;
    size_t getNumColumns() const override;
    bool hasColumn(const std::string &name) const override;

    // TODO: Compatibility. Very inefficient method to retreive a column's index
    // if the set of columns was sorted
    size_t getColumnSortedIndex(size_t index) const;

  private:
    typedef std::map<AttributeKey, std::unique_ptr<AttributeRowImpl>> StorageType;
    StorageType m_rows;

    // Requires a transparent comparator to allow comparing with string_view
    // see https://stackoverflow.com/a/35525806
    std::map<std::string, size_t, std::less<>> m_columnMapping;
    std::vector<AttributeColumnImpl> m_columns;
    KeyColumn m_keyColumn;
    DisplayParams m_displayParams;

    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  private:
    void checkColumnIndex(size_t index) const;
    size_t addColumnInternal(const std::string &name, const std::string &formula);

    // warning - here be dragons!
    // This is the implementation of stl style iterators on attribute table, allowing efficient
    // iteration of rows without resorting to log(n) access via the map

  public:
    ///
    /// \brief The iterator_item class
    /// The value of an iterator over the table - we want to hide the actual storage details and
    /// just return references to rows and keys.
    class iterator_item {
      public:
        virtual const AttributeKey &getKey() const = 0;
        virtual const AttributeRow &getRow() const = 0;
        virtual AttributeRow &getRow() = 0;
        virtual ~iterator_item() {}
    };

  private:
    // implementation of the iterator_item, templated on iterator type to allow const and non-const
    // iterator
    template <typename iterator_type> class iterator_item_impl : public iterator_item {
      public:
        iterator_item_impl(const iterator_type &iterTp) : iter(iterTp) {}
        template <typename other_type>
        iterator_item_impl(const iterator_item_impl<other_type> &other) : iter(other.iter) {}

        template <typename other_type>
        iterator_item_impl<iterator_type> &operator=(const iterator_item_impl<other_type> &other) {
            iter = other.iter;
            return *this;
        }

        const AttributeKey &getKey() const override { return iter->first; }

        const AttributeRow &getRow() const override { return *iter->second; }

        AttributeRow &getRow() override { return *iter->second; }

        void forward() const { ++iter; }

        auto forward(int n) const { return std::next(iter, n); }

        void back() const { --iter; }

        template <typename other_type>
        bool operator==(const iterator_item_impl<other_type> &other) const {
            return iter == other.iter;
        }

      public:
        mutable iterator_type iter;
    };

    // iterator implementation - templated on iterator type for const/non-const
    template <typename iterator_type> class const_iterator_impl {
        template <typename other_type> friend class const_iterator_impl;

      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = iterator_item;

        const_iterator_impl(const iterator_type &iter) : m_item(iter) {}
        template <typename other_type>
        const_iterator_impl(const const_iterator_impl<other_type> &other) : m_item(other.m_item) {}
        template <typename other_type>
        const_iterator_impl &operator=(const const_iterator_impl<other_type> &other) {
            m_item = other.m_item;
            return *this;
        }

        const_iterator_impl &advance(int n) {
            m_item.forward(n);
            return *this;
        }

        const_iterator_impl &operator++() {
            m_item.forward();
            return *this;
        }
        const_iterator_impl operator++(int) {
            const_iterator_impl<iterator_type> tmp(*this);
            operator++();
            return tmp;
        }
        const_iterator_impl &operator--() {
            m_item.back();
            return *this;
        }
        const_iterator_impl operator--(int) {
            const_iterator_impl<iterator_type> tmp(*this);
            operator--();
            return tmp;
        }
        template <typename other_type>
        bool operator==(const const_iterator_impl<other_type> &rhs) const {
            return m_item == rhs.m_item;
        }
        template <typename other_type>
        bool operator!=(const const_iterator_impl<other_type> &rhs) const {
            return !(m_item == rhs.m_item);
        }
        const iterator_item &operator*() const { return m_item; }
        const iterator_item *operator->() const { return &m_item; }

      protected:
        iterator_item_impl<iterator_type> m_item;
    };

  public:
    // const iterator is just a typedef on the impl
    typedef const_iterator_impl<typename StorageType::const_iterator> const_iterator;

    // non const iterator needs some extra methods
    class iterator : public const_iterator_impl<typename StorageType::iterator> {
      public:
        iterator(const typename StorageType::iterator &iter)
            : const_iterator_impl<typename StorageType::iterator>(iter) {}
        template <typename other_type>
        iterator(const const_iterator_impl<other_type> &other)
            : const_iterator_impl<StorageType::iterator>(other.item) {
            // m_item = other.m_item;
        }
        template <typename other_type>
        iterator &operator=(const const_iterator_impl<other_type> &other) {
            const_iterator_impl<typename StorageType::iterator>::m_item = other.m_item;
            return *this;
        }
        iterator_item &operator*() {
            return const_iterator_impl<typename StorageType::iterator>::m_item;
        }
        iterator_item *operator->() {
            return &(const_iterator_impl<typename StorageType::iterator>::m_item);
        }
    };

    // stl style iteration methods
    const_iterator begin() const { return const_iterator(m_rows.begin()); }

    iterator begin() { return iterator(m_rows.begin()); }

    const_iterator end() const { return const_iterator(m_rows.end()); }

    iterator end() { return iterator(m_rows.end()); }

    const_iterator find(AttributeKey key) const { return const_iterator(m_rows.find(key)); }

    iterator find(AttributeKey key) { return iterator(m_rows.find(key)); }

    std::pair<const AttributeKey, std::unique_ptr<AttributeRowImpl>> &back() {
        return *m_rows.rbegin();
    }
};
