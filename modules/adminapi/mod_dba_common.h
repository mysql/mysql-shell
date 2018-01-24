/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/innodbcluster/cluster.h"

namespace mysqlsh {
namespace dba {

void SHCORE_PUBLIC validate_cluster_name(const std::string& name);
void SHCORE_PUBLIC validate_label(const std::string &lavel);

class MetadataStorage;

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

  bool operator==(const Instance_definition& other) const {
    return host_id == other.host_id &&
           replicaset_id == other.replicaset_id &&
           uuid == other.uuid &&
           label == other.label &&
           role == other.role &&
           state == other.state &&
           endpoint == other.endpoint &&
           xendpoint == other.xendpoint &&
           grendpoint == other.grendpoint;
  }
};

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
  StandaloneWithMetadata = 1 << 3
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
    std::shared_ptr<mysqlshdk::db::ISession> group_session);

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
void validate_local_address_option(const shcore::Value::Map_type_ref &options);
void validate_group_seeds_option(const shcore::Value::Map_type_ref &options);
void validate_group_name_option(const shcore::Value::Map_type_ref &options);
void validate_replication_filters(
    std::shared_ptr<mysqlshdk::db::ISession> session);
std::pair<int, int> find_cluster_admin_accounts(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &admin_user, std::vector<std::string> *out_hosts);
bool validate_cluster_admin_user_privileges(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &admin_user, const std::string &admin_host);
void create_cluster_admin_user(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &username, const std::string &password);
std::string SHCORE_PUBLIC resolve_cluster_ssl_mode(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string& member_ssl_mode);
std::string SHCORE_PUBLIC resolve_instance_ssl_mode(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    std::shared_ptr<mysqlshdk::db::ISession> psession,
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
    std::shared_ptr<mysqlshdk::db::ISession> session);
bool SHCORE_PUBLIC validate_replicaset_group_name(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &group_name);
bool validate_super_read_only(
    std::shared_ptr<mysqlshdk::db::ISession> session, bool clear_read_only);
bool validate_instance_rejoinable(
    std::shared_ptr<mysqlshdk::db::ISession> instance_session,
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id);
void validate_host_ip(const std::string &hostname);

inline void translate_cluster_exception(std::string operation) {
  if (!operation.empty())
    operation.append(": ");
  try {
    throw;
  } catch (mysqlshdk::innodbcluster::cluster_error &e) {
    throw shcore::Exception::runtime_error(
        operation + e.format());
  } catch (shcore::Exception &e) {
    auto error = e.error();
    (*error)["message"] = shcore::Value(operation + e.what());
    throw shcore::Exception(error);
  } catch (mysqlshdk::db::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  } catch (std::runtime_error &e) {
    throw shcore::Exception::runtime_error(operation + e.what());
  } catch (std::logic_error &e) {
    throw shcore::Exception::logic_error(operation + e.what());
  } catch (...) {
    throw;
  }
}

#define CATCH_AND_TRANSLATE_CLUSTER_EXCEPTION(operation)  \
  catch (...) {                                           \
    mysqlsh::dba::translate_cluster_exception(operation); \
    throw;                                                \
  }

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_COMMON_H_
