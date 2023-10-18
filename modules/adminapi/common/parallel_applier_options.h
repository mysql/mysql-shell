/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_PARALLEL_APPLIER_OPTIONS_H_
#define MODULES_ADMINAPI_COMMON_PARALLEL_APPLIER_OPTIONS_H_

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlsh {
namespace dba {

inline constexpr const char kBinlogTransactionDependencyTracking[] =
    "binlog_transaction_dependency_tracking";
inline constexpr const char kReplicaPreserveCommitOrder[] =
    "replica_preserve_commit_order";
inline constexpr const char kReplicaParallelWorkers[] =
    "replica_parallel_workers";
inline constexpr const int64_t kReplicaParallelWorkersDefault = 4;
inline constexpr const char kReplicaParallelType[] = "replica_parallel_type";
inline constexpr const char kTransactionWriteSetExtraction[] =
    "transaction_write_set_extraction";

struct Parallel_applier_options {
  Parallel_applier_options() = default;

  explicit Parallel_applier_options(
      const mysqlshdk::mysql::IInstance &instance) {
    read_option_values(instance);
  }

  /**
   * Get the list of required values for the parallel appliers options
   *
   * @param skip_transaction_writeset_extraction boolean value to indicate if
   * skip_transaction_writeset_extraction must be skipped from the list
   *
   * @return a list of pairs containing each option and the corresponding
   * required value.
   */
  std::vector<std::tuple<std::string, std::string>> get_required_values(
      const mysqlshdk::utils::Version &version,
      bool skip_transaction_writeset_extraction = false) const;

  /**
   * Get all parallel applier settings and current values
   *
   * @return A list of all parallel applier settings and corresponding values in
   * place
   */
  std::map<std::string, std::optional<std::string>> get_current_settings(
      const mysqlshdk::utils::Version &version) const;

  std::optional<std::string> binlog_transaction_dependency_tracking;
  std::optional<std::string> replica_preserve_commit_order;
  std::optional<std::string> replica_parallel_type;
  std::optional<std::string> transaction_write_set_extraction;
  std::optional<int64_t> replica_parallel_workers;

 private:
  /**
   * Read the Parallel Applier option values from the given Instance.
   *
   * @param instance target Instance object to read the GR options.
   */
  void read_option_values(const mysqlshdk::mysql::IInstance &instance);
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_PARALLEL_APPLIER_OPTIONS_H_
