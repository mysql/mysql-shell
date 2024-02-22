/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/mysql/auth_plugins/oci.h"

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/auth_plugins/common.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
namespace oci {

namespace {

struct st_mysql_client_plugin *get_authentication_oci_client(MYSQL *conn) {
  return auth::get_authentication_plugin(conn, "authentication_oci_client");
}

}  // namespace

void set_config_file(MYSQL *conn) {
  const auto plugin = get_authentication_oci_client(conn);
  const auto &oci_config_file =
      mysqlsh::current_shell_options()->get().oci_config_file;

  if (mysql_plugin_options(plugin, "oci-config-file",
                           oci_config_file.c_str())) {
    throw std::runtime_error(
        "Failed to set the OCI config file path on authentication_oci_client "
        "plugin.");
  }
}

}  // namespace oci
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
