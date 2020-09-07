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

#ifndef MODULES_UTIL_DUMP_INSTANCE_CACHE_H_
#define MODULES_UTIL_DUMP_INSTANCE_CACHE_H_

#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dump {

struct Instance_cache {
  bool has_ndbinfo = false;
  std::string user;
  std::string hostname;
  std::string server;
  std::string server_version;
  std::string gtid_executed;
  std::vector<shcore::Account> users;
};

class Instance_cache_builder final {
 public:
  using Users = std::vector<shcore::Account>;

  Instance_cache_builder() = delete;

  explicit Instance_cache_builder(
      const std::shared_ptr<mysqlshdk::db::ISession> &session);

  Instance_cache_builder(const Instance_cache_builder &) = delete;
  Instance_cache_builder(Instance_cache_builder &&) = default;

  ~Instance_cache_builder() = default;

  Instance_cache_builder &operator=(const Instance_cache_builder &) = delete;
  Instance_cache_builder &operator=(Instance_cache_builder &&) = default;

  Instance_cache_builder &users(const Users &included, const Users &excluded);

  Instance_cache build();

 private:
  void fetch_metadata();

  void fetch_server_metadata();

  void fetch_ndbinfo();

  std::shared_ptr<mysqlshdk::db::ISession> m_session;

  Instance_cache m_cache;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_INSTANCE_CACHE_H_
