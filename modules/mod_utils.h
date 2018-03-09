/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>
#include <set>
#include <string>

#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
using mysqlshdk::db::Connection_options;

enum class PasswordFormat {
  NONE,
  STRING,
  OPTIONS,
};

Connection_options SHCORE_PUBLIC
get_connection_options(const shcore::Value& args);
Connection_options SHCORE_PUBLIC get_connection_options(
    const shcore::Argument_list& args, PasswordFormat format);

/**
 * Unpack an options dictionary.
 *
 * @param  options Dictionary containing options to extract from (null OK)
 *
 * @example
 *
 * bool opt_prompts = false;
 * std::string opt_name;
 * Connection_options opt_target;
 *
 * if (args.size() > 1)
 *  Unpack_options(args[1]).
 *      optional("prompts", &opt_prompts).
 *      optional("name", &opt_name).
 *      required("target", &opt_target).
 *      end();
 */
class Unpack_options : public shcore::Option_unpacker {
 public:
  explicit Unpack_options(shcore::Dictionary_t options)
      : shcore::Option_unpacker(options) {}

  shcore::Option_unpacker& optional_obj(const char* name,
                                        Connection_options* out_value) {
    shcore::Value value = get_optional(name, shcore::Undefined);
    if (value) {
      if (value.type == shcore::String) {
        try {
          *out_value = Connection_options(value.as_string());
        } catch (std::exception& e) {
          throw shcore::Exception::argument_error(
              std::string("Invalid value for option ") + name + ": " +
              e.what());
        }
        // } else if (value.type == shcore::Map) {
        //   try {
        //     *out_value = get_connection_options();
        //   } catch (std::exception& e) {
        //     throw shcore::Exception::argument_error(
        //         std::string("Invalid value for option ") + name + ": " +
        //         e.what());
        //   }
      } else {
        throw shcore::Exception::type_error(shcore::str_format(
            "Option '%s' is expected to be of type String, but is %s", name,
            type_name(value.type).c_str()));
      }
    }
    return *this;
  }
};

void SHCORE_PUBLIC set_password_from_map(
    Connection_options* options, const shcore::Value::Map_type_ref& map);
void SHCORE_PUBLIC set_user_from_map(Connection_options* options,
                                     const shcore::Value::Map_type_ref& map);

shcore::Value::Map_type_ref SHCORE_PUBLIC
get_connection_map(const Connection_options& connection_options);

void SHCORE_PUBLIC resolve_connection_credentials(
    Connection_options* options,
    std::shared_ptr<mysqlsh::IConsole> console_handler = nullptr);

// To be removed and replaced with resolve_connection_credentials
// as soon as the full move to IConsole is done
void SHCORE_PUBLIC resolve_connection_credentials_deleg(
    Connection_options* options,
    shcore::Interpreter_delegate* delegate = nullptr);

}  // namespace mysqlsh

#endif  // MODULES_MOD_UTILS_H_
