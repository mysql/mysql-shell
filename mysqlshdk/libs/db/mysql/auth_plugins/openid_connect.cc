/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/mysql/auth_plugins/openid_connect.h"

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/auth_plugins/common.h"
#include "mysqlshdk/libs/utils/utils_file.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
namespace openid_connect {

namespace {

struct st_mysql_client_plugin *get_authentication_openid_connect_client(
    MYSQL *conn) {
  return auth::get_authentication_plugin(
      conn, "authentication_openid_connect_client");
}

}  // namespace

void set_client_token_file(MYSQL *conn) {
  auto conn_data = auth::get_connection_options_for_mysql(conn);
  assert(conn_data != nullptr);

  const auto plugin = get_authentication_openid_connect_client(conn);

  if (!conn_data->has(kOpenIdConnectAuthenticationClientTokenFile)) {
    mysql_plugin_options(plugin, "id-token-file",
                         nullptr);  // reset id-token file in client plugin
    return;
  }

  std::string token_file_path =
      conn_data->get(kOpenIdConnectAuthenticationClientTokenFile);
  if (token_file_path.empty()) {
    mysql_plugin_options(plugin, "id-token-file",
                         nullptr);  // reset id-token file in client plugin
    return;
  }

  if (!shcore::is_file(token_file_path)) {
    throw std::runtime_error(
        "Token file for openid connect authorization does not exist for "
        "authentication_openid_connect_client plugin.");
  }

  if (mysql_plugin_options(plugin, "id-token-file", token_file_path.c_str())) {
    throw std::runtime_error(
        "Failed to set openid connect token file on "
        "authentication_openid_connect_client plugin.");
  }
}

}  // namespace openid_connect
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
