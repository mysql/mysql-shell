/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/util/binlog/utils.h"

#include <stdexcept>
#include <utility>

#include <mysql/gtids/legacy_glue.h>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace binlog {

namespace {

template <typename T>
inline T convert(std::string_view str, const char *context) {
  T ret;
  const auto result = mysql::strconv::decode_text(str, ret);

  if (!result.is_ok()) {
    throw std::runtime_error{shcore::str_format(
        "Failed to parse %s '%.*s': %s", context, static_cast<int>(str.size()),
        str.data(), mysql::strconv::throwing::encode_text(result).c_str())};
  }

  return ret;
}

}  // namespace

mysql::gtids::Gtid to_gtid(const mysql::binlog::event::Gtid_event &gtid_event) {
  const auto old_tsid = gtid_event.get_tsid();
  mysql::gtids::Tsid tsid;

  mysql::gtids::old_to_new(old_tsid, tsid);

  return mysql::gtids::Gtid::throwing_make(tsid, gtid_event.get_gno());
}

mysql::gtids::Gtid_set to_gtid_set(std::string_view str) {
  return convert<mysql::gtids::Gtid_set>(str, "GTID set");
}

mysql::gtids::Gtid to_gtid(std::string_view str) {
  return convert<mysql::gtids::Gtid>(str, "GTID");
}

std::string to_string(const mysql::gtids::Gtid_set &gtid_set) {
  return mysql::strconv::throwing::encode_text(gtid_set);
}

std::string to_string(const mysql::gtids::Gtid &gtid) {
  return mysql::strconv::throwing::encode_text(gtid);
}

std::string subtract(std::string_view l, std::string_view r) {
  return subtract(to_gtid_set(l), to_gtid_set(r));
}

std::string subtract(const mysql::gtids::Gtid_set &l,
                     const mysql::gtids::Gtid_set &r) {
  return mysql::strconv::throwing::encode_text(
      mysql::sets::make_subtraction_view(l, r));
}

void throw_on_failure(mysql::utils::Return_status status) {
  if (mysql::utils::Return_status::ok == status) {
    return;
  }

  throw std::bad_alloc{};
}

}  // namespace binlog
}  // namespace mysqlsh
