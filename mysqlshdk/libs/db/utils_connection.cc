/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <algorithm>
#include <cctype>

#include "mysqlshdk/libs/db/utils_connection.h"

namespace mysqlshdk {
namespace db {

int MapSslModeNameToValue::get_value(const std::string& value) {
  std::string my_value(value);
  std::transform(my_value.begin(), my_value.end(), my_value.begin(), ::tolower);

  auto index = std::find(ssl_modes.begin(), ssl_modes.end(), my_value);

  return index != ssl_modes.end() ? index - ssl_modes.begin() : 0;
}

const std::string& MapSslModeNameToValue::get_value(int value) {
  auto index = value >= 1 && value <= 5 ? value : 0;

  return ssl_modes[index];
}

}  // namespace db
}  // namespace mysqlshdk
