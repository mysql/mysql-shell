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

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/ssh/ssh_manager.h"

namespace mysqlshdk {
namespace db {
void ISession::connect(const mysqlshdk::db::Connection_options &data) {
  do_connect(data);
  if (is_open()) {
    const auto &ssh = data.get_ssh_options();
    if (ssh.has_data()) {
      mysqlshdk::ssh::current_ssh_manager()->port_usage_increment(ssh);
    }
  }
}

void ISession::close() {
  auto connection_options = get_connection_options();
  bool was_open = is_open();
  do_close();
  if (was_open) {
    try {
      const auto &ssh = connection_options.get_ssh_options();
      if (ssh.has_data()) {
        mysqlshdk::ssh::current_ssh_manager()->port_usage_decrement(ssh);
      }
    } catch (const std::exception &ex) {
      log_error("An error occurred while closing connection: %s", ex.what());
      // no op
    }
  }
}
}  // namespace db
}  // namespace mysqlshdk
