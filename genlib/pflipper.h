// SPDX-FileCopyrightText: 1996-2011 Alasdair Turner (a.turner@ucl.ac.uk)
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

template <class T> class pflipper {
  protected:
    T m_contents[2];
    short m_parity;

  public:
    pflipper() : m_parity(0) {}
    pflipper(const T &a, const T &b) : m_parity(0) {

        m_contents[0] = a;
        m_contents[1] = b;
    }
    pflipper(const pflipper &f) : m_parity(f.m_parity) {

        m_contents[0] = f.m_contents[0];
        m_contents[1] = f.m_contents[1];
    }
    virtual ~pflipper() {}
    pflipper &operator=(const pflipper &f) {
        if (this != &f) {
            m_parity = f.m_parity;
            m_contents[0] = f.m_contents[0];
            m_contents[1] = f.m_contents[1];
        }
        return *this;
    }
    void flip() { m_parity = (m_parity == 0) ? 1 : 0; }
    T &a() { return m_contents[m_parity]; }
    T &b() { return m_contents[(m_parity == 0) ? 1 : 0]; }
    const T &a() const { return m_contents[m_parity]; }
    const T &b() const { return m_contents[(m_parity == 0) ? 1 : 0]; }
};