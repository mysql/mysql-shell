/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

namespace shcore {
namespace uri {
enum TargetType {
  Tcp,
  Socket,
  Pipe
};

class SHCORE_PUBLIC Uri_data {
public:
  Uri_data();
  std::string get_scheme() { return _scheme; }
  std::string get_scheme_ext() { return _scheme_ext; }
  std::string get_user() { return _user; }
  std::string get_password();

  TargetType get_type() { return _type; }
  std::string get_host();
  bool has_port() { return _has_port; }
  int get_port();
  bool has_ssl_mode() { return _ssl_mode != 0; }
  int get_ssl_mode();

  std::string get_pipe();
  std::string get_socket();

  std::string get_db() { return _db; }

  bool has_password() { return _has_password; }

  bool has_attribute(const std::string& name) { return _attributes.find(name) != _attributes.end(); }
  std::string get_attribute(const std::string& name, size_t index = 0) { return _attributes[name][index]; }

private:
  std::string _scheme;
  std::string _scheme_ext;
  std::string _user;
  bool _has_password;
  std::string _password;
  TargetType _type;
  std::string _host;
  std::string _socket;
  std::string _pipe;
  bool _has_port;
  int _port;
  int _ssl_mode;
  std::string _db;

  std::map<std::string, std::vector< std::string > > _attributes;

  friend class Uri_parser;
};

class SHCORE_PUBLIC Uri_parser {
public:
  Uri_parser();
  Uri_data parse(const std::string& input);

private:
  Uri_data *_data;
  std::string _input;
  BaseTokenizer _tokenizer;
  std::map<std::string, std::pair<size_t, size_t> > _chunks;

  void parse_scheme();
  void parse_userinfo();
  void parse_target();
  void parse_host();
  bool parse_ipv4(BaseTokenizer &tok, size_t &offset);
  void parse_ipv6(const std::pair<size_t, size_t> &range, size_t &offset);
  void parse_port(const std::pair<size_t, size_t> &range, size_t &offset);

  void parse_path();

  void parse_query();
  void parse_attribute(const std::pair<size_t, size_t>& range, size_t &offset);
  std::vector < std::string > parse_values(const std::pair<size_t, size_t>& range, size_t &offset);
  std::string parse_value(const std::pair<size_t, size_t>& range, size_t &offset, const std::string& finalizers);
  std::string parse_unencoded_value(const std::pair<size_t, size_t>& range, size_t &offset, const std::string& finalizers = "");
  std::string parse_encoded_value(const std::pair<size_t, size_t>& range, size_t &offset, const std::string& finalizers = "");

  char percent_decode(const std::string& value);
  std::string get_input_chunk(const std::pair<size_t, size_t>& range);

  bool input_contains(const std::string& what, size_t position = std::string::npos);
  void normalize_ssl_mode();

  static std::string DELIMITERS;
  static std::string SUBDELIMITERS;
  static std::string ALPHA;
  static std::string DIGIT;
  static std::string HEXDIG;
  static std::string DOT;
  static std::string ALPHANUMERIC;
  static std::string UNRESERVED;
  static std::string RESERVED;

  static std::map<char, char> hex_literals;
};

class Parser_error : public std::runtime_error {
public:
  Parser_error(const std::string& msg) : std::runtime_error(msg) {}
};
}
}

#endif
