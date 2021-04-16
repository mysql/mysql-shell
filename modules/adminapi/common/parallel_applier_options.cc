/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/parallel_applier_options.h"
#include <string>
#include "mysqlshdk/libs/mysql/repl_config.h"
#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlsh {
namespace dba {

void Parallel_applier_options::read_option_values(
    const mysqlshdk::mysql::IInstance &instance) {
  mysqlshdk::utils::Version version = instance.get_version();

  if (binlog_transaction_dependency_tracking.is_null()) {
    binlog_transaction_dependency_tracking =
        instance.get_sysvar_string(kBinlogTransactionDependencyTracking);
  }

  if (replica_preserve_commit_order.is_null()) {
    replica_preserve_commit_order = instance.get_sysvar_string(
        mysqlshdk::mysql::get_replication_option_keyword(
            version, kReplicaPreserveCommitOrder));
  }

  if (replica_parallel_type.is_null()) {
    replica_parallel_type = instance.get_sysvar_string(
        mysqlshdk::mysql::get_replication_option_keyword(version,
                                                         kReplicaParallelType));
  }

  if (transaction_write_set_extraction.is_null()) {
    transaction_write_set_extraction =
        instance.get_sysvar_string(kTransactionWriteSetExtraction);
  }

  if (replica_parallel_workers.is_null()) {
    replica_parallel_workers = instance.get_sysvar_int(
        mysqlshdk::mysql::get_replication_option_keyword(
            version, kReplicaParallelWorkers));
  }
}

std::vector<std::tuple<std::string, std::string>>
Parallel_applier_options::get_required_values(
    const mysqlshdk::utils::Version &version,
    bool skip_transaction_writeset_extraction) const {
  std::vector<std::tuple<std::string, std::string>> req_cfgs{
      std::make_tuple(mysqlshdk::mysql::get_replication_option_keyword(
                          version, kReplicaParallelType),
                      "LOGICAL_CLOCK"),
      std::make_tuple(mysqlshdk::mysql::get_replication_option_keyword(
                          version, kReplicaPreserveCommitOrder),
                      "ON"),
      std::make_tuple(kBinlogTransactionDependencyTracking, "WRITESET"),
      std::make_tuple(kTransactionWriteSetExtraction, "XXHASH64")};

  if (skip_transaction_writeset_extraction) {
    req_cfgs.pop_back();
  }

  return req_cfgs;
}

std::map<std::string, mysqlshdk::utils::nullable<std::string>>
Parallel_applier_options::get_current_settings(
    const mysqlshdk::utils::Version &version) const {
  std::map<std::string, mysqlshdk::utils::nullable<std::string>> ret_val;

  ret_val[mysqlshdk::mysql::get_replication_option_keyword(
      version, kReplicaParallelType)] = replica_parallel_type;
  ret_val[mysqlshdk::mysql::get_replication_option_keyword(
      version, kReplicaPreserveCommitOrder)] = replica_preserve_commit_order;
  ret_val[kBinlogTransactionDependencyTracking] =
      binlog_transaction_dependency_tracking;
  ret_val[kTransactionWriteSetExtraction] = transaction_write_set_extraction;
  ret_val[mysqlshdk::mysql::get_replication_option_keyword(
      version, kReplicaParallelWorkers)] =
      std::to_string(replica_parallel_workers.get_safe());

  return ret_val;
}

}  // namespace dba
}  // namespace mysqlsh
