/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include <mysql/gtid/tsid.h>

#include <tuple>

#include "mysqlshdk/libs/mysql/gtid_utils.h"

namespace mysqlsh {
namespace binlog {

void to_gtid_set(const std::string &str, mysql::gtid::Gtid_set *gtid_set) {
  mysqlshdk::mysql::Gtid_set::from_normalized_string(str).enumerate_ranges(
      [gtid_set](const mysqlshdk::mysql::Gtid_range &range) {
        mysql::gtid::Tsid tsid;
        std::ignore = tsid.from_cstring(range.uuid_tag.c_str());

        std::ignore = gtid_set->add(
            tsid, mysql::gtid::Gno_interval{
                      static_cast<mysql::gtid::gno_t>(range.begin),
                      static_cast<mysql::gtid::gno_t>(range.end)});
      });
}

}  // namespace binlog
}  // namespace mysqlsh
