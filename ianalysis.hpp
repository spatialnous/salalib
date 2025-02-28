// SPDX-FileCopyrightText: 2020-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "analysisresult.hpp"

#include "genlib/comm.hpp"

#include <set>
#include <string>

class IAnalysis {
  protected:
    class AnalysisColumn {
        AttributeColumnStats m_stats;
        std::vector<float> m_data;

        void updateStats(float val, float oldVal) {
            if (m_stats.total < 0) {
                m_stats.total = val;
            } else {
                m_stats.total += val;
                m_stats.total -= oldVal;
            }
            if (val > m_stats.max) {
                m_stats.max = val;
            }
            if (m_stats.min < 0 || val < m_stats.min) {
                m_stats.min = val;
            }
        }

      public:
        AnalysisColumn(size_t n = 0, float defValue = 0) : m_stats(), m_data(n, defValue) {}
        void setValue(size_t index, float newValue, bool updateColumnStats = false) {
            if (updateColumnStats) {
                float oldVal = m_data[index];
                if (oldVal < 0.0f) {
                    oldVal = 0.0f;
                }
                updateStats(newValue, oldVal);
            }
            m_data[index] = newValue;
        }

        float getValue(size_t index) { return m_data[index]; }
        const AttributeColumnStats getStats() { return m_stats; }
    };

  public:
    virtual std::string getAnalysisName() const = 0;
    virtual AnalysisResult run(Communicator *comm) = 0;
    virtual ~IAnalysis() {}
};
