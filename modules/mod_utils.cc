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

#include "modules/mod_utils.h"

#include <string>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/credential_manager.h"

namespace mysqlsh {

/**
 * Retrieves the connection data from a String
 *
 * @param String containing the connection data
 *
 * @return a Connection_options object with the connection data
 */
mysqlshdk::db::Connection_options get_connection_options(
    const std::string &instance_def) {
  if (instance_def.empty()) throw std::invalid_argument("Invalid URI: empty.");

  return shcore::get_connection_options(instance_def, false);
}

/**
 * Retrieves the connection data from a Dictionary
 *
 * @param dictionary containing the connection data
 *
 * @return a Connection_options object with the connection data
 */
mysqlshdk::db::Connection_options get_connection_options(
    const shcore::Dictionary_t &instance_def) {
  mysqlshdk::db::Connection_options ret_val;

  if (instance_def == nullptr || instance_def->size() == 0) {
    throw std::invalid_argument(
        "Invalid connection options, no options provided.");
  }

  shcore::Argument_map connection_map(*instance_def);

  std::set<std::string> mandatory;
  if (!connection_map.has_key(mysqlshdk::db::kSocket)) {
    mandatory.insert(mysqlshdk::db::kHost);
  }

  connection_map.ensure_keys(
      mandatory, mysqlshdk::db::connection_attributes, "connection options",
      ret_val.get_mode() == mysqlshdk::db::Comparison_mode::CASE_SENSITIVE);

  for (auto &option : *instance_def) {
    if (ret_val.compare(option.first, mysqlshdk::db::kPort) == 0) {
      ret_val.set_port(connection_map.int_at(option.first));
    } else if (ret_val.compare(option.first, mysqlshdk::db::kSocket) == 0) {
      const auto &sock = connection_map.string_at(option.first);
#ifdef _WIN32
      ret_val.set_pipe(sock);
#else   // !_WIN32
      ret_val.set_socket(sock);
#endif  // !_WIN32
    } else if (ret_val.compare(option.first, mysqlshdk::db::kConnectTimeout) ==
               0) {
      // Additional connection options are internally stored as strings.
      // Even so, when given in a dictionary, the connect-timeout option
      // must be given as an integer value
      if (connection_map.at(option.first).type != shcore::Integer) {
        mysqlshdk::db::Connection_options::throw_invalid_connect_timeout(
            connection_map.at(option.first).descr());
      } else {
        ret_val.set(option.first, {connection_map.at(option.first).descr()});
      }
    } else {
      ret_val.set(option.first, {connection_map.string_at(option.first)});
    }
  }
  return ret_val;
}

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
    const shcore::Argument_list &args, PasswordFormat format) {
  mysqlshdk::db::Connection_options ret_val;

  try {
    if (args.size() > 0 && args[0].type == shcore::String) {
      std::string uri = args.string_at(0);

      ret_val = get_connection_options(uri);
    } else if (args.size() > 0 && args[0].type == shcore::Map) {
      shcore::Value::Map_type_ref options = args.map_at(0);

      ret_val = get_connection_options(options);
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
  } catch (const std::invalid_argument &error) {
    throw shcore::Exception::argument_error(error.what());
  }

  return ret_val;
}

void SHCORE_PUBLIC set_password_from_map(
    Connection_options *options, const shcore::Value::Map_type_ref &map) {
  bool override_pwd = false;
  std::string key;
  for (auto option : *map) {
    if (!options->compare(option.first, mysqlshdk::db::kPassword) ||
        !options->compare(option.first, mysqlshdk::db::kDbPassword)) {
      // Will allow one override, a second means the password option was
      // duplicate on the second map, let the error raise
      if (!override_pwd) options->clear_password();

      options->set_password(option.second.get_string());
      key = option.first;
      override_pwd = true;
    }
  }

  // Removes the password from the map if found, this is because password is
  // case insensitive and the rest of the options are not
  if (override_pwd) map->erase(key);
}

void SHCORE_PUBLIC set_user_from_map(Connection_options *options,
                                     const shcore::Value::Map_type_ref &map) {
  bool override_user = false;
  std::string key;
  for (auto option : *map) {
    if (!options->compare(option.first, mysqlshdk::db::kUser) ||
        !options->compare(option.first, mysqlshdk::db::kDbUser)) {
      // Will allow one override, a second means the password option was
      // duplicate on the second map, let the error raise
      if (!override_user) options->clear_user();

      options->set_user(option.second.get_string());
      key = option.first;
      override_user = true;
    }
  }

  // Removes the password from the map if found, this is because password is
  // case insensitive and the rest of the options are not
  if (override_user) map->erase(key);
}

shcore::Value::Map_type_ref get_connection_map(
    const mysqlshdk::db::Connection_options &connection_options) {
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

  for (auto &option : connection_options.get_extra_options()) {
    if (option.second.is_null())
      (*map)[option.first] = shcore::Value();
    else {
      if (shcore::str_caseeq(option.first, mysqlshdk::db::kConnectTimeout)) {
        (*map)[option.first] =
            shcore::Value(std::atoi((*option.second).c_str()));
      } else {
        (*map)[option.first] = shcore::Value(*option.second);
      }
    }
  }

  return map;
}

namespace {

std::shared_ptr<mysqlshdk::db::mysqlx::Session> create_x_session() {
  auto xsession = mysqlshdk::db::mysqlx::Session::create();

  if (current_shell_options()->get().trace_protocol)
    xsession->enable_protocol_trace(true);

  return xsession;
}

std::shared_ptr<mysqlshdk::db::ISession> create_and_connect(
    const Connection_options &connection_options) {
  std::shared_ptr<mysqlshdk::db::ISession> session;
  std::string connection_error;

  Connection_options copy = connection_options;

  SessionType type = copy.get_session_type();

  // Automatic protocol detection is ON
  // Attempts X Protocol first, then Classic
  if (type == mysqlsh::SessionType::Auto) {
    session = create_x_session();

    try {
      copy.set_scheme("mysqlx");
      session->connect(copy);
      return session;
    } catch (const mysqlshdk::db::Error &e) {
      // Unknown message received from server indicates an attempt to create
      // And X Protocol session through the MySQL protocol
      int code = e.code();
      if (code == CR_MALFORMED_PACKET ||  // Unknown message received from
                                          // server 10
          code == CR_CONNECTION_ERROR ||  // No connection could be made because
                                          // the target machine actively refused
                                          // it connecting to host:port
          code == CR_SERVER_GONE_ERROR) {  // MySQL server has gone away
                                           // (randomly sent by libmysqlx)
        type = mysqlsh::SessionType::Classic;
        copy.clear_scheme();
        copy.set_scheme("mysql");

        // Since this is an unexpected error, we store the message to be logged
        // in case the classic session connection fails too
        if (code == CR_SERVER_GONE_ERROR)
          connection_error.append("X protocol error: ").append(e.what());
      } else {
        throw;
      }
    }
  }

  switch (type) {
    case mysqlsh::SessionType::X:
      session = create_x_session();
      break;
    case mysqlsh::SessionType::Classic:
      session = mysqlshdk::db::mysql::Session::create();
      break;
    default:
      throw shcore::Exception::argument_error(
          "Invalid session type specified for MySQL connection.");
      break;
  }

  try {
    session->connect(copy);
  } catch (shcore::Exception &e) {
    if (connection_error.empty()) {
      throw;
    } else {
      // If an error was cached for the X protocol connection
      // it is included on a new exception
      connection_error.append("\nClassic protocol error: ");
      connection_error.append(e.format());
      throw shcore::Exception::argument_error(connection_error);
    }
  }

  return session;
}

std::shared_ptr<mysqlshdk::db::ISession> create_session(
    const Connection_options &connection_options) {
  // allow SIGINT to interrupt the connect()
  bool cancelled = false;
  shcore::Interrupt_handler intr([&cancelled]() {
    cancelled = true;
    return true;
  });

  auto session = create_and_connect(connection_options);

  if (cancelled) throw shcore::cancelled("Cancelled");

  return session;
}

void password_prompt(Connection_options *options) {
  const std::string uri =
      options->as_uri(mysqlshdk::db::uri::formats::user_transport());
  const std::string prompt = "Please provide the password for '" + uri + "': ";
  std::string answer;
  const auto result = current_console()->prompt_password(prompt, &answer);

  if (result == shcore::Prompt_result::Ok) {
    options->set_password(answer);
  } else {
    throw shcore::cancelled("Cancelled");
  }
}

}  // namespace

std::shared_ptr<mysqlshdk::db::ISession> establish_session(
    const Connection_options &options, bool prompt_for_password,
    bool prompt_in_loop) {
  Connection_options copy = options;

  copy.set_default_connection_data();

  if (!copy.has_password()) {
    if (shcore::Credential_manager::get().get_password(&copy)) {
      try {
        return create_session(copy);
      } catch (const mysqlshdk::db::Error &e) {
        if (e.code() != ER_ACCESS_DENIED_ERROR) {
          throw;
        } else {
          copy.clear_password();
          shcore::Credential_manager::get().remove_password(copy);
          log_info(
              "Connection to \"%s\" could not be established using the stored "
              "password: %s. "
              "Invalid password has been erased.",
              copy.as_uri(mysqlshdk::db::uri::formats::user_transport())
                  .c_str(),
              e.format().c_str());
        }
      }
    }
  }

  if (prompt_for_password) {
    do {
      bool prompted_for_password = false;

      if (!copy.has_password()) {
        password_prompt(&copy);
        prompted_for_password = true;
      }

      try {
        auto session = create_session(copy);

        if (prompted_for_password) {
          // save password using the same connection options as the ones used
          // to fetch the password from the secret storage
          shcore::Credential_manager::get().save_password(copy);
        }

        return session;
      } catch (const mysqlshdk::db::Error &e) {
        if (!prompt_in_loop || e.code() != ER_ACCESS_DENIED_ERROR) {
          throw;
        } else {
          copy.clear_password();
          current_console()->print_error(e.format());
        }
      }
    } while (prompt_in_loop);
  }

  return create_session(copy);
}

std::shared_ptr<mysqlshdk::db::ISession> establish_mysql_session(
    const Connection_options &options, bool prompt_for_password,
    bool prompt_in_loop) {
  Connection_options copy = options;

  if (copy.has_scheme()) {
    copy.clear_scheme();
  }

  copy.set_scheme("mysql");

  return establish_session(copy, prompt_for_password, prompt_in_loop);
}

}  // namespace mysqlsh
