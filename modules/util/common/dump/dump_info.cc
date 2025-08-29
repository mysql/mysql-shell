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

#include "modules/util/common/dump/dump_info.h"

#include <utility>

namespace mysqlsh {
namespace dump {
namespace common {

Dump_info dump_info(const shcore::json::Value &object) {
  using mysqlshdk::utils::Version;

  using shcore::json::optional_bool;
  using shcore::json::optional_object;
  using shcore::json::optional_object_array;
  using shcore::json::optional_string_array;
  using shcore::json::optional_uint;

  const auto optional_string = [](const shcore::json::Value &o, const char *n) {
    return shcore::json::optional(o, n, true).value_or(std::string{});
  };

  const auto required_string = [](const shcore::json::Value &o, const char *n) {
    return shcore::json::required(o, n, false);
  };

  Dump_info info;

  info.version = Version{optional_string(object, "version")};
  info.origin = optional_string(object, "origin");

  info.has_users = object.HasMember("users");

  info.default_charset = optional_string(object, "defaultCharacterSet");
  info.tz_utc = optional_bool(object, "tzUtc").value_or(true);
  info.bytes_per_chunk = optional_uint(object, "bytesPerChunk").value_or(0);

  info.gtid_executed_inconsistent =
      optional_bool(object, "gtidExecutedInconsistent").value_or(false);
  info.consistent = optional_bool(object, "consistent").value_or(false);
  info.mds_compatibility =
      optional_bool(object, "mdsCompatibility").value_or(false);

  if (const auto target_version = optional_string(object, "targetVersion");
      !target_version.empty()) {
    info.target_version = Version{target_version};
  }

  info.has_library_ddl = optional_bool(object, "hasLibraryDdl").value_or(false);

  info.estimated_row_count =
      optional_uint(object, "estimatedRowCount").value_or(0);
  info.estimated_data_size =
      optional_uint(object, "estimatedDataSize").value_or(0);

  if (const auto source = optional_object(object, "source");
      source.has_value()) {
    info.source = server_info(*source);
  } else {
    if (const auto server_ver = optional_string(object, "serverVersion");
        !server_ver.empty()) {
      info.source.version = server_version(server_ver);
    }

    info.source.binlog.file.name = optional_string(object, "binlogFile");
    info.source.binlog.file.position =
        optional_uint(object, "binlogPosition").value_or(0);
    info.source.binlog.gtid_executed = optional_string(object, "gtidExecuted");

    info.source.sysvars.partial_revokes =
        optional_bool(object, "partialRevokes");
  }

  if (auto compatibility_options =
          optional_string_array(object, "compatibilityOptions");
      compatibility_options.has_value()) {
    info.compatibility_options = std::move(*compatibility_options);
  }

  if (auto capabilities = optional_object_array(object, "capabilities");
      capabilities.has_value()) {
    info.capabilities.reserve(capabilities->size());

    for (const auto &capability : *capabilities) {
      auto id = required_string(capability, "id");
      auto opt_id = capability::to_capability(id);

      info.capabilities.emplace_back(Capability_info{
          std::move(id),
          required_string(capability, "description"),
          Version(required_string(capability, "versionRequired")),
          std::move(opt_id),
      });
    }
  }

  info.has_checksum = optional_bool(object, "checksum").value_or(false);

  return info;
}

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh
