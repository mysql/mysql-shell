/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "modules/util/dump/instance_cache.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/dump/schema_dumper.h"

namespace mysqlsh {
namespace dump {

Instance_cache_builder::Instance_cache_builder(
    const std::shared_ptr<mysqlshdk::db::ISession> &session)
    : m_session(session) {
  fetch_metadata();
}

Instance_cache_builder &Instance_cache_builder::users(const Users &included,
                                                      const Users &excluded) {
  m_cache.users = Schema_dumper(m_session).get_users(included, excluded);

  return *this;
}

Instance_cache Instance_cache_builder::build() { return std::move(m_cache); }

void Instance_cache_builder::fetch_metadata() {
  fetch_ndbinfo();
  fetch_server_metadata();
}

void Instance_cache_builder::fetch_server_metadata() {
  const auto &co = m_session->get_connection_options();

  m_cache.user = co.get_user();

  {
    const auto result = m_session->query("SELECT @@GLOBAL.HOSTNAME;");

    if (const auto row = result->fetch_one()) {
      m_cache.server = row->get_string(0);
    } else {
      m_cache.server = co.has_host() ? co.get_host() : "localhost";
    }
  }

  {
    const auto result = m_session->query("SELECT @@GLOBAL.VERSION;");

    if (const auto row = result->fetch_one()) {
      m_cache.server_version = row->get_string(0);
    } else {
      m_cache.server_version = "unknown";
    }
  }

  m_cache.hostname = mysqlshdk::utils::Net::get_hostname();

  {
    const auto result = m_session->query("SELECT @@GLOBAL.GTID_EXECUTED;");

    if (const auto row = result->fetch_one()) {
      m_cache.gtid_executed = row->get_string(0);
    }
  }
}

void Instance_cache_builder::fetch_ndbinfo() {
  try {
    const auto result =
        m_session->query("SHOW VARIABLES LIKE 'ndbinfo\\_version'");
    m_cache.has_ndbinfo = nullptr != result->fetch_one();
  } catch (const mysqlshdk::db::Error &) {
    log_error("Failed to check if instance is a part of NDB cluster.");
  }
}

}  // namespace dump
}  // namespace mysqlsh
