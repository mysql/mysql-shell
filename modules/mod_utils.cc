/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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
#include <utility>

#include "mysqlshdk/include/scripting/obj_date.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/utils/fault_injection.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/credential_manager.h"

namespace mysqlsh {

namespace {

/**
 * Retrieves the connection data from a String
 *
 * @param String containing the connection data
 *
 * @return a Connection_options object with the connection data
 */
Connection_options get_connection_options(const std::string &instance_def) {
  if (instance_def.empty()) throw std::invalid_argument("Invalid URI: empty.");

  auto ret_val = shcore::get_connection_options(instance_def, false);
  for (const auto &warning : ret_val.get_warnings())
    mysqlsh::current_console()->print_warning(warning);
  ret_val.clear_warnings();

  return ret_val;
}

/**
 * Retrieves the connection data from a Dictionary
 *
 * @param dictionary containing the connection data
 *
 * @return a Connection_options object with the connection data
 */
Connection_options get_connection_options(
    const shcore::Dictionary_t &instance_def) {
  if (instance_def == nullptr || instance_def->size() == 0) {
    throw std::invalid_argument(
        "Invalid connection options, no options provided.");
  }

  shcore::Argument_map connection_map(*instance_def);

  std::set<std::string> mandatory;
  // host defaults to localhost, so it's not mandatory
  // if (!connection_map.has_key(mysqlshdk::db::kSocket)) {
  //   mandatory.insert(mysqlshdk::db::kHost);
  // }

  Connection_options ret_val;

  connection_map.ensure_keys(
      mandatory, mysqlshdk::db::connection_attributes, "connection options",
      ret_val.get_mode() == mysqlshdk::db::Comparison_mode::CASE_SENSITIVE);

  for (auto &option : *instance_def) {
    if (ret_val.compare(option.first, mysqlshdk::db::kHost) == 0) {
      const auto &host = connection_map.string_at(option.first);

      if (host.empty()) {
        throw std::invalid_argument("Host value cannot be an empty string.");
      }

      ret_val.set(option.first, host);
    } else if (ret_val.compare(option.first, mysqlshdk::db::kPort) == 0) {
      ret_val.set_port(connection_map.int_at(option.first));
    } else if (ret_val.compare(option.first, mysqlshdk::db::kSocket) == 0) {
      const auto &sock = connection_map.string_at(option.first);
#ifdef _WIN32
      ret_val.set_pipe(sock);
#else   // !_WIN32
      ret_val.set_socket(sock);
#endif  // !_WIN32
    } else if (ret_val.compare(option.first,
                               mysqlshdk::db::kCompressionLevel) == 0) {
      ret_val.set_compression_level(connection_map.int_at(option.first));
    } else if (ret_val.compare(option.first, mysqlshdk::db::kConnectTimeout) ==
               0) {
      // Additional connection options are internally stored as strings.
      // Even so, when given in a dictionary, the connect-timeout option
      // must be given as an integer value
      if (connection_map.at(option.first).type != shcore::Integer) {
        mysqlshdk::db::Connection_options::throw_invalid_connect_timeout(
            connection_map.at(option.first).descr());
      } else {
        ret_val.set(option.first, connection_map.at(option.first).descr());
      }
    } else if (ret_val.compare(option.first,
                               mysqlshdk::db::kConnectionAttributes) == 0) {
      if (option.second.type == shcore::Map) {
        // Supports connection-attributes: {key:val, key:val, ...}
        auto map = option.second.as_map();
        for (const auto &entry : *map) {
          ret_val.set_connection_attribute(entry.first, entry.second.descr());
        }
      } else if (option.second.type == shcore::Array) {
        // Supports connection-attributes: ["key=val", "key=val", ...]
        auto array = option.second.as_array();
        std::vector<std::string> values;
        for (const auto &entry : *array) {
          values.push_back(entry.descr());
        }
        ret_val.set_connection_attributes(values);
      } else {
        // Supports connection-attributes=false|true|1|0
        ret_val.set(mysqlshdk::db::kConnectionAttributes,
                    option.second.descr());
      }
    } else {
      ret_val.set(option.first, connection_map.string_at(option.first));
    }
  }

  for (const auto &warning : ret_val.get_warnings())
    mysqlsh::current_console()->print_warning(warning);
  ret_val.clear_warnings();

  return ret_val;
}

}  // namespace

Connection_options get_connection_options(const shcore::Value &v) {
  if (shcore::String == v.type) {
    return get_connection_options(v.get_string());
  } else if (shcore::Map == v.type) {
    return get_connection_options(v.as_map());
  } else {
    throw shcore::Exception::type_error(
        "Invalid connection options, expected either a URI or a Connection "
        "Options Dictionary");
  }
}

void SHCORE_PUBLIC set_password_from_map(Connection_options *options,
                                         const shcore::Dictionary_t &map) {
  if (!options || !map) {
    return;
  }

  bool override_pwd = false;
  std::string key;

  for (const auto &option : *map) {
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

void set_password_from_string(Connection_options *options,
                              const char *password) {
  if (options && password) {
    options->clear_password();
    options->set_password(password);
  }
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

    if (ssl.has_tls_ciphersuites())
      (*map)[mysqlshdk::db::kSslTlsCiphersuites] =
          shcore::Value(ssl.get_tls_ciphersuites());
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

  if (connection_options.is_connection_attributes_enabled()) {
    auto conn_atts = connection_options.get_connection_attributes();

    if (conn_atts.size()) {
      auto attributes = shcore::make_dict();
      for (auto &option : conn_atts) {
        (*attributes)[option.first] = shcore::Value(*option.second);
      }
      (*map)[mysqlshdk::db::kConnectionAttributes] = shcore::Value(attributes);
    }
  } else {
    (*map)[mysqlshdk::db::kConnectionAttributes] = shcore::Value::False();
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
          code == CR_SERVER_GONE_ERROR ||  // MySQL server has gone away
                                           // (randomly sent by libmysqlx)
          (CR_X_UNSUPPORTED_OPTION_VALUE == code &&       // auth-method was not
           strstr(e.what(), "authentication method"))) {  // valid for X proto
        type = mysqlsh::SessionType::Classic;
        copy.clear_scheme();
        copy.set_scheme("mysql");

        // Since this is an unexpected error, we store the message to be
        // logged in case the classic session connection fails too
        if (code == CR_SERVER_GONE_ERROR ||
            CR_X_UNSUPPORTED_OPTION_VALUE == code) {
          connection_error.append("X protocol error: ").append(e.what());
        }
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
  } catch (const mysqlshdk::db::Error &e) {
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
    bool prompt_in_loop, bool show_tls_deprecation) {
  FI_SUPPRESS(mysql);
  FI_SUPPRESS(mysqlx);

  try {
    Connection_options copy = options;

    copy.set_default_connection_data();
    copy.show_tls_deprecation_warning(show_tls_deprecation);

    if (!copy.has_password() && copy.has_user()) {
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
                "Connection to \"%s\" could not be established using the "
                "stored password: %s. Invalid password has been erased.",
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

        if (!copy.has_password() && copy.has_user() &&
            !copy.is_auth_method(mysqlshdk::db::kAuthMethodKerberos) &&
            !copy.is_auth_method(mysqlshdk::db::kAuthMethodOci)) {
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
  } catch (const mysqlshdk::db::Error &e) {
    throw shcore::Exception::mysql_error_with_code_and_state(e.what(), e.code(),
                                                             e.sqlstate());
  }
}

std::shared_ptr<mysqlshdk::db::ISession> establish_mysql_session(
    const Connection_options &options, bool prompt_for_password,
    bool prompt_in_loop, bool show_tls_deprecation) {
  Connection_options copy = options;

  if (copy.has_scheme()) {
    copy.clear_scheme();
  }

  copy.set_scheme("mysql");

  return establish_session(copy, prompt_for_password, prompt_in_loop,
                           show_tls_deprecation);
}

Connection_options get_classic_connection_options(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  // copy the connection options
  auto co = session->get_connection_options();

  // switch from X protocol to classic
  if (SessionType::Classic != co.get_session_type()) {
    co.clear_scheme();
    co.set_scheme("mysql");

    if (co.has_port()) {
      const auto result = session->query("SELECT @@GLOBAL.port");
      const auto row = result->fetch_one();

      if (!row) {
        throw std::logic_error(
            "Unable to determine classic MySQL port from the given shell "
            "connection");
      }

      co.clear_port();
      co.set_port(row->get_int(0));
    } else {
      // if we're here then socket was used
      const auto result =
          session->query("SELECT @@GLOBAL.socket, @@GLOBAL.datadir");
      const auto row = result->fetch_one();

      if (!row) {
        throw std::logic_error(
            "Unable to determine classic MySQL socket from the given shell "
            "connection");
      }

      const auto socket = row->get_string(0);

      co.clear_socket();
      co.set_socket(shcore::path::is_absolute(socket)
                        ? socket
                        : shcore::path::join_path(row->get_string(1), socket));
    }
  }

  return co;
}

std::vector<shcore::Value> get_row_values(const mysqlshdk::db::IRow &row) {
  using mysqlshdk::db::Type;
  using shcore::Date;
  using shcore::Value;
  std::vector<Value> value_array;

  for (uint32_t i = 0, c = row.num_fields(); i < c; i++) {
    Value v;

    if (row.is_null(i)) {
      v = Value::Null();
    } else {
      switch (row.get_type(i)) {
        case Type::Null:
          v = Value::Null();
          break;

        case Type::String:
          v = Value(row.get_string(i));
          break;

        case Type::Integer:
          v = Value(row.get_int(i));
          break;

        case Type::UInteger:
          v = Value(row.get_uint(i));
          break;

        case Type::Float:
          v = Value(row.get_float(i));
          break;

        case Type::Double:
          v = Value(row.get_double(i));
          break;

        case Type::Decimal:
          v = Value(row.get_as_string(i));
          break;

        case Type::Date:
        case Type::DateTime:
          v = Value(Date::unrepr(row.get_string(i)));
          break;

        case Type::Bit:
          v = Value(row.get_bit(i));
          break;

        case Type::Bytes:
        case Type::Geometry:
        case Type::Json:
        case Type::Time:
        case Type::Enum:
        case Type::Set:
          v = Value(row.get_string(i));
          break;
      }
    }

    value_array.emplace_back(std::move(v));
  }

  return value_array;
}

}  // namespace mysqlsh

// We need to hide these from doxygen to avoid warnings
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
namespace shcore {
namespace detail {

mysqlshdk::db::Connection_options Type_info<
    mysqlshdk::db::Connection_options>::to_native(const shcore::Value &in) {
  return mysqlsh::get_connection_options(in);
}

mysqlshdk::db::Connection_options
Type_info<mysqlshdk::db::Connection_options>::default_value() {
  return mysqlshdk::db::Connection_options();
}

}  // namespace detail
}  // namespace shcore
#endif
