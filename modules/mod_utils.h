/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates.
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
#include <vector>

#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/document_parser.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

using mysqlshdk::db::Connection_options;

Connection_options SHCORE_PUBLIC get_connection_options(const shcore::Value &v);

/*
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

  shcore::Option_unpacker &optional_obj(const char *name,
                                        Connection_options *out_value) {
    shcore::Value value = get_optional(name, shcore::Undefined);
    if (value) {
      if (value.type == shcore::String) {
        try {
          *out_value = Connection_options(value.get_string());
        } catch (const std::exception &e) {
          throw shcore::Exception::argument_error(
              std::string("Invalid value for option ") + name + ": " +
              e.what());
        }
        // } else if (value.type == shcore::Map) {
        //   try {
        //     *out_value = get_connection_options();
        //   } catch (const std::exception& e) {
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

void SHCORE_PUBLIC set_password_from_map(Connection_options *options,
                                         const shcore::Dictionary_t &map);
void SHCORE_PUBLIC set_password_from_string(Connection_options *options,
                                            const char *password);
void SHCORE_PUBLIC set_user_from_map(Connection_options *options,
                                     const shcore::Value::Map_type_ref &map);

shcore::Value::Map_type_ref SHCORE_PUBLIC
get_connection_map(const Connection_options &connection_options);

/**
 * Establishes a new session using given connection options.
 *
 * Fills in connection options, if they're missing:
 * - calls shcore::set_default_connection_data(),
 * - if password is not specified, queries the credential helper for one (if
 *   applicable) and tries to establish a session; if session cannot be
 *   established due to wrong credentials, queries the user for password, unless
 *   prompt_for_password is set to false; in that case attempts to connect
 *   without password,
 * - uses scheme of the established session if no scheme is specified.
 *
 * If session was successfully established and password was not previously
 * stored by the credential helper, stores the password (if applicable).
 *
 * @param options Connection options used to establish a session.
 * @param prompt_for_password If true and password is missing will prompt the
 *        user for password.
 * @param prompt_in_loop If true, prompt is presented in a loop until correct
 *        password is given or operation is canceled via CTRL-C.
 *
 * @return A session object connected to the server specified by connection
 *         options.
 *
 * @throws shcore::cancelled if user interrupts the operation with CTRL-C
 * @throws shcore::Exception::argument_error if scheme was not specified and
 *         connection cannot be established using both X and MySQL protocol
 * @throws mysqlshdk::db::Error if session could not be established
 */
std::shared_ptr<mysqlshdk::db::ISession> SHCORE_PUBLIC
establish_session(const Connection_options &options, bool prompt_for_password,
                  bool prompt_in_loop = false);

/**
 * Forces the session to use MySQL protocol changing the scheme to "mysql",
 * otherwise behaves the same as establish_session().
 *
 * @param options Connection options used to establish a session.
 * @param prompt_for_password If true and password is missing will prompt the
 *        user for password.
 * @param prompt_in_loop If true, prompt is presented in a loop until correct
 *        password is given or operation is canceled via CTRL-C.
 *
 * @return A session object connected to the server specified by connection
 *         options.
 */
std::shared_ptr<mysqlshdk::db::ISession> SHCORE_PUBLIC
establish_mysql_session(const Connection_options &options,
                        bool prompt_for_password, bool prompt_in_loop = false);

/**
 * Provides connection options for a classic session. If session parameter is a
 * classic session, simply returns its connection options, if it's not, queries
 * the server for relevant parameters, maintaining the transport type.
 *
 * @param session An open session.
 *
 * @returns Connection options which can be used to establish a classic session.
 *
 * @throws std::logic_error If the given session uses the X protocol and classic
 *         port/socket cannot be determined.
 */
Connection_options SHCORE_PUBLIC get_classic_connection_options(
    const std::shared_ptr<mysqlshdk::db::ISession> &session);

void unpack_json_import_flags(shcore::Option_unpacker *unpacker,
                              shcore::Document_reader_options *options);

/**
 * Converts SQL values from a row into shcore::Values.
 *
 * @param row Row to be converted.
 *
 * @return Vector with converted values.
 */
std::vector<shcore::Value> get_row_values(const mysqlshdk::db::IRow &row);

/**
 * Returns formatted text for current active exception.
 *
 * Ex:
 * try {
 *    do_something();
 * } catch (...) {
 *    cerr << "ERROR: " << format_active_exception() << "\n";
 * }
 */
inline std::string format_active_exception() {
  try {
    throw;
  } catch (const shcore::Exception &e) {
    return e.format();
  } catch (const mysqlshdk::db::Error &e) {
    return e.format();
  } catch (const std::exception &e) {
    return e.what();
  }
  return "";
}

}  // namespace mysqlsh

#endif  // MODULES_MOD_UTILS_H_
