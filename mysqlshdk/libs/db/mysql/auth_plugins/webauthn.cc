/*
 * Copyright (c) 2023, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/db/mysql/auth_plugins/webauthn.h"

#include <cassert>

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/auth_plugins/common.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
namespace webauthn {

namespace {
/**
 * Callback for print operations on the authentication_webauthn_client plugin
 */
void plugin_messages_callback(const char *msg) {
  mysqlsh::current_console()->print_info(msg);
}

/**
 * Webauthn callback for password retrieval when the FIDO device has PIN
 */
int plugin_messages_callback_get_password(char *buffer,
                                          const unsigned int buffer_len) {
  std::string reply;
  // The empty prompt is because the plugin already printed the prompt using the
  // message callback.
  if (mysqlsh::current_console()->prompt_password("", &reply) ==
      shcore::Prompt_result::Ok) {
    // check if reply will fit inside buffer - if not, the function fails
    if (reply.length() >= buffer_len) {
      shcore::clear_buffer(reply);
      return 1;
    }

    std::memcpy(buffer, reply.c_str(), reply.length() + 1);
    shcore::clear_buffer(reply);
    return 0;
  }
  return 1;
}

/**
 * Webauthn callback for item selection when multiple credentials are found on
 * the device.
 */
int plugin_messages_callback_get_uint(unsigned int *val) {
  std::string reply;
  // The empty prompt is because the plugin already printed the prompt using the
  // message callback.
  if (mysqlsh::current_console()->prompt("", &reply) ==
      shcore::Prompt_result::Ok) {
    try {
      *val = shcore::lexical_cast<unsigned int>(reply);
      return 0;
    } catch (const std::invalid_argument &) {
      // NO-OP, plugin will handle it through the returned value
    }
  }
  return 1;
}
}  // namespace

void register_callbacks(MYSQL *conn) {
  const auto plugin =
      auth::get_authentication_plugin(conn, "authentication_webauthn_client");
  assert(plugin != nullptr);

  /* set plugin message handler */
  if (mysql_plugin_options(
          plugin, "plugin_authentication_webauthn_client_messages_callback",
          reinterpret_cast<const void *>(&plugin_messages_callback))) {
    throw std::runtime_error(
        "Failed to set message callback on webauthn plugin.");
  }

  if (mysql_plugin_options(
          plugin, "plugin_authentication_webauthn_client_callback_get_password",
          reinterpret_cast<const void *>(
              &plugin_messages_callback_get_password))) {
    throw std::runtime_error(
        "Failed to set password message callback on webauthn plugin.");
  }

  if (mysql_plugin_options(
          plugin, "plugin_authentication_webauthn_client_callback_get_uint",
          reinterpret_cast<const void *>(&plugin_messages_callback_get_uint))) {
    throw std::runtime_error(
        "Failed to set uint message callback on webauthn plugin.");
  }
}

void set_preserve_privacy(MYSQL *conn) {
  auto conn_data = auth::get_connection_options_for_mysql(conn);
  assert(conn_data != nullptr);

  if (conn_data->has(mysqlshdk::db::kWebauthnClientPreservePrivacy)) {
    auto &str_value =
        conn_data->get(mysqlshdk::db::kWebauthnClientPreservePrivacy);
    bool value = str_value.at(0) == '1';

    const auto plugin =
        auth::get_authentication_plugin(conn, "authentication_webauthn_client");

    if (mysql_plugin_options(plugin,
                             "authentication_webauthn_client_preserve_privacy",
                             &value)) {
      throw std::runtime_error(
          "Failed to set the Preserve Privacy option on "
          "authentication_webauthn_client plugin.");
    } else {
      log_debug3("Using authentication_webauthn_client_preserve_privacy =  %s",
                 str_value.c_str());
    }
  }
}

void set_device_index(MYSQL *conn) {
  auto conn_data = auth::get_connection_options_for_mysql(conn);
  assert(conn_data != nullptr);

  if (conn_data->has(mysqlshdk::db::kWebauthnClientDeviceIndex)) {
    auto int_value =
        conn_data->get_numeric(mysqlshdk::db::kWebauthnClientDeviceIndex);

    const auto plugin =
        auth::get_authentication_plugin(conn, "authentication_webauthn_client");

    if (mysql_plugin_options(plugin, "device", &int_value)) {
      throw std::runtime_error(
          "Failed to set the device option on "
          "authentication_webauthn_client plugin.");
    } else {
      log_debug3("Using device option on authentication_webauthn_client =  %d",
                 int_value);
    }
  }
}

}  // namespace webauthn
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
