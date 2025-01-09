/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_COMMON_DUMP_DUMP_INFO_H_
#define MODULES_UTIL_COMMON_DUMP_DUMP_INFO_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/common/dump/server_info.h"

namespace mysqlsh {
namespace dump {
namespace common {

struct Capability_info {
  std::string id;
  std::string description;
  mysqlshdk::utils::Version version_required;
};

struct Dump_info {
  mysqlshdk::utils::Version version;
  std::string origin;
  bool has_users = false;
  std::string default_charset;
  bool tz_utc = true;
  uint64_t bytes_per_chunk = 0;
  bool gtid_executed_inconsistent = false;
  bool consistent = false;
  bool mds_compatibility = false;
  std::optional<mysqlshdk::utils::Version> target_version;
  bool has_library_ddl = false;
  Server_info source;
  std::vector<std::string> compatibility_options;
  std::vector<Capability_info> capabilities;
  bool has_checksum = false;
};

Dump_info dump_info(const shcore::json::Value &object);

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COMMON_DUMP_DUMP_INFO_H_
