/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_LIBS_DB_URI_PARSER_H_
#define MYSQLSHDK_LIBS_DB_URI_PARSER_H_

#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/base_tokenizer.h"
#include "mysqlshdk/libs/utils/nullable_options.h"
#include "scripting/types.h"

// Avoid warnings from includes of other project and protobuf
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4018 4996)
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#elif defined _MSC_VER
#pragma warning(pop)
#endif

namespace mysqlshdk {
namespace db {
namespace uri {
using mysqlshdk::utils::nullable_options::Comparison_mode;
class SHCORE_PUBLIC Uri_parser {
 public:
  Uri_parser();
  mysqlshdk::db::Connection_options parse(
      const std::string &input,
      Comparison_mode mode = Comparison_mode::CASE_INSENSITIVE);

 private:
  mysqlshdk::db::Connection_options *_data;
  std::string _input;
  shcore::BaseTokenizer _tokenizer;
  std::map<std::string, std::pair<size_t, size_t>> _chunks;

  void parse_scheme();
  void parse_userinfo();
  void parse_target();
  void parse_host();
  std::string parse_ipv4(size_t *offset);
  void parse_ipv6(const std::pair<size_t, size_t> &range, size_t *offset);
  void parse_port(const std::pair<size_t, size_t> &range, size_t *offset);

  void parse_path();

  void parse_query();
  void parse_attribute(const std::pair<size_t, size_t> &range, size_t *offset);
  bool is_value_array(const std::pair<size_t, size_t> &range);
  std::vector<std::string> parse_values(size_t *offset);
  std::string parse_value(const std::pair<size_t, size_t> &range,
                          size_t *offset, const std::string &finalizers);
  std::string parse_unencoded_value(const std::pair<size_t, size_t> &range,
                                    size_t *offset,
                                    const std::string &finalizers = "");
  std::string parse_encoded_value(const std::pair<size_t, size_t> &range,
                                  size_t *offset,
                                  const std::string &finalizers = "");

  char percent_decode(const std::string &value);
  std::string get_input_chunk(const std::pair<size_t, size_t> &range);

  bool input_contains(const std::string &what,
                      size_t position = std::string::npos);

  static std::map<char, char> hex_literals;
};

}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_URI_PARSER_H_
