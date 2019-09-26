/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_COMMON_COMMON_H_
#define MODULES_ADMINAPI_COMMON_COMMON_H_

#include <functional>
#include <locale>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/preconditions.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "scripting/lang_base.h"
#include "scripting/types.h"

namespace mysqlsh {
namespace dba {

class MetadataStorage;

void SHCORE_PUBLIC parse_fully_qualified_cluster_name(
    const std::string &name, std::string *out_domain,
    std::string *out_partition, std::string *out_cluster_group);

void SHCORE_PUBLIC validate_cluster_name(const std::string &name,
                                         Cluster_type cluster_type);
void SHCORE_PUBLIC validate_label(const std::string &lavel);

// Default names
constexpr const char k_default_domain_partition_name[] = "default";
constexpr const char k_default_domain_name[] = "mydb";

// TODO(alfredo) - all server configuration constants and general functions
// should be moved to repl_config.h

// common keys for group replication and clone configuration options
constexpr const char kExitStateAction[] = "exitStateAction";
constexpr const char kGrExitStateAction[] =
    "group_replication_exit_state_action";
constexpr const char kGroupSeeds[] = "groupSeeds";
constexpr const char kGrGroupSeeds[] = "group_replication_group_seeds";
constexpr const char kIpWhitelist[] = "ipWhitelist";
constexpr const char kGrIpWhitelist[] = "group_replication_ip_whitelist";
constexpr const char kLocalAddress[] = "localAddress";
constexpr const char kGrLocalAddress[] = "group_replication_local_address";
constexpr const char kMemberWeight[] = "memberWeight";
constexpr const char kGrMemberWeight[] = "group_replication_member_weight";
constexpr const char kExpelTimeout[] = "expelTimeout";
constexpr const char kGrExpelTimeout[] =
    "group_replication_member_expel_timeout";
constexpr const char kFailoverConsistency[] = "failoverConsistency";
constexpr const char kGrFailoverConsistency[] = "group_replication_consistency";
constexpr const char kConsistency[] = "consistency";
constexpr const char kGrConsistency[] = "group_replication_consistency";
constexpr const char kGroupName[] = "groupName";
constexpr const char kGrGroupName[] = "group_replication_group_name";
constexpr const char kMemberSslMode[] = "memberSslMode";
constexpr const char kGrMemberSslMode[] = "group_replication_ssl_mode";
constexpr const char kClusterName[] = "clusterName";
constexpr const char kAutoRejoinTries[] = "autoRejoinTries";
constexpr const char kGrAutoRejoinTries[] =
    "group_replication_autorejoin_tries";
constexpr const char kDisableClone[] = "disableClone";
constexpr const char kGtidSetIsComplete[] = "gtidSetIsComplete";

// Group Replication configuration option availability regarding MySQL Server
// version
struct Option_availability {
  std::string option_variable;
  mysqlshdk::utils::Version support_in_80;
  mysqlshdk::utils::Version support_in_57;
};

/**
 * Map of the global ReplicaSet configuration options of the AdminAPI
 * <sysvar, name>
 */
const std::map<std::string, std::string> k_global_options{
    {kGroupName, kGrGroupName}, {kMemberSslMode, kGrMemberSslMode}};

/**
 * Map of the instance configuration options of the AdminAPI
 * <sysvar, name>
 */
const std::map<std::string, std::string> k_instance_options{
    {kExitStateAction, kGrExitStateAction}, {kGroupSeeds, kGrGroupSeeds},
    {kIpWhitelist, kGrIpWhitelist},         {kLocalAddress, kGrLocalAddress},
    {kMemberWeight, kGrMemberWeight},       {kExpelTimeout, kGrExpelTimeout},
    {kConsistency, kGrConsistency}};

/**
 * Map of the supported global ReplicaSet configuration options in the
 * AdminAPI <sysvar, name>
 */
const std::map<std::string, Option_availability>
    k_global_replicaset_supported_options{
        {kExitStateAction,
         {kGrExitStateAction, mysqlshdk::utils::Version("8.0.12"),
          mysqlshdk::utils::Version("5.7.24")}},
        {kMemberWeight,
         {kGrMemberWeight, mysqlshdk::utils::Version("8.0.11"),
          mysqlshdk::utils::Version("5.7.20")}},
        {kExpelTimeout,
         {kGrExpelTimeout, mysqlshdk::utils::Version("8.0.13"), {}}},
        {kFailoverConsistency,
         {kGrFailoverConsistency, mysqlshdk::utils::Version("8.0.14"), {}}},
        {kConsistency,
         {kGrConsistency, mysqlshdk::utils::Version("8.0.14"), {}}},
        {kAutoRejoinTries,
         {kGrAutoRejoinTries, mysqlshdk::utils::Version("8.0.16"), {}}}};

/**
 * Map of the supported global Cluster configuration options in the AdminAPI
 * <sysvar, name>
 */
// TODO(.) This and its dependencies must be moved out to a new user_options.h
const std::map<std::string, Option_availability>
    k_global_cluster_supported_options{
        {kClusterName, {"", {}, {}}},
        {kDisableClone, {"", mysqlshdk::utils::Version("8.0.17"), {}}}};

/**
 * Map of the supported instance configuration options in the AdminAPI
 * <sysvar, name>
 */
const std::map<std::string, Option_availability> k_instance_supported_options{
    {kExitStateAction,
     {kGrExitStateAction, mysqlshdk::utils::Version("8.0.12"),
      mysqlshdk::utils::Version("5.7.24")}},
    {kMemberWeight,
     {kGrMemberWeight, mysqlshdk::utils::Version("8.0.11"),
      mysqlshdk::utils::Version("5.7.20")}},
    {kAutoRejoinTries,
     {kGrAutoRejoinTries, mysqlshdk::utils::Version("8.0.16"), {}}}};

struct Instance_definition {
  int host_id;
  int replicaset_id;
  std::string uuid;
  std::string label;
  std::string role;
  std::string state;
  std::string endpoint;
  std::string xendpoint;
  std::string grendpoint;

  bool operator==(const Instance_definition &other) const = delete;
};

namespace ReplicaSetStatus {
enum Status { OK, OK_PARTIAL, OK_NO_TOLERANCE, NO_QUORUM, UNKNOWN };

std::string describe(Status state);
}  // namespace ReplicaSetStatus

enum class ConfigureInstanceAction {
  UPDATE_SERVER_AND_CONFIG_DYNAMIC,  // "server_update+config_update" - no
                                     // restart
  UPDATE_SERVER_AND_CONFIG_STATIC,   // "config_update+restart" - restart
  UPDATE_CONFIG,                     // "config_update" - no restart
  UPDATE_SERVER_DYNAMIC,             // "server_update" - no restart
  UPDATE_SERVER_STATIC,              // "server_update+restart" - restart
  RESTART,                           // "restart" - restart
  UNDEFINED
};

// Recovery progress style:
// - 0 no wait and no progress
// - 1 wait without progress info
// - 2 wait with textual info only
// - 3 wait with progressbar
enum class Recovery_progress_style { NOWAIT, NOINFO, TEXTUAL, PROGRESSBAR };

std::string get_mysqlprovision_error_string(
    const shcore::Value::Array_type_ref &errors);

extern const char *kMemberSSLModeAuto;
extern const char *kMemberSSLModeRequired;
extern const char *kMemberSSLModeDisabled;

/**
 * Check if a setting is supported on the target instance
 *
 * @param version MySQL version of the target instance
 * @param option the name of the option as defined in the AdminAPI.
 * @param options_map the map with the Version restrictions for the options
 * @return Boolean indicating if the target instance supports the option.
 *
 */
bool is_option_supported(
    const mysqlshdk::utils::Version &version, const std::string &option,
    const std::map<std::string, Option_availability> &options_map);

/*
 * Check the existence of replication filters and throws an exception if any
 * are found.
 *
 * GR does not support filters:
 * https://dev.mysql.com/doc/refman/8.0/en/replication-options-slave.html
 * Global replication filters cannot be used on a MySQL server instance that
 * is configured for Group Replication, because filtering transactions on some
 * servers would make the group unable to reach agreement on a consistent
 * state. Channel specific replication filters can be used on replication
 * channels that are not directly involved with Group Replication, such as
 * where a group member also acts as a replication slave to a master that is
 * outside the group. They cannot be used on the group_replication_applier or
 * group_replication_recovery channels.
 *
 * In AR, we also forbid filters in channels we manage.
 */
void validate_replication_filters(const mysqlshdk::mysql::IInstance &instance,
                                  Cluster_type cluster_type);

std::pair<int, int> find_cluster_admin_accounts(
    const mysqlshdk::mysql::IInstance &instance, const std::string &admin_user,
    std::vector<std::string> *out_hosts);
bool validate_cluster_admin_user_privileges(
    const mysqlshdk::mysql::IInstance &instance, const std::string &admin_user,
    const std::string &admin_host, std::string *validation_error);
void create_cluster_admin_user(const mysqlshdk::mysql::IInstance &instance,
                               const std::string &username,
                               const std::string &password);
std::string SHCORE_PUBLIC
resolve_cluster_ssl_mode(const mysqlshdk::mysql::IInstance &instance,
                         const std::string &member_ssl_mode);
std::string SHCORE_PUBLIC
resolve_instance_ssl_mode(const mysqlshdk::mysql::IInstance &instance,
                          const mysqlshdk::mysql::IInstance &pinstance,
                          const std::string &member_ssl_mode);

std::vector<NewInstanceInfo> get_newly_discovered_instances(
    const mysqlshdk::mysql::IInstance &group_server,
    const std::shared_ptr<MetadataStorage> &metadata, Cluster_id cluster_id);
std::vector<MissingInstanceInfo> get_unavailable_instances(
    const mysqlshdk::mysql::IInstance &group_server,
    const std::shared_ptr<MetadataStorage> &metadata, Cluster_id cluster_id);

bool SHCORE_PUBLIC validate_replicaset_group_name(
    const mysqlshdk::mysql::IInstance &instance, const std::string &group_name);

bool validate_super_read_only(const mysqlshdk::mysql::IInstance &instance,
                              mysqlshdk::utils::nullable<bool> clear_read_only,
                              bool interactive);

bool validate_instance_rejoinable(
    const mysqlshdk::mysql::IInstance &instance,
    const std::shared_ptr<MetadataStorage> &metadata, Cluster_id cluster_id);
bool is_sandbox(const mysqlshdk::mysql::IInstance &instance,
                std::string *cnfPath = nullptr);

// AdminAPI interactive handling specific methods
std::string prompt_cnf_path(const mysqlshdk::mysql::IInstance &instance);
std::string prompt_new_account_password();
int prompt_menu(const std::vector<std::string> &options, int defopt);
bool check_admin_account_access_restrictions(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::string &host, bool interactive);
bool prompt_create_usable_admin_account(const std::string &user,
                                        const std::string &host,
                                        std::string *out_create_account);
bool prompt_super_read_only(const mysqlshdk::mysql::IInstance &instance,
                            bool throw_on_error = false);
void dump_table(const std::vector<std::string> &column_names,
                const std::vector<std::string> &column_labels,
                shcore::Value::Array_type_ref documents);
ConfigureInstanceAction get_configure_instance_action(
    const shcore::Value::Map_type &opt_map);
void print_validation_results(const shcore::Value::Map_type_ref &result,
                              bool print_note = false);

/**
 * Validates the connection options.
 *
 * Checks if the given connection options are valid for use with AdminAPI.
 *
 * @param options Connection options to validate.
 * @param factory Factory function used to create the exception,
 *                shcore::Exception::argument_error by default.
 *
 * @throws shcore::Exception created by the 'factory' function if connection
 *                           options are not valid.
 */
void validate_connection_options(
    const Connection_options &options,
    std::function<shcore::Exception(const std::string &)> factory =
        shcore::Exception::argument_error);

/**
 * Auxiliary function to get the reported host address (used in the Metadata).
 *
 * This function tries to connect to the instance to get the reported host
 * information. If it fails to connect to the instance the connection address
 * is used (returned).
 *
 * @param cnx_opts Connection options of the target instance.
 * @param group_cnx_opts Connection options of the group session, used to set
 *                       the login credentials if needed.
 * @return the instance reported host address used in the metadata, or the
 *         given instance connection address if not able to connect to the
 *         instance.
 */
std::string get_report_host_address(
    const mysqlshdk::db::Connection_options &cnx_opts,
    const mysqlshdk::db::Connection_options &group_cnx_opts);

/**
 *  Creates a config object with a config_server_handler
 *
 * @param instance object pointing to the server to create the handler for.
 * @param srv_cfg_handler_name the name of the handler.
 * @param silent if true, print no warnings.
 * @return a unique pointer to a config object.
 */
std::unique_ptr<mysqlshdk::config::Config> create_server_config(
    mysqlshdk::mysql::IInstance *instance,
    const std::string &srv_cfg_handler_name, bool silent = false);

/** Adds a config file handler to the config object
 *
 * @param cfg pointer to a config object
 * @param handler_name name to be given to the config_file handler
 * @param mycnf_path path to the mycnf_path
 * @param output_mycnf_path path to the output_cnf_path
 */
void add_config_file_handler(mysqlshdk::config::Config *cfg,
                             const std::string handler_name,
                             const std::string &mycnf_path,
                             const std::string &output_mycnf_path);

/**
 * Resolves Group Replication local address.
 *
 * If the host part is not specified for the local_address then use the
 * instance report host (used by GR and the Metadata). If the port part is not
 * specified for the local_address then use the instance connection port value
 * * 10 + 1.
 *
 * The port cannot be greater than 65535. If it is when determined
 * automatically using the formula "<port> * 10 + 1" then a random port number
 * will be generated. If an invalid port value is specified (by the user) in
 * the local_address then an error is issued.
 *
 * @param local_address Nullable string with the input local address value to
 *                      resolve.
 * @param report_host String with the report host value of the instance (host
 *                    used by GR and the Metadata).
 * @param port integer with port used to connect to the instance.
 *
 * @throw std::runtime_error if the port specified by the user is invalid or
 * if it is already being used.
 */
std::string resolve_gr_local_address(
    const mysqlshdk::utils::nullable<std::string> &local_address,
    const std::string &report_host, int port);

struct Instance_gtid_info {
  std::string server;
  std::string gtid_executed;
};

/**
 * Return list of instances that could become a PRIMARY.
 *
 * Given a list of instances with GTID set data, returns all instances that
 * have the most transactions, except for those that have purged transactions
 * needed by others.
 *
 * An exception will be thrown if any instance with a conflicting transaction
 * set is found.
 *
 * @param server - a server in which GTID set operations will be evaluated.
 * The server doesn't need to be related to the candidates being checked.
 * @param gtid_info - a list of candidates instances with their
 * @@GTID_EXECUTED data.
 * @returns list of instances that could become a PRIMARY.
 */
std::vector<Instance_gtid_info> filter_primary_candidates(
    const mysqlshdk::mysql::IInstance &server,
    const std::vector<Instance_gtid_info> &gtid_info);

/**
 * Ensures that the replication channel is either ON or OFF without errors.
 *
 * @throw shcore::Exception with errors detected.
 */
void validate_replication_channel_startup_status(
    const mysqlshdk::mysql::Replication_channel &channel, bool *out_io_on,
    bool *out_applier_on);

/**
 * Waits for the given replication channel to start, throw exception if
 * an error is detected.
 *
 * @param instance the replica instance
 * @param channel_name name of the replication channel to check
 */
void check_replication_startup(const mysqlshdk::mysql::IInstance &instance,
                               const std::string &channel_name);

/*
 * Synchronize transactions on target instance.
 *
 * Wait for gtid_set to be applied on the specified target instance. Function
 * will monitor for replication errors on the named channel and throw an
 * exception if an error is detected.
 *
 * @param target_instance instance to wait for transaction to be applied.
 * @param gtid_set the transaction set to wait for
 * @param channel_name the name of the channel to monitor
 * @param timeout number of seconds to wait
 *
 * @throw RuntimeError if the timeout is reached when waiting for
 * transactions to be applied or replication errors are detected.
 */
bool wait_for_gtid_set_safe(const mysqlshdk::mysql::IInstance &target_instance,
                            const std::string &gtid_set,
                            const std::string &channel_name, int timeout);

void execute_script(const std::shared_ptr<Instance> &group_server,
                    const std::string &script, const std::string &context);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_COMMON_H_
