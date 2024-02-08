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

#include "mysqlshdk/libs/db/mysql/auth_plugins/webauthn.h"

#include <cassert>

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/auth_plugins/common.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
namespace webauthn {

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

}  // namespace webauthn
}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
