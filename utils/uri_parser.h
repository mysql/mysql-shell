/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _URI_PARSER_
#define _URI_PARSER_

#include "base_tokenizer.h"
#include "shellcore/types.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <stdexcept>

// Avoid warnings from includes of other project and protobuf
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4018 4996)
#endif

#include <boost/function.hpp>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning (pop)
#endif

namespace shcore
{
  class SHCORE_PUBLIC Uri_parser
  {
  public:
    Uri_parser();
    bool parse(const std::string& input);

    std::string get_scheme() { return _scheme; }
    std::string get_scheme_ext() { return _scheme_ext; }
    std::string get_user() { return _user; }
    std::string get_password() { return _password; }
    std::string get_host() { return _host; }
    int get_port() { return _port; }
    std::string get_socket() { return _sock; }
    std::string get_db() { return _db; }

    bool has_password() { return _has_password; }
    bool has_port() { return _has_port; }

    std::string get_attribute(const std::string& name){ return _attributes[name]; }
  private:
    std::map<std::string, std::string> _components;
    std::map<std::string, size_t> _offsets;

    std::string _scheme;
    std::string _scheme_ext;
    std::string _user;
    bool _has_password;
    std::string _password;
    std::string _host;
    bool _has_port;
    int _port;
    std::string _sock;
    std::string _db;

    void parse_scheme();
    void parse_userinfo();
    void parse_host();
    void parse_path();
    void parse_query();
    void parse_attribute(const std::string& remaining, size_t offset);
    void parse_value(const std::string& attribute, const std::string& remaining, size_t offset);

    static std::string DELIMITERS;
    static std::string SUBDELIMITERS;
    static std::string ALPHA;
    static std::string DIGIT;
    static std::string HEXDIG;
    static std::string ALPHANUMERIC;
    static std::string UNRESERVED;
    static std::string RESERVED;

    std::map<std::string, std::string> _attributes;
  };

  class Parser_error : public std::runtime_error
  {
  public:
    Parser_error(const std::string& msg) : std::runtime_error(msg)
    {
    }
  };
}

#endif
