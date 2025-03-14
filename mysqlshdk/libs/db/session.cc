/*
 * Copyright (c) 2021, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/session.h"

#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/ssh/ssh_manager.h"

namespace mysqlshdk {
namespace db {

Query_attribute::Query_attribute(
    std::string n, std::unique_ptr<IQuery_attribute_value> v) noexcept
    : name(std::move(n)), value(std::move(v)) {}

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

void ISession::set_sql_mode(const std::string &sql_mode) {
  m_sql_mode = shcore::str_upper(sql_mode);
  m_ansi_quotes_enabled = m_sql_mode->find("ANSI_QUOTES") != std::string::npos;
  m_no_backslash_escapes_enabled =
      m_sql_mode->find("NO_BACKSLASH_ESCAPES") != std::string::npos;
}

void ISession::refresh_sql_mode() {
  assert(is_open());
  try {
    auto result = query("select @@sql_mode;");
    auto row = result->fetch_one();

    if (!row || row->is_null(0)) throw std::runtime_error("Missing sql_mode");

    set_sql_mode(row->get_string(0));
  } catch (...) {
    m_sql_mode = std::nullopt;
    m_ansi_quotes_enabled = m_no_backslash_escapes_enabled = false;
  }
}

}  // namespace db
}  // namespace mysqlshdk
