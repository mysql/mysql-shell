/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/mysql/auth_plugins/kerberos.h"

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/auth_plugins/common.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
namespace kerberos {

namespace {

#ifdef _WIN32
struct st_mysql_client_plugin *get_authentication_kerberos_client(MYSQL *conn) {
  return auth::get_authentication_plugin(conn,
                                         "authentication_kerberos_client");
}
#endif

}  // namespace

#ifdef _WIN32
void set_client_auth_mode(MYSQL *conn) {
  auto conn_data = auth::get_connection_options_for_mysql(conn);
  assert(conn_data != nullptr);

  if (!conn_data->has_kerberos_auth_mode()) return;

  auto client_auth_mode = conn_data->get_kerberos_auth_mode();
  const auto plugin = get_authentication_kerberos_client(conn);

  if (mysql_plugin_options(plugin, "plugin_authentication_kerberos_client_mode",
                           client_auth_mode.c_str())) {
    throw std::runtime_error(
        "Failed to set the kerberos client authentication mode on "
        "authentication_kerberos_client plugin.");
  }
}
#endif

}  // namespace kerberos
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
