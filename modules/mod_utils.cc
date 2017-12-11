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

#include "modules/mod_utils.h"
#include <string>
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

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

mysqlshdk::db::Connection_options get_connection_options(
    const shcore::Argument_list& args, PasswordFormat format) {
  mysqlshdk::db::Connection_options ret_val;

  try {
    if (args.size() > 0 && args[0].type == shcore::String) {
      std::string uri = args.string_at(0);
      if (uri.empty())
        throw std::invalid_argument("Invalid URI: empty.");

      ret_val = shcore::get_connection_options(args.string_at(0), false);
    } else if (args.size() > 0 && args[0].type == shcore::Map) {
      shcore::Value::Map_type_ref options = args.map_at(0);

      if (options->size() == 0)
        throw std::invalid_argument(
            "Invalid connection options, "
            "no options provided.");

      shcore::Argument_map connection_map(*options);
      connection_map.ensure_keys(
          {mysqlshdk::db::kHost}, mysqlshdk::db::connection_attributes,
          "connection options",
          ret_val.get_mode() == mysqlshdk::db::Comparison_mode::CASE_SENSITIVE);

      for (auto& option : *options) {
        if (ret_val.compare(option.first, mysqlshdk::db::kPort) == 0)
          ret_val.set_port(connection_map.int_at(option.first));
        else
          ret_val.set(option.first, {connection_map.string_at(option.first)});
      }
    } else {
      throw std::invalid_argument(
          "Invalid connection options, expected either "
          "a URI or a Dictionary.");
    }

    // Some functions override the password in a second parameter
    // which could be either a string or an options map, such behavior is
    // handled here
    if (format != PasswordFormat::NONE && args.size() > 1) {
      if (format == PasswordFormat::OPTIONS) {
        set_password_from_map(&ret_val, args.map_at(1));
      } else if (format == PasswordFormat::STRING) {
        ret_val.clear_password();
        ret_val.set_password(args.string_at(1));
      }
    }
  } catch (const std::invalid_argument& error) {
    throw shcore::Exception::argument_error(error.what());
  }

  return ret_val;
}

void SHCORE_PUBLIC set_password_from_map(
    Connection_options* options, const shcore::Value::Map_type_ref& map) {
  bool override_pwd = false;
  std::string key;
  for (auto option : *map) {
    if (!options->compare(option.first, mysqlshdk::db::kPassword) ||
        !options->compare(option.first, mysqlshdk::db::kDbPassword)) {
      // Will allow one override, a second means the password option was
      // duplicate on the second map, let the error raise
      if (!override_pwd)
        options->clear_password();

      options->set_password(option.second.as_string());
      key = option.first;
      override_pwd = true;
    }
  }

  // Removes the password from the map if found, this is because password is
  // case insensitive and the rest of the options are not
  if (override_pwd)
    map->erase(key);
}

void SHCORE_PUBLIC set_user_from_map(Connection_options* options,
                                     const shcore::Value::Map_type_ref& map) {
  bool override_user = false;
  std::string key;
  for (auto option : *map) {
    if (!options->compare(option.first, mysqlshdk::db::kUser) ||
        !options->compare(option.first, mysqlshdk::db::kDbUser)) {
      // Will allow one override, a second means the password option was
      // duplicate on the second map, let the error raise
      if (!override_user)
        options->clear_user();

      options->set_user(option.second.as_string());
      key = option.first;
      override_user = true;
    }
  }

  // Removes the password from the map if found, this is because password is
  // case insensitive and the rest of the options are not
  if (override_user)
    map->erase(key);
}

shcore::Value::Map_type_ref get_connection_map(
    const mysqlshdk::db::Connection_options& connection_options) {
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());

  if (connection_options.has_scheme())
    (*map)[mysqlshdk::db::kScheme] =
        shcore::Value(connection_options.get_scheme());

  if (connection_options.has_user())
    (*map)[mysqlshdk::db::kUser] = shcore::Value(connection_options.get_user());

  if (connection_options.has_password())
    (*map)[mysqlshdk::db::kPassword] =
        shcore::Value(connection_options.get_password());

  if (connection_options.has_host())
    (*map)[mysqlshdk::db::kHost] = shcore::Value(connection_options.get_host());

  if (connection_options.has_port())
    (*map)[mysqlshdk::db::kPort] = shcore::Value(connection_options.get_port());

  if (connection_options.has_schema())
    (*map)[mysqlshdk::db::kSchema] =
        shcore::Value(connection_options.get_schema());

  if (connection_options.has_socket())
    (*map)[mysqlshdk::db::kSocket] =
        shcore::Value(connection_options.get_socket());

  // Yes: on socket, is not a typo
  if (connection_options.has_pipe())
    (*map)[mysqlshdk::db::kSocket] =
        shcore::Value(connection_options.get_pipe());

  auto ssl = connection_options.get_ssl_options();
  if (ssl.has_data()) {
    if (ssl.has_ca())
      (*map)[mysqlshdk::db::kSslCa] = shcore::Value(ssl.get_ca());

    if (ssl.has_capath())
      (*map)[mysqlshdk::db::kSslCaPath] = shcore::Value(ssl.get_capath());

    if (ssl.has_cert())
      (*map)[mysqlshdk::db::kSslCert] = shcore::Value(ssl.get_cert());

    if (ssl.has_cipher())
      (*map)[mysqlshdk::db::kSslCipher] = shcore::Value(ssl.get_cipher());

    if (ssl.has_crl())
      (*map)[mysqlshdk::db::kSslCrl] = shcore::Value(ssl.get_crl());

    if (ssl.has_crlpath())
      (*map)[mysqlshdk::db::kSslCrlPath] = shcore::Value(ssl.get_crlpath());

    if (ssl.has_key())
      (*map)[mysqlshdk::db::kSslKey] = shcore::Value(ssl.get_key());

    if (ssl.has_mode())
      (*map)[mysqlshdk::db::kSslMode] =
          shcore::Value(ssl.get_value(mysqlshdk::db::kSslMode));

    if (ssl.has_tls_version())
      (*map)[mysqlshdk::db::kSslTlsVersion] =
          shcore::Value(ssl.get_tls_version());
  }

  for (auto& option : connection_options.get_extra_options()) {
    if (option.second.is_null())
      (*map)[option.first] = shcore::Value();
    else
      (*map)[option.first] = shcore::Value(*option.second);
  }

  return map;
}

/*shcore::Value::Map_type_ref get_connection_map
  (const std::string &uri) {
    auto c_opts = shcore::get_connection_options(uri, false);

    shcore::Value::Map_type_ref options(new shcore::Value::Map_type());

    if (c_opts.has_scheme())
      (*options)[mysqlshdk::db::kScheme] = shcore::Value(c_opts.get_scheme());
    if (c_opts.has_user())
      (*options)[mysqlshdk::db::kUser] = shcore::Value(c_opts.get_user());
    if (c_opts.has_password())
      (*options)[mysqlshdk::db::kPassword] =
shcore::Value(c_opts.get_password()); if (c_opts.has_host())
      (*options)[mysqlshdk::db::kHost] = shcore::Value(c_opts.get_host());
    if (c_opts.has_port())
      (*options)[mysqlshdk::db::kPort] = shcore::Value(c_opts.get_port());
    if (c_opts.has_pipe())
      (*options)[mysqlshdk::db::kSocket] = shcore::Value(c_opts.get_pipe());
    if (c_opts.has_socket())
      (*options)[mysqlshdk::db::kSocket] = shcore::Value(c_opts.get_socket());
    if (c_opts.has_schema())
      (*options)[mysqlshdk::db::kSchema] = shcore::Value(c_opts.get_schema());
    if (c_opts.has(mysqlshdk::db::kAuthMethod))
      (*options)[mysqlshdk::db::kAuthMethod] =
shcore::Value(c_opts.get(mysqlshdk::db::kAuthMethod));


    auto ssl = c_opts.get_ssl_options();
    if (ssl.has_data()) {
      if (ssl.has_mode())
        (*options)[mysqlshdk::db::kSslMode] =
shcore::Value(ssl.get_value(mysqlshdk::db::kSslMode)); if (ssl.has_ca())
        (*options)[mysqlshdk::db::kSslCa] = shcore::Value(ssl.get_ca());
      if (ssl.has_capath())
        (*options)[mysqlshdk::db::kSslCaPath] = shcore::Value(ssl.get_capath());
      if (ssl.has_cert())
        (*options)[mysqlshdk::db::kSslCert] = shcore::Value(ssl.get_cert());
      if (ssl.has_key())
        (*options)[mysqlshdk::db::kSslKey] = shcore::Value(ssl.get_key());
      if (ssl.has_cipher())
        (*options)[mysqlshdk::db::kSslCipher] =
shcore::Value(ssl.get_cipher()); if (ssl.has_crl())
        (*options)[mysqlshdk::db::kSslCrl] = shcore::Value(ssl.get_crl());
      if (ssl.has_crlpath())
        (*options)[mysqlshdk::db::kSslCrlPath] =
shcore::Value(ssl.get_crlpath()); if (ssl.has_tls_version())
        (*options)[mysqlshdk::db::kSslTlsVersion] =
shcore::Value(ssl.get_tls_version());
    }

    return options;
}*/

/**
 * This function receives an connection options map and will try to resolve user
 * and password:
 * - If no user is specified, "root" will be used.
 * - Password is resolved based on a delegate, if no password is available and
 *   a delegate is provided, the password will be retrieved through the delegate
 */
void resolve_connection_credentials(mysqlshdk::db::Connection_options* options,
                                    shcore::Interpreter_delegate* delegate) {
  // Sets a default user if not specified
  // TODO(rennox): Why is it setting root when it should actually be the
  // system user???
  // Maybe not for the interactives??
  // Maybe the default user should be passed as parameters and use system if
  // empty oO
  if (!options->has_user())
    options->set_user("root");

  if (!options->has_password()) {
    std::string uri =
        options->as_uri(mysqlshdk::db::uri::formats::full_no_password());
    if (delegate) {
      std::string answer;

      std::string prompt = "Please provide the password for '" + uri + "': ";

      if (delegate->password(delegate->user_data, prompt.c_str(), &answer) ==
          shcore::Prompt_result::Ok) {
        options->set_password(answer);
      } else {
        throw shcore::Exception::argument_error("Missing password for '" + uri +
                                                "'");
      }
    } else {
      throw shcore::Exception::argument_error("Missing password for '" + uri +
                                              "'");
    }
  }
}

}  // namespace mysqlsh
