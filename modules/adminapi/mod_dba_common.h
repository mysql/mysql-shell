/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MODULES_ADMINAPI_MOD_DBA_COMMON_H_
#define MODULES_ADMINAPI_MOD_DBA_COMMON_H_

#include <locale>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <utility>

#include "scripting/types.h"
#include "scripting/lang_base.h"
#include "modules/mod_mysql_session.h"
#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "modules/mod_utils.h"

namespace mysqlsh {
namespace dba {

void SHCORE_PUBLIC validate_cluster_name(const std::string& name);
void SHCORE_PUBLIC validate_label(const std::string &lavel);

class MetadataStorage;

// Note that this structure may be initialized using initializer
// lists, so the order of hte fields is very important
struct FunctionAvailability {
  int instance_config_state;
  int cluster_status;
  int instance_status;
};

enum GRInstanceType {
  Standalone = 1 << 0,
  GroupReplication = 1 << 1,
  InnoDBCluster = 1 << 2,
  Any = Standalone | GroupReplication | InnoDBCluster
};

enum class SlaveReplicationState {
  New,
  Recoverable,
  Diverged,
  Irrecoverable
};

struct NewInstanceInfo {
  std::string member_id;
  std::string host;
  int port;
};

struct MissingInstanceInfo {
  std::string id;
  std::string label;
  std::string host;
};

namespace ManagedInstance {
enum State {
  OnlineRW = 1 << 0,
  OnlineRO = 1 << 1,
  Recovering = 1 << 2,
  Unreachable = 1 << 3,
  Offline = 1 << 4,
  Error = 1 << 5,
  Missing = 1 << 6,
  Any = OnlineRO | OnlineRW | Recovering | Unreachable | Offline | Error |
      Missing
};

std::string describe(State state);
};  // namespace ManagedInstance

namespace ReplicationQuorum {
enum State {
  Normal = 1 << 0,
  Quorumless = 1 << 1,
  Dead = 1 << 2,
  Any = Normal | Quorumless | Dead
};
}

struct ReplicationGroupState {
  // The state of the cluster from the quorum point of view
  ReplicationQuorum::State quorum;

  // The UIUD of the master instance
  std::string master;

  // The UUID of the instance from which the data was consulted
  std::string source;

  // The configuration type of the instance from which the data was consulted
  GRInstanceType source_type;

  // The state of the instance from which the data was consulted
  ManagedInstance::State source_state;
};

namespace ReplicaSetStatus {
enum Status {
  OK,
  OK_PARTIAL,
  OK_NO_TOLERANCE,
  NO_QUORUM,
  UNKNOWN
};

std::string describe(Status state);
};  // namespace ReplicaSetStatus

std::string get_mysqlprovision_error_string(
    const shcore::Value::Array_type_ref &errors);
ReplicationGroupState check_function_preconditions(
    const std::string &class_name, const std::string &base_function_name,
    const std::string &function_name,
    const std::shared_ptr<MetadataStorage> &metadata);

extern const char *kMemberSSLModeAuto;
extern const char *kMemberSSLModeRequired;
extern const char *kMemberSSLModeDisabled;
extern const std::set<std::string> kMemberSSLModeValues;
extern const std::set<std::string> k_global_privileges;
extern const std::set<std::string> k_metadata_schema_privileges;
extern const std::set<std::string> k_mysql_schema_privileges;
extern const std::map<std::string, std::set<std::string>> k_schema_grants;

void validate_ssl_instance_options(const shcore::Value::Map_type_ref &options);
void validate_ip_whitelist_option(const shcore::Value::Map_type_ref &options);
void validate_replication_filters(mysqlsh::mysql::ClassicSession *session);
std::pair<int, int> find_cluster_admin_accounts(
    std::shared_ptr<mysqlsh::mysql::ClassicSession> session,
    const std::string &admin_user, std::vector<std::string> *out_hosts);
bool validate_cluster_admin_user_privileges(
    std::shared_ptr<mysqlsh::mysql::ClassicSession> session,
    const std::string &admin_user, const std::string &admin_host);
void create_cluster_admin_user(
    std::shared_ptr<mysqlsh::mysql::ClassicSession> session,
    const std::string &username, const std::string &password);
std::string SHCORE_PUBLIC resolve_cluster_ssl_mode(
    mysqlsh::mysql::ClassicSession *session,
    const std::string& member_ssl_mode);
std::string SHCORE_PUBLIC resolve_instance_ssl_mode(
    mysqlsh::mysql::ClassicSession *session,
    mysqlsh::mysql::ClassicSession *psession,
    const std::string& member_ssl_mode);
std::vector<std::string> get_instances_gr(
    const std::shared_ptr<MetadataStorage> &metadata);
std::vector<std::string> get_instances_md(
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id);
std::vector<NewInstanceInfo> get_newly_discovered_instances(
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id);
std::vector<MissingInstanceInfo> get_unavailable_instances(
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id);
std::string SHCORE_PUBLIC get_gr_replicaset_group_name(
    mysqlsh::mysql::ClassicSession *session);
bool SHCORE_PUBLIC validate_replicaset_group_name(
    const std::shared_ptr<MetadataStorage> &metadata,
    mysqlsh::mysql::ClassicSession *session, uint64_t rs_id);
bool validate_super_read_only(
    mysqlsh::mysql::ClassicSession *session, bool clear_read_only);
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_COMMON_H_
