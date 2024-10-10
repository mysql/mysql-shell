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

#ifndef MODULES_UTIL_COMMON_DUMP_SERVER_INFO_H_
#define MODULES_UTIL_COMMON_DUMP_SERVER_INFO_H_

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace dump {
namespace common {

struct Binlog {
  struct File {
    std::string name;
    uint64_t position = 0;

    bool operator==(const Binlog::File &other) const {
      return name == other.name && position == other.position;
    }

    std::string to_string() const {
      return name + ':' + std::to_string(position);
    }
  } file;
  struct {
    std::string do_db;
    std::string ignore_db;
  } startup_options;
  std::string gtid_executed;
};

struct Server_version {
  mysqlshdk::utils::Version number;
  bool is_5_6 = false;
  bool is_5_7 = false;
  bool is_8_0 = false;
  bool is_maria_db = false;
};

struct Server_variables {
  std::string_view hostname;
  std::optional<int8_t> lower_case_table_names;
  std::optional<bool> partial_revokes;
  uint16_t port = 0;
  std::string_view server_uuid;
  std::string_view version;

  std::map<std::string, std::string> all;
};

struct Replication_topology {
  std::string canonical_address;
  std::string group_name;
  std::string view_change_uuid;
  std::string cluster_id;
  std::string cluster_set_id;
};

struct Server_info {
  Server_version version;
  Binlog binlog;
  Server_variables sysvars;
  Replication_topology topology;
};

Binlog binlog(const std::shared_ptr<mysqlshdk::db::ISession> &session,
              const Server_version &version, bool quiet = false);

Binlog binlog(const std::shared_ptr<mysqlshdk::db::ISession> &session,
              bool quiet = false);

void serialize(const Binlog &binlog, shcore::JSON_dumper *dumper);

Binlog binlog(const shcore::json::Value &object);

Server_version server_version(
    const std::shared_ptr<mysqlshdk::db::ISession> &session);

Server_version server_version(std::string_view version);

Server_variables server_variables(
    const std::shared_ptr<mysqlshdk::db::ISession> &session);

Replication_topology replication_topology(
    const std::shared_ptr<mysqlshdk::db::ISession> &session);

void serialize(const Server_info &info, shcore::JSON_dumper *dumper,
               bool binlog = true, bool sysvars = true);

Server_info server_info(const shcore::json::Value &object);

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COMMON_DUMP_SERVER_INFO_H_
