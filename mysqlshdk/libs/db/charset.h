/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

#ifndef _MYSQLX_CHARSET_H_
#define _MYSQLX_CHARSET_H_

#include <stdint.h>
#include <string>
#include <string_view>
#include <unordered_set>

namespace mysqlshdk {
namespace db {
namespace charset {

constexpr const int k_binary_collation_id = 63;

std::string charset_name_from_collation_id(uint32_t id);
std::string collation_name_from_collation_id(uint32_t id);
uint32_t collation_id_from_collation_name(std::string_view collation_name);
std::unordered_set<std::string_view> charset_names();

}  // namespace charset
}  // namespace db
}  // namespace mysqlshdk

#endif  // _MYSQLX_CHARSET_H_
