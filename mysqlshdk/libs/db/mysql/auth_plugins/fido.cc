/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/mysql/auth_plugins/fido.h"

#include <mysql.h>

#include <string>
#include <vector>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/db/mysql/auth_plugins/common.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"

namespace mysqlshdk {
namespace db {
namespace mysql {

namespace fido {

using uchar = unsigned char; /* Short for unsigned char */

namespace {

constexpr int HOSTNAME_LENGTH = 255;

/**
 * Callback for print operations on the authentication_fido_client and
 * authentication_webauthn_client plugins
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
    memcpy(buffer, reply.c_str(), buffer_len);
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

/**
 * Method to parse the parameter for --register-factor, taken verbatim from
 * the CLI code (unavailable in the client lib).
 */
bool parse_register_factor(const char *what_factor,
                           std::vector<unsigned int> *list) {
  assert(list);
  std::string token;
  std::stringstream str(what_factor);
  while (getline(str, token, ',')) {
    unsigned int nth_factor = 0;
    try {
      nth_factor = std::stoul(token);
    } catch (std::invalid_argument &) {
      return false;
    } catch (std::out_of_range &) {
      return false;
    }
    /* nth_factor can be either 2 or 3 */
    if (nth_factor < 2 || nth_factor > 3 || list->size() == 2 ||
        std::find(list->begin(), list->end(), nth_factor) != list->end()) {
      return false;
    }
    list->push_back(nth_factor);
  }
  return true;
}

void set_callbacks(struct st_mysql_client_plugin *plugin,
                   const std::string &plugin_name) {
  bool is_webauthn = plugin_name == "authentication_webauthn_client";
  const char *callback_option =
      is_webauthn ? "plugin_authentication_webauthn_client_messages_callback"
                  : "fido_messages_callback";

  /* set plugin message handler */
  if (mysql_plugin_options(plugin, callback_option,
                           (const void *)&plugin_messages_callback)) {
    throw std::runtime_error(shcore::str_format(
        "Failed to set message callback on %s plugin.", plugin_name.c_str()));
  }

  if (is_webauthn) {
    mysql_plugin_options(
        plugin, "plugin_authentication_webauthn_client_callback_get_password",
        (const void *)&plugin_messages_callback_get_password);

    mysql_plugin_options(
        plugin, "plugin_authentication_webauthn_client_callback_get_uint",
        (const void *)&plugin_messages_callback_get_uint);
  }
}

}  // namespace

void register_callbacks(MYSQL *conn, const std::string &plugin_name) {
  auto plugin = auth::get_authentication_plugin(conn, plugin_name.c_str());
  set_callbacks(plugin, plugin_name);
}

/**
 * Method to handle FIDO device registration for an account using
 * authentication_fido. Taken from the CLI code and adapted to the Shell
 * (unavailable on the client lib)
 */

void register_device(MYSQL *conn, const char *factor) {
  std::vector<unsigned int> list;
  if (!parse_register_factor(factor, &list)) {
    throw std::invalid_argument(
        "Incorrect value specified for --register-factor option. "
        "Correct values can be '2', '3', '2,3' or '3,2'.");
  }

  st_mysql_client_plugin *plugin = nullptr;

  for (auto this_factor : list) {
    shcore::sqlstring init("ALTER USER USER() ? FACTOR INITIATE REGISTRATION",
                           0);
    init << this_factor;
    init.done();

    if (mysql_real_query(conn, init.str().c_str(), init.size())) {
      auth::handle_mysql_error(conn);
    }

    MYSQL_RES *result;
    if (!(result = mysql_store_result(conn))) {
      auth::handle_mysql_error(conn);
    }

    if (auto count = mysql_num_rows(result) != 1) {
      const char *reason = count == 0 ? "no" : "multiple";
      throw std::runtime_error(shcore::str_format(
          "Device registration failed, %s server challenge returned.", reason));
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
      // This should not happen since mysql_store_result was used, so a row is
      // always expected
      throw std::runtime_error("Unexpected failure getting server challenge.");
    }

    unsigned long *lengths = mysql_fetch_lengths(result);
    if (!lengths) {
      auth::handle_mysql_error(conn);
    }

    // Servers < 8.2.0 return server challenge (only fido authentication is
    // supported). Newer versions return server challenge and plugin name
    std::string plugin_name = "authentication_fido_client";
    int field_count = mysql_field_count(conn);
    if (field_count > 1) {
      plugin_name.assign(row[1], lengths[1]);
    }

    /*
      max length of challenge can be 32 (random challenge) +
      255 (relying party ID) + 255 (host name) + 32 (user name) + 4 byte for
      length encodings
    */
    if (lengths[0] > (CHALLENGE_LENGTH + RELYING_PARTY_ID_LENGTH +
                      HOSTNAME_LENGTH + USERNAME_LENGTH + 4)) {
      mysql_free_result(result);
      throw std::runtime_error(
          "Received server challenge is corrupt. Please retry.");
    }

    uchar server_challenge[CHALLENGE_LENGTH + RELYING_PARTY_ID_LENGTH +
                           HOSTNAME_LENGTH + USERNAME_LENGTH + 4];

    memset(server_challenge, 0,
           CHALLENGE_LENGTH + RELYING_PARTY_ID_LENGTH + HOSTNAME_LENGTH +
               USERNAME_LENGTH + 4);

    memcpy(&server_challenge[0], row[0], lengths[0]);
    mysql_free_result(result);

    if (nullptr == plugin) {
      plugin = auth::get_authentication_plugin(conn, plugin_name.c_str());
    }

    /* set server challenge in plugin */
    if (mysql_plugin_options(plugin, "registration_challenge",
                             &server_challenge[0])) {
      // my_free(server_challenge);
      throw std::runtime_error(
          shcore::str_format("Failed to set \"registration_challenge\" for %s "
                             "plugin",
                             plugin_name.c_str()));
    }

    /* get challenge response from plugin */
    /* NOTE: Setting this plugin option will trigger the read from the FIDO
     * device */
    uchar *server_challenge_response = nullptr;
    if (mysql_plugin_get_option(plugin, "registration_response",
                                &server_challenge_response)) {
      throw std::runtime_error(
          shcore::str_format("Failed to get \"registration_response\" on %s "
                             "plugin",
                             plugin_name.c_str()));
    }

    shcore::sqlstring finish(
        "ALTER USER USER() ? FACTOR FINISH REGISTRATION SET "
        "CHALLENGE_RESPONSE AS ?",
        0);

    finish << this_factor;
    finish << reinterpret_cast<char *>(server_challenge_response);
    finish.done();

    if (mysql_real_query(conn, finish.str().c_str(), finish.size())) {
      auth::handle_mysql_error(conn);
    }
  }
}
}  // namespace fido
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
