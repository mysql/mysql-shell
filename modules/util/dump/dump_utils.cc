/*
 * Copyright (c) 2020, 2021 Oracle and/or its affiliates.
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

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
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
  return encode_schema_basename(schema) + k_separator + hexencode(table);
}

std::string encode_partition_basename(const std::string &schema,
                                      const std::string &table,
                                      const std::string &partition) {
  return encode_table_basename(schema, table) + k_separator +
         hexencode(partition);
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

bool error_on_user_filters_conflicts(
    const std::vector<shcore::Account> &included_users,
    const std::vector<shcore::Account> &excluded_users) {
  const auto console = current_console();
  bool conflict = false;

  for (const auto &included : included_users) {
    for (const auto &excluded : excluded_users) {
      if (included.user == excluded.user) {
        if (included.host == excluded.host) {
          conflict = true;
          console->print_error(
              "Both includeUsers and excludeUsers options contain a user " +
              shcore::make_account(included) + ".");
          // exact match, need to check the remaining excluded users only if the
          // included host is not empty, as we may find the conflict handled
          // below
          if (included.host.empty()) {
            break;
          }
        } else if (excluded.host.empty()) {
          conflict = true;
          console->print_error(
              "The includeUsers option contains a user " +
              shcore::make_account(included) +
              " which is excluded by the value of the excludeUsers option: " +
              shcore::make_account(excluded) + ".");
          // fully specified included account has been excluded, but there can
          // be another conflict, if this account is also explicitly excluded,
          // check the remaining excluded users
        }
        // else, the included host is empty and excluded host is not, a valid
        // scenario; or, the hosts are different -> no conflict
      }
    }
  }

  return conflict;
}

}  // namespace dump
}  // namespace mysqlsh
