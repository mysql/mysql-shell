/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/dump/dump_utils.h"

#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {

namespace {

constexpr auto k_sql_ext = ".sql";
constexpr auto k_separator = "@";

// Byte-values that are reserved and must be hex-encoded [0..255]
// clang-format off
static const int k_reserved_chars[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//                          SP !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1,
//  0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?  @  A  B  C  D  E  F  G
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
//  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
//  `  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//  x  y  z  {  |  }  ~
    0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
// clang-format on

std::string hexencode(const std::string &s) {
  std::string enc;
  size_t offs = 0;

  enc.resize(s.size() * 3 + 1);

  for (unsigned char c : s) {
    unsigned int i = static_cast<unsigned int>(c);
    // we're expecting UTF-8, only US-ASCII is encoded, remaining code points
    // are copied as is
    if (i < 128 && k_reserved_chars[i]) {
      snprintf(&enc[offs], 4, "%%%02X", i);
      offs += 3;
    } else {
      enc[offs] = c;
      ++offs;
    }
  }

  enc.resize(offs);

  return enc;
}

}  // namespace

std::string encode_schema_basename(const std::string &schema) {
  return hexencode(schema);
}

std::string encode_table_basename(const std::string &schema,
                                  const std::string &table) {
  return hexencode(schema) + k_separator + hexencode(table);
}

std::string get_schema_filename(const std::string &basename) {
  return basename + k_sql_ext;
}

std::string get_schema_filename(const std::string &basename,
                                const std::string &ext) {
  return basename + "." + ext;
}

std::string get_table_filename(const std::string &basename) {
  return basename + k_sql_ext;
}

std::string get_table_data_filename(const std::string &basename,
                                    const std::string &ext) {
  return basename + "." + ext;
}

std::string get_table_data_filename(const std::string &basename,
                                    const std::string &ext, size_t index,
                                    bool last_chunk) {
  return basename + k_separator + (last_chunk ? k_separator : "") +
         std::to_string(index) + "." + ext;
}

}  // namespace dump
}  // namespace mysqlsh
