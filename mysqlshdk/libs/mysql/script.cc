/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/mysql/script.h"

#include <sstream>

#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"

namespace mysqlshdk {
namespace mysql {

size_t execute_sql_script(
    const mysqlshdk::mysql::IInstance &instance, const std::string &script,
    const std::function<void(std::string_view)> &err_callback) {
  std::stringstream stream(script);
  size_t count = 0;
  utils::iterate_sql_stream(
      &stream, 1024 * 64,
      [&instance, &count](std::string_view s, std::string_view, size_t,
                          size_t) {
        instance.query(std::string{s});
        ++count;
        return true;
      },
      err_callback);
  return count;
}

}  // namespace mysql
}  // namespace mysqlshdk
