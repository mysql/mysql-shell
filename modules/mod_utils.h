/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_MOD_UTILS_H_
#define MODULES_MOD_UTILS_H_

#include <set>

#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/connection_options.h"
using mysqlshdk::db::Connection_options;
namespace mysqlsh {

enum class PasswordFormat {
  NONE,
  STRING,
  OPTIONS,
};

Connection_options SHCORE_PUBLIC get_connection_options(
    const shcore::Argument_list& args, PasswordFormat format);

void SHCORE_PUBLIC set_password_from_map(
    Connection_options* options, const shcore::Value::Map_type_ref& map);
void SHCORE_PUBLIC set_user_from_map(Connection_options* options,
                                     const shcore::Value::Map_type_ref& map);

shcore::Value::Map_type_ref SHCORE_PUBLIC
get_connection_map(const Connection_options& connection_options);

void SHCORE_PUBLIC resolve_connection_credentials(
    Connection_options* options,
    shcore::Interpreter_delegate* delegate = nullptr);

}  // namespace mysqlsh

#endif  // MODULES_MOD_UTILS_H_
