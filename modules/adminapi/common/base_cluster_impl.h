/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_BASE_CLUSTER_IMPL_H_
#define MODULES_ADMINAPI_COMMON_BASE_CLUSTER_IMPL_H_

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "scripting/types.h"

#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/mysql/lock_service.h"

namespace mysqlsh {
namespace dba {

struct Command_conditions;

// User provided option for telling us to assume that the cluster was created
// with a server where the full update history is reflected in its GTID set
inline constexpr std::string_view k_cluster_attribute_assume_gtid_set_complete{
    "opt_gtidSetIsComplete"};

// host (as in user@host) to use when creating managed replication accounts
inline constexpr std::string_view k_cluster_attribute_replication_allowed_host{
    "opt_replicationAllowedHost"};

inline constexpr std::string_view k_cluster_attribute_transaction_size_limit{
    "opt_transactionSizeLimit"};

// replication account authentication type
inline constexpr std::string_view k_cluster_attribute_member_auth_type{
    "opt_memberAuthType"};
// replication account certificate issuer
inline constexpr std::string_view k_cluster_attribute_cert_issuer{
    "opt_certIssuer"};

// replication channel options
inline constexpr std::string_view k_cluster_attribute_repl_connect_retry{
    "opt_replConnectRetry"};
inline constexpr std::string_view k_cluster_attribute_repl_retry_count{
    "opt_replRetryCount"};
inline constexpr std::string_view k_cluster_attribute_repl_heartbeat_period{
    "opt_replHeartbeatPeriod"};
inline constexpr std::string_view
    k_cluster_attribute_repl_compression_algorithms{
        "opt_replCompressionAlgorithms"};
inline constexpr std::string_view
    k_cluster_attribute_repl_zstd_compression_level{
        "opt_replZstdCompressionLevel"};
inline constexpr std::string_view k_cluster_attribute_repl_bind{"opt_replBind"};
inline constexpr std::string_view k_cluster_attribute_repl_network_namespace{
    "opt_replNetworkNamespace"};

// Cluster capabilities
inline constexpr const char k_cluster_capabilities[] = "capabilities";

class Base_cluster_impl {
 public:
  Base_cluster_impl(std::string cluster_name,
                    std::shared_ptr<Instance> group_server,
                    std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~Base_cluster_impl();

  virtual Cluster_type get_type() const = 0;

  virtual std::string get_topology_type() const = 0;

  bool get_gtid_set_is_complete() const;

  Cluster_id get_id() const { return m_id; }
  void set_id(Cluster_id id) { m_id = id; }

  std::string get_description() const { return m_description; }

  void set_description(const std::string &description) {
    m_description = description;
  }

  const std::string &get_name() const { return m_cluster_name; }

  void set_cluster_name(const std::string &name) { m_cluster_name = name; }
  const std::string &cluster_name() const { return m_cluster_name; }

  void set_target_server(const std::shared_ptr<Instance> &instance);

  std::shared_ptr<Instance> get_cluster_server() const {
    return m_cluster_server;
  }

  std::shared_ptr<Instance> get_primary_master() const {
    return m_primary_master;
  }

  virtual bool check_valid() const;

  void check_preconditions(const Command_conditions &conds);

  std::shared_ptr<MetadataStorage> get_metadata_storage() const {
    return m_metadata_storage;
  }

  const mysqlshdk::mysql::Auth_options &default_admin_credentials() const {
    return m_admin_credentials;
  }

  virtual std::tuple<mysqlsh::dba::Instance *, mysqlshdk::mysql::Lock_scoped>
  acquire_primary_locked(
      [[maybe_unused]] mysqlshdk::mysql::Lock_mode mode,
      [[maybe_unused]] std::string_view skip_lock_uuid = "") {
    assert(!"Method not implemented");
    throw std::logic_error("Method not implemented");
  }

  virtual mysqlsh::dba::Instance *acquire_primary(
      bool primary_required = true, bool check_primary_status = false) = 0;

  virtual void release_primary() = 0;

  // Locking methods (only cluster and clusterset supports them yet)
  virtual mysqlshdk::mysql::Lock_scoped get_lock_shared(
      [[maybe_unused]] std::chrono::seconds timeout = {}) {
    return {};
  }

  virtual mysqlshdk::mysql::Lock_scoped get_lock_exclusive(
      [[maybe_unused]] std::chrono::seconds timeout = {}) {
    return {};
  }

  virtual void disconnect();

  virtual shcore::Value list_routers(bool only_upgrade_required);

  virtual void setup_admin_account(const std::string &username,
                                   const std::string &host,
                                   const Setup_account_options &options);

  virtual void setup_router_account(const std::string &username,
                                    const std::string &host,
                                    const Setup_account_options &options);

  virtual void remove_router_metadata(const std::string &router);

  void set_instance_tag(const std::string &instance_def,
                        const std::string &option, const shcore::Value &value);

  void set_cluster_tag(const std::string &option, const shcore::Value &value);

  void set_router_tag(const std::string &router, const std::string &option,
                      const shcore::Value &value);

  void set_instance_option(const std::string &instance_def,
                           const std::string &option,
                           const shcore::Value &value);

  void set_option(const std::string &option, const shcore::Value &value);

  Replication_auth_type query_cluster_auth_type() const;
  std::string query_cluster_auth_cert_issuer() const;

  std::string query_cluster_instance_auth_cert_subject(
      std::string_view instance_uuid) const;
  std::string query_cluster_instance_auth_cert_subject(
      const mysqlshdk::mysql::IInstance &instance) const {
    return query_cluster_instance_auth_cert_subject(instance.get_uuid());
  }

  virtual void set_routing_option(const std::string &router,
                                  const std::string &option,
                                  const shcore::Value &value);
  virtual shcore::Dictionary_t routing_options(const std::string &router);

  virtual shcore::Dictionary_t router_options(
      const shcore::Option_pack_ref<Router_options_options> &options);

  virtual shcore::Value execute(
      const std::string &cmd, const shcore::Value &instances,
      const shcore::Option_pack_ref<Execute_options> &options) = 0;

  /**
   * Get the tags for a specific Cluster/ReplicaSet
   *
   * This function gets the tags for a Cluster/ReplicaSet and for its members
   *
   * @return a shcore::Value containing a dictionary object with the command
   * output
   */
  shcore::Value get_cluster_tags() const;

 public:
  /*
   * Synchronize transactions on target instance.
   *
   * Wait for all current cluster transactions to be applied on the specified
   * target instance. Function will monitor for replication errors on the named
   * channel and throw an exception if an error is detected.
   *
   * @param target_instance instance to wait for transaction to be applied.
   * @param channel_name the name of the replication channel to monitor for I/O
   * and SQL errors
   * @param timeout number of seconds to wait
   * @param only_received if true, only wait for already received transactions
   * to be applied
   *
   * @throw RuntimeError if the timeout is reached when waiting for
   * transactions to be applied or replication errors are detected.
   */
  void sync_transactions(const mysqlshdk::mysql::IInstance &target_instance,
                         const std::string &channel_name, int timeout,
                         bool only_received = false, bool silent = false) const;

  void sync_transactions(const mysqlshdk::mysql::IInstance &target_instance,
                         Instance_type instance_type, int timeout,
                         bool only_received = false, bool silent = false) const;

  /**
   * Connect to the given instance specification given, while validating its
   * syntax.
   *
   * @param instance_def the instance to connect to, as a host:port string.
   * @param print_error boolean value to indicate whether an error shall be
   * printed or not
   * @param allow_account_override if true, allows the user to override the
   * account, otherwise they must match the cluster session
   *
   * A URL is allowed, if it matches that of m_cluster_server.
   * @return instance object owned by ipool
   */
  std::shared_ptr<Instance> connect_target_instance(
      const std::string &instance_def, bool print_error = true,
      bool allow_account_override = false);

  std::shared_ptr<Instance> connect_target_instance(
      const mysqlshdk::db::Connection_options &instance_def,
      bool print_error = true, bool allow_account_override = false);

  virtual std::vector<Instance_metadata> get_instances_from_metadata()
      const = 0;

  void create_clone_recovery_user_nobinlog(
      mysqlshdk::mysql::IInstance *target_instance,
      const mysqlshdk::mysql::Auth_options &donor_account,
      const std::string &account_host, const std::string &account_cert_issuer,
      const std::string &account_cert_subject, bool dry_run);

  void handle_clone_provisioning(
      const std::shared_ptr<mysqlsh::dba::Instance> &recipient,
      const std::shared_ptr<mysqlsh::dba::Instance> &donor,
      const Async_replication_options &ar_options,
      const std::string &repl_account_host, const std::string &cert_issuer,
      const std::string &cert_subject,
      const Recovery_progress_style &progress_style, int sync_timeout,
      bool dry_run);

  static void check_compatible_clone_donor(
      const mysqlshdk::mysql::IInstance &donor,
      const mysqlshdk::mysql::IInstance &recipient);

  virtual Clone_availability check_clone_availablity(
      const mysqlshdk::mysql::IInstance &donor,
      const mysqlshdk::mysql::IInstance &recipient) const;

  static void check_replication_version_compatibility(
      const std::string &source_descr, const std::string &replica_descr,
      const mysqlshdk::utils::Version &source_version,
      const mysqlshdk::utils::Version &replica_version, bool is_potential,
      bool log_only, shcore::Array_t *instance_errors);

  static void check_compatible_replication_sources(
      const mysqlshdk::mysql::IInstance &source,
      const mysqlshdk::mysql::IInstance &replica,
      const std::list<std::shared_ptr<mysqlshdk::mysql::IInstance>>
          *potential_sources = nullptr,
      bool potential = false);

  static void add_incompatible_replication_source_issues(
      const mysqlshdk::mysql::IInstance &source,
      const mysqlshdk::mysql::IInstance &replica, bool potential,
      shcore::Array_t &instance_errors);

 protected:
  Cluster_id m_id;
  std::string m_cluster_name;
  std::string m_description;

  // Session to the primary member of the Cluster/ReplicaSet upon the
  // initialization of the corresponding objects
  std::shared_ptr<Instance> m_cluster_server;

  // Session to the primary instance
  // NOTE: In standalone Clusters it's the primary member of the Cluster, but if
  // the Cluster belongs to a ClusterSet, it's the primary member of the primary
  // Cluster
  std::shared_ptr<Instance> m_primary_master;

  std::shared_ptr<MetadataStorage> m_metadata_storage;

  mysqlshdk::mysql::Auth_options m_admin_credentials;

  enum class Setup_account_type { ADMIN, ROUTER };

  void setup_account_common(const std::string &username,
                            const std::string &host,
                            const Setup_account_options &options,
                            const Setup_account_type &type);

  virtual void _set_instance_option(const std::string &instance_def,
                                    const std::string &option,
                                    const shcore::Value &value) = 0;

  virtual void _set_option(const std::string &option,
                           const shcore::Value &value) = 0;

  void target_server_invalidated();

  /**
   * Does simple validation on the option provided to setOption and
   * setInstanceOption methods and splits the option into a pair: namespace,
   * option_name. If dealing with a built-in tag, it also converts the value to
   * the expected type. If the namespace is empty, it means the option has no
   * namespace.
   * @param option the option parameter of the setOption and setInstance option
   * methods.
   * @param value the value that was provided for the option. We use it to
   * validate if built-in tags are of the expected type or can be converted to
   * it.
   * @param built_in_tags allowed tags+types that start with _
   * @return a tuple with strings and value: namespace, option_name, value.
   * @throws argumentError if the format of the option is invalid, or using an
   * unsupported namespace.
   */
  std::tuple<std::string, std::string, shcore::Value>
  validate_set_option_namespace(const std::string &option,
                                const shcore::Value &value,
                                const built_in_tags_map_t &built_in_tags) const;

 private:
  void disconnect_internal();
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_BASE_CLUSTER_IMPL_H_
