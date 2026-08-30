// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/stringutils.hpp"

class FileProperties {
  protected:
    std::string m_createPerson;
    std::string m_createOrganization;
    std::string m_createDate;
    std::string m_createProgram;
    std::string m_title;
    std::string m_location;
    std::string m_description;

  public:
    FileProperties()
        : m_createPerson(), m_createOrganization(), m_createDate(), m_createProgram(), m_title(),
          m_location(), m_description() {}
    virtual ~FileProperties() {}
    //
    void setProperties(const std::string &person, const std::string &organization,
                       const std::string &date, const std::string &program) {
        m_createPerson = person;
        m_createOrganization = organization;
        m_createDate = date;
        m_createProgram = program;
    }
    void setTitle(const std::string &title) { m_title = title; }
    void setLocation(const std::string &location) { m_location = location; }
    void setDescription(const std::string &description) { m_description = description; }
    //
    const std::string &getPerson() const { return m_createPerson; }
    const std::string &getOrganization() const { return m_createOrganization; }
    const std::string &getDate() const { return m_createDate; }
    const std::string &getProgram() const { return m_createProgram; }
    const std::string &getTitle() const { return m_title; }
    const std::string &getLocation() const { return m_location; }
    const std::string &getDescription() const { return m_description; }
    //
    bool read(std::istream &stream);
    bool write(std::ostream &stream) const;
};

inline bool FileProperties::read(std::istream &stream) {
    m_createPerson = dXstring::readString(stream);
    m_createOrganization = dXstring::readString(stream);
    m_createDate = dXstring::readString(stream);
    m_createProgram = dXstring::readString(stream);
    m_title = dXstring::readString(stream);
    m_location = dXstring::readString(stream);
    m_description = dXstring::readString(stream);

    return true;
}

inline bool FileProperties::write(std::ostream &stream) const {
    dXstring::writeString(stream, m_createPerson);
    dXstring::writeString(stream, m_createOrganization);
    dXstring::writeString(stream, m_createDate);
    dXstring::writeString(stream, m_createProgram);
    dXstring::writeString(stream, m_title);
    dXstring::writeString(stream, m_location);
    dXstring::writeString(stream, m_description);

    return true;
}
