// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/stringutils.h"

class FileProperties {
  protected:
    std::string m_create_person;
    std::string m_create_organization;
    std::string m_create_date;
    std::string m_create_program;
    std::string m_title;
    std::string m_location;
    std::string m_description;

  public:
    FileProperties() { ; }
    virtual ~FileProperties() { ; }
    //
    void setProperties(const std::string &person, const std::string &organization,
                       const std::string &date, const std::string &program) {
        m_create_person = person;
        m_create_organization = organization;
        m_create_date = date;
        m_create_program = program;
    }
    void setTitle(const std::string &title) { m_title = title; }
    void setLocation(const std::string &location) { m_location = location; }
    void setDescription(const std::string &description) { m_description = description; }
    //
    const std::string &getPerson() const { return m_create_person; }
    const std::string &getOrganization() const { return m_create_organization; }
    const std::string &getDate() const { return m_create_date; }
    const std::string &getProgram() const { return m_create_program; }
    const std::string &getTitle() const { return m_title; }
    const std::string &getLocation() const { return m_location; }
    const std::string &getDescription() const { return m_description; }
    //
    bool read(std::istream &stream);
    bool write(std::ostream &stream);
};

inline bool FileProperties::read(std::istream &stream) {
    m_create_person = dXstring::readString(stream);
    m_create_organization = dXstring::readString(stream);
    m_create_date = dXstring::readString(stream);
    m_create_program = dXstring::readString(stream);
    m_title = dXstring::readString(stream);
    m_location = dXstring::readString(stream);
    m_description = dXstring::readString(stream);

    return true;
}

inline bool FileProperties::write(std::ostream &stream) {
    dXstring::writeString(stream, m_create_person);
    dXstring::writeString(stream, m_create_organization);
    dXstring::writeString(stream, m_create_date);
    dXstring::writeString(stream, m_create_program);
    dXstring::writeString(stream, m_title);
    dXstring::writeString(stream, m_location);
    dXstring::writeString(stream, m_description);

    return true;
}
