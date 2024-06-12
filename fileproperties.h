// Copyright (C) 2011-2012, Tasos Varoudis

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
