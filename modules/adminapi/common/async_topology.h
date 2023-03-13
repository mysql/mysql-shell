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
#ifndef MODULES_ADMINAPI_COMMON_ASYNC_TOPOLOGY_H_
#define MODULES_ADMINAPI_COMMON_ASYNC_TOPOLOGY_H_

#include "modules/adminapi/common/async_replication_options.h"

#include <list>
#include <string>
#include <vector>

#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh {
namespace dba {

struct Managed_async_channel_source {
  std::string host;
  int port = 0;
  int weight = 0;

  Managed_async_channel_source(const std::string &source_string, int wgt)
      : weight(wgt) {
    std::pair<std::string, int> src_pair =
        mysqlshdk::utils::split_host_and_port(source_string);
    host = std::move(src_pair.first);
    port = src_pair.second;
  }

  explicit Managed_async_channel_source(const std::string &source_string) {
    std::pair<std::string, int> src_pair =
        mysqlshdk::utils::split_host_and_port(source_string);
    host = std::move(src_pair.first);
    port = src_pair.second;
  }

  Managed_async_channel_source() = default;

  std::string to_string() const {
    return shcore::str_format("%s:%d", host.c_str(), port);
  }

  struct Predicate_weight final {
    const Managed_async_channel_source &lhs;
    explicit constexpr Predicate_weight(
        const Managed_async_channel_source &other)
        : lhs(other) {}

    bool operator()(const Managed_async_channel_source &rhs) const {
      return lhs.host == rhs.host && lhs.port == rhs.port &&
             lhs.weight == rhs.weight;
    }
  };

  struct Predicate final {
    const Managed_async_channel_source &lhs;
    explicit constexpr Predicate(const Managed_async_channel_source &other)
        : lhs(other) {}

    bool operator()(const Managed_async_channel_source &rhs) const {
      return lhs == rhs;
    }
  };

  bool operator==(const Managed_async_channel_source &other) const {
    return host == other.host && port == other.port;
  }

  bool operator<(const Managed_async_channel_source &other) const {
    if (weight == other.weight) {
      return to_string() < other.to_string();
    }

    return weight < other.weight;
  }

  bool operator>(const Managed_async_channel_source &other) const {
    if (weight == other.weight) {
      return to_string() > other.to_string();
    }

    return weight > other.weight;
  }

  bool operator!=(const Managed_async_channel_source &other) const {
    return !operator==(other);
  }
};

struct Managed_async_channel {
  std::string channel_name;
  std::string host;
  int port = 0;
  std::string network_namespace;
  int primary_weight = 0;
  int secondary_weight = 0;
  std::string managed_name;
  bool automatic_sources = false;
  bool automatic_failover = false;
  std::set<Managed_async_channel_source,
           std::greater<Managed_async_channel_source>>
      sources;
};

// AR topology change functions.
// Assumes validations, metadata updates and account management are handled
// outside.

/**
 * Adds a prepared instance as a replica of the given primary.
 *
 * @param primary primary/source instance
 * @param target target instance
 * @param channel_name The replication channel name
 * @param repl_options AR options, including repl credentials
 * @param fence_slave if true, sets SUPER_READ_ONLY at the slave end
 * @param dry_run boolean value to indicate if the command should do a dry run
 * or not
 * @param start_replica boolean value to indicate if the replication channel
 * should be started after configured or not
 */
void async_add_replica(mysqlshdk::mysql::IInstance *primary,
                       mysqlshdk::mysql::IInstance *target,
                       const std::string &channel_name,
                       const Async_replication_options &repl_options,
                       bool fence_slave, bool dry_run,
                       bool start_replica = true);

/**
 * Rejoins a replica to replicaset.
 *
 * More specifically, set the replicaset primary (in case it has changed)
 * and restart replication.
 *
 * NOTE: It is assumed that the replication user already exists (created when
 * adding the instance the first time), otherwise the instance needs to be
 * removed and added again.
 *
 * @param primary Instance object for the replicaset PRIMARY.
 * @param target Instance object for the target replica to rejoin.
 * @param channel_name The replication channel name
 * @param rpl_options Async_replication_options object with the replication
 *                    user credentials to use to rejoin the target instance.
 */
void async_rejoin_replica(mysqlshdk::mysql::IInstance *primary,
                          mysqlshdk::mysql::IInstance *target,
                          const std::string &channel_name,
                          const Async_replication_options &rpl_options);

/**
 * Removes a replica from the replicaset.
 */
void async_remove_replica(mysqlshdk::mysql::IInstance *target,
                          const std::string &channel_name, bool dry_run);

/**
 * Updates the replication credentials and re-starts the channel
 */
void async_update_replica_credentials(
    mysqlshdk::mysql::IInstance *target, const std::string &channel_name,
    const Async_replication_options &rpl_options, bool dry_run);

/**
 * Promotes a secondary to primary.
 */
void async_swap_primary(mysqlshdk::mysql::IInstance *current_primary,
                        mysqlshdk::mysql::IInstance *promoted,
                        const std::string &channel_name,
                        const Async_replication_options &repl_options,
                        shcore::Scoped_callback_list *undo_list, bool dry_run);

/**
 * Promotes a secondary to primary, when the primary is unavailable.
 */
void async_force_primary(mysqlshdk::mysql::IInstance *promoted,
                         const std::string &channel_name,
                         const Async_replication_options &repl_options,
                         bool dry_run);

/**
 * Revert the change performed by async_force_primary().
 *
 * @param promoted Instance that was going to be promoted.
 * @param channel_name The replication channel name
 * @param dry_run bool indicate if dry run is being performed.
 */
void undo_async_force_primary(mysqlshdk::mysql::IInstance *promoted,
                              const std::string &channel_name, bool dry_run);

void async_create_channel(mysqlshdk::mysql::IInstance *target,
                          mysqlshdk::mysql::IInstance *primary,
                          const std::string &channel_name,
                          const Async_replication_options &repl_options,
                          bool dry_run);

void async_change_primary(mysqlshdk::mysql::IInstance *target,
                          mysqlshdk::mysql::IInstance *primary,
                          const std::string &channel_name,
                          const Async_replication_options &repl_options,
                          bool start_replica, bool dry_run);

/**
 * Change the primary of one or more secondary instances.
 *
 * Reuses the same credentials as currently in use, unless given in repl_options
 */
void async_change_primary(
    mysqlshdk::mysql::IInstance *primary,
    const std::list<std::shared_ptr<Instance>> &secondaries,
    const std::string &channel_name,
    const Async_replication_options &repl_options,
    mysqlshdk::mysql::IInstance *old_primary,
    shcore::Scoped_callback_list *undo_list, bool dry_run);

void wait_apply_retrieved_trx(mysqlshdk::mysql::IInstance *instance,
                              std::chrono::seconds timeout);

void wait_all_apply_retrieved_trx(
    std::list<std::shared_ptr<Instance>> *out_instances,
    std::chrono::seconds timeout, bool invalidate_error_instances,
    std::vector<Instance_metadata> *out_instances_md,
    std::list<Instance_id> *out_invalidate_ids);

void fence_instance(mysqlshdk::mysql::IInstance *instance);

void unfence_instance(mysqlshdk::mysql::IInstance *instance, bool persist);

void reset_channel(const mysqlshdk::mysql::IInstance &instance,
                   const std::string &channel_name = "",
                   bool reset_credentials = false, bool dry_run = false);

enum class Stop_channel_result { NOT_EXIST, NOT_RUNNING, STOPPED };

Stop_channel_result stop_channel(const mysqlshdk::mysql::IInstance &instance,
                                 const std::string &channel_name, bool safe,
                                 bool dry_run);

void start_channel(const mysqlshdk::mysql::IInstance &instance,
                   const std::string &channel_name = "", bool dry_run = false);

void remove_channel(const mysqlshdk::mysql::IInstance &instance,
                    const std::string &channel_name, bool dry_run);

void add_managed_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance,
    const mysqlshdk::mysql::IInstance &source, const std::string &channel_name,
    const std::string &network_namespace, int64_t primary_weight = 80,
    int64_t secondary_weight = 60, bool dry_run = false);

void delete_managed_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance,
    const std::string &channel_name, bool dry_run = false);

void reset_managed_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance, bool dry_run = false);

void add_source_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance,
    const mysqlshdk::mysql::IInstance &source, const std::string &channel_name,
    const std::string &network_namespace, int64_t weight = 80,
    bool dry_run = false);

void add_source_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance, const std::string &host,
    int port, const std::string &channel_name,
    const std::string &network_namespace, int64_t weight = 80,
    bool dry_run = false);

void delete_source_connection_failover(
    const mysqlshdk::mysql::IInstance &target_instance, const std::string &host,
    int port, const std::string &channel_name,
    const std::string &network_namespace, bool dry_run = false);

bool get_managed_connection_failover_configuration(
    const mysqlshdk::mysql::IInstance &instance,
    Managed_async_channel *out_channel_info);

void drop_clone_recovery_user_nobinlog(
    mysqlshdk::mysql::IInstance *target_instance,
    const mysqlshdk::mysql::Auth_options &account,
    const std::string &account_host);

void cleanup_clone_recovery(mysqlshdk::mysql::IInstance *recipient,
                            const mysqlshdk::mysql::Auth_options &clone_user,
                            const std::string &account_host);
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_ASYNC_TOPOLOGY_H_
