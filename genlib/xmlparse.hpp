// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct xmlelement {
    std::string name;
    bool closetag;

  private:
    [[maybe_unused]] unsigned _padding0 : 3 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  public:
    std::map<std::string, std::string> attributes;
    std::vector<xmlelement> subelements;
    xmlelement()
        : name(), closetag(false), _padding0(0), _padding1(0), attributes(), subelements() {}
    bool parse(std::ifstream &stream, bool parsesubelements = false);
    friend std::ostream &operator<<(std::ostream &stream, const xmlelement &elem);

  protected:
    bool subparse(std::ifstream &stream);
    [[noreturn]] void badcharacter(char c, const std::string &location);
};

struct xmlerror {
    std::string error;
    xmlerror(const std::string &e = std::string()) : error(e) {}
};
