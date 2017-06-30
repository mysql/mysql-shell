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
#include "utils/uri_data.h"
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

  static std::map<char, char> hex_literals;
};

class Parser_error : public std::runtime_error {
public:
  Parser_error(const std::string& msg) : std::runtime_error(msg) {}
};
}
}

#endif
