/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_MOD_UTILS_
#define MODULES_MOD_UTILS_

#include <set>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/lang_base.h"


namespace mysqlsh {

enum class PasswordFormat {
  NONE,
  STRING,
  OPTIONS,
};


shcore::Value::Map_type_ref SHCORE_PUBLIC get_connection_data(
  const shcore::Argument_list &args, PasswordFormat format);

void SHCORE_PUBLIC resolve_connection_credentials(const shcore::Value::Map_type_ref& options,
  shcore::Interpreter_delegate* delegate = nullptr);

}

#endif // MODULES_MOD_UTILS_