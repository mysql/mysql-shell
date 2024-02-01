/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_ASYNC_REPLICATION_OPTIONS_H_
#define MODULES_ADMINAPI_COMMON_ASYNC_REPLICATION_OPTIONS_H_

#include <optional>
#include <string>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/mod_common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh::dba {

struct Async_replication_options {
  enum Unpack_target {
    NONE,   // none
    CREATE  // all options OK
  };

  Async_replication_options() noexcept = default;
  explicit Async_replication_options(Unpack_target t) noexcept : target(t) {}

  Unpack_target target = NONE;

  std::optional<mysqlshdk::mysql::Auth_options> repl_credentials;

  std::optional<int> connect_retry;
  std::optional<int> retry_count;
  std::optional<bool> auto_failover;
  std::optional<int> delay;
  std::optional<double> heartbeat_period;
  std::optional<std::string> compression_algos;
  std::optional<int> zstd_compression_level;
  std::optional<std::string> bind;
  std::optional<std::string> network_namespace;

  Cluster_ssl_mode ssl_mode = Cluster_ssl_mode::NONE;
  Replication_auth_type auth_type = Replication_auth_type::PASSWORD;
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_ASYNC_REPLICATION_OPTIONS_H_
