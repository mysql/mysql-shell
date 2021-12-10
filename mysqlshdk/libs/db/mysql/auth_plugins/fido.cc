/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/mysql/auth_plugins/fido.h"

#include <mysql.h>

#include <string>
#include <vector>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"

namespace mysqlshdk {
namespace db {
namespace mysql {

namespace fido {

using uchar = unsigned char; /* Short for unsigned char */

namespace {

constexpr int HOSTNAME_LENGTH = 255;

void handle_mysql_error(MYSQL *conn) {
  unsigned int code = mysql_errno(conn);

  throw mysqlshdk::db::Error(mysql_error(conn), code, mysql_sqlstate(conn));
}

/**
 * Callback for print operations on the authentication_fido_client plugin
 */
void plugin_messages_callback(const char *msg) {
  mysqlsh::current_console()->print_info(msg);
}

/**
 * Method to parse the parameter for --fido-register-factor, taken verbatim from
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

struct st_mysql_client_plugin *get_authentication_fido_client(MYSQL *conn) {
  /* load fido client authentication plugin if required */
  auto plugin = mysql_client_find_plugin(conn, "authentication_fido_client",
                                         MYSQL_CLIENT_AUTHENTICATION_PLUGIN);

  if (!conn) {
    handle_mysql_error(conn);
  }

  return plugin;
}

void set_print_callback(struct st_mysql_client_plugin *plugin) {
  /* set plugin message handler */
  if (mysql_plugin_options(plugin, "fido_messages_callback",
                           (const void *)&plugin_messages_callback)) {
    throw std::runtime_error(
        "Failed to set message callback on authentication_fido_client plugin.");
  }
}

}  // namespace

void register_print_callback(MYSQL *conn) {
  auto plugin = get_authentication_fido_client(conn);
  set_print_callback(plugin);
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
        "Incorrect value specified for --fido-register-factor option. "
        "Correct values can be '2', '3', '2,3' or '3,2'.");
  }

  auto plugin = get_authentication_fido_client(conn);
  set_print_callback(plugin);

  for (auto this_factor : list) {
    shcore::sqlstring init("ALTER USER USER() ? FACTOR INITIATE REGISTRATION",
                           0);
    init << this_factor;
    init.done();

    if (mysql_real_query(conn, init.str().c_str(), init.size())) {
      handle_mysql_error(conn);
    }

    MYSQL_RES *result;
    if (!(result = mysql_store_result(conn))) {
      handle_mysql_error(conn);
    }

    if (mysql_num_rows(result) > 1) {
      handle_mysql_error(conn);
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    unsigned long *lengths = mysql_fetch_lengths(result);
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

    memcpy(&server_challenge[0], row[0], lengths[0]);
    mysql_free_result(result);

    /* set server challenge in plugin */
    if (mysql_plugin_options(plugin, "registration_challenge",
                             &server_challenge[0])) {
      // my_free(server_challenge);
      throw std::runtime_error(
          "Failed to set \"registration_challenge\" for authentication_fido "
          "plugin");
    }

    /* get challenge response from plugin */
    /* NOTE: Setting this plugin option will trigger the read from the FIDO
     * device */
    uchar *server_challenge_response = nullptr;
    if (mysql_plugin_get_option(plugin, "registration_response",
                                &server_challenge_response)) {
      throw std::runtime_error(
          "Failed to get \"registration_response\" on authentication_fido "
          "plugin");
    }

    shcore::sqlstring finish(
        "ALTER USER USER() ? FACTOR FINISH REGISTRATION SET "
        "CHALLENGE_RESPONSE AS ?",
        0);

    finish << this_factor;
    finish << reinterpret_cast<char *>(server_challenge_response);
    finish.done();

    if (mysql_real_query(conn, finish.str().c_str(), finish.size())) {
      handle_mysql_error(conn);
    }
  }
}
}  // namespace fido
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
