/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MYSQLX_CHARSET_H_
#define _MYSQLX_CHARSET_H_

#include <stdint.h>
#include <string>

namespace mysqlshdk {
namespace db {
namespace charset {

std::string charset_name_from_collation_id(uint32_t id);
std::string collation_name_from_collation_id(uint32_t id);
uint32_t collation_id_from_collation_name(const std::string& collation_name);

}  // namespace charset
}  // namespace db
}  // namespace mysqlshdk

#endif  // _MYSQLX_CHARSET_H_
