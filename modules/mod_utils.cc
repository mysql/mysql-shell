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

#include "mod_utils.h"
#include "shellcore/types.h"
#include "utils/utils_general.h"
#include "utils/utils_connection.h"

namespace mysqlsh {
/**
 * This function will retrieve the connection data from the received arguments
 * Connection data can be specified in one of:
 * - Dictionary
 * - URI
 *
 * A common pattern is functions with a signature as follows:
 *
 * function name(connection[, password])
 * function name(connection[, options])
 *
 * On both functions connection may be either a dictionary or a string (URI)
 * An optional password parameter may be defined, this will override the
 * password defined in connection.
 *
 * An optional options parameter may be defined, it may contain a password
 * entry that wold override the one defined in connection.
 *
 * Conflicting options will also be validated.
 */
shcore::Value::Map_type_ref get_connection_data(
  const shcore::Argument_list &args, PasswordFormat format) {

  shcore::Value::Map_type_ref options;

  // Verifies the case where an URI is specified
  if (args[0].type == shcore::String) {
    try {
      options = shcore::get_connection_data(args.string_at(0), false);
    }
    catch (std::exception &e) {
      std::string error(e.what());
      throw shcore::Exception::argument_error("Invalid connection data, "
      "expected a URI. Error: " + error);
    }
  }

  // Not URI? then Dictionary
  else if (args[0].type == shcore::Map)
    options = args.map_at(0);
  else
    throw shcore::Exception::argument_error("Invalid connection data, expected "
    "either a URI or a Dictionary");

  if (options->size() == 0)
    throw shcore::Exception::argument_error("Connection definition is empty");

  // If password override is allowed then we review the second
  // argument if any
  if (format != PasswordFormat::NONE && args.size() > 1) {
    bool override_pwd = false;
    std::string new_password;

    if (format == PasswordFormat::OPTIONS) {
      auto other_options = args.map_at(1);
      shcore::Argument_map other_args(*other_options);

      if (other_args.has_key(shcore::kPassword)) {
        new_password = other_args.string_at(shcore::kPassword);
        override_pwd = true;
      } else if (other_args.has_key(shcore::kDbPassword)) {
        new_password = other_args.string_at(shcore::kDbPassword);
        override_pwd = true;
      }
    } else if (format == PasswordFormat::STRING) {
      new_password = args.string_at(1);
      override_pwd = true;
    }

    if (override_pwd) {
      if (options->has_key(shcore::kDbPassword))
        (*options)[shcore::kDbPassword] = shcore::Value(new_password);
      else
        (*options)[shcore::kPassword] = shcore::Value(new_password);
    }
  }

  // Finally validates the defined
  shcore::Argument_map conn_options(*options);

  // Ensures the options are in the allowed set
  conn_options.ensure_keys({}, shcore::connection_attributes,
                           "connection data");

  // Ensures no duplicates are specified
  if (options->has_key(shcore::kPassword) &&
      options->has_key(shcore::kDbPassword))
    throw shcore::Exception::argument_error("Conflicting connection options: "
    "password and dbPassword defined, use either one or the other.");

  if (options->has_key(shcore::kUser) && options->has_key(shcore::kDbUser))
    throw shcore::Exception::argument_error("Conflicting connection options: "
    "user and dbUser defined, use either one or the other.");

  if (options->has_key(shcore::kPort) && options->has_key(shcore::kSocket))
    throw shcore::Exception::argument_error("Conflicting connection options: "
    "port and socket defined, use either one or the other.");

  return options;
}

/**
 * This function receives an connection options map and will try to resolve user
 * and password:
 * - If no user is specified, "root" will be used.
 * - Password is resolved based on a delegate, if no password is available and
 *   a delegate is provided, the password will be retrieved through the delegate
 */
void resolve_connection_credentials(const shcore::Value::Map_type_ref& options,
				    shcore::Interpreter_delegate* delegate) {
  // Sets a default user if not specified
  if (!options->has_key("user") && !options->has_key("dbUser"))
    (*options)["dbUser"] = shcore::Value("root");

  std::string uri = build_connection_string(options, false);
  if (!options->has_key("password") && !options->has_key("dbPassword")) {
    if (delegate) {
      std::string answer;

      std::string prompt = "Please provide the password for '" + uri + "': ";

      if (delegate->password(delegate->user_data, prompt.c_str(), answer)) {
        (*options)["dbPassword"] = shcore::Value(answer);
      } else {
        throw shcore::Exception::argument_error("Missing password for '" +
          uri + "'");
      }
    } else {
      throw shcore::Exception::argument_error("Missing password for '" +
        uri + "'");
    }
  }
}


}