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

#include <functional>
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
#include "modules/adminapi/dba/preconditions.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/innodbcluster/cluster.h"
#include "mysqlshdk/include/shellcore/console.h"

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

enum class ConfigureInstanceAction {
  UPDATE_SERVER_AND_CONFIG_DYNAMIC,  // "server_update+config_update" - no
                                     // restart
  UPDATE_SERVER_AND_CONFIG_STATIC,   // "config_update+restart" - restart
  UPDATE_CONFIG,                     // "config_update" - no restart
  UPDATE_SERVER_DYNAMIC,             // "server_update" - no restart
  UPDATE_SERVER_STATIC,              // "restart" - restart
  UNDEFINED
};

std::string get_mysqlprovision_error_string(
    const shcore::Value::Array_type_ref &errors);

extern const char *kMemberSSLModeAuto;
extern const char *kMemberSSLModeRequired;
extern const char *kMemberSSLModeDisabled;
extern const std::set<std::string> kMemberSSLModeValues;

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
    const std::string &admin_user, const std::string &admin_host,
    std::string *validation_error);
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
    std::shared_ptr<mysqlshdk::db::ISession> session, bool clear_read_only,
    std::shared_ptr<mysqlsh::IConsole> console);
bool validate_instance_rejoinable(
    std::shared_ptr<mysqlshdk::db::ISession> instance_session,
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id);
void validate_host_ip(const std::string &hostname);
bool is_sandbox(const mysqlshdk::mysql::IInstance &instance,
                std::string *cnfPath = nullptr);
std::string get_canonical_instance_address(
    std::shared_ptr<mysqlshdk::db::ISession> session);

// AdminAPI interactive handling specific methods
std::string prompt_cnf_path(
    const mysqlshdk::mysql::IInstance &instance,
    std::shared_ptr<mysqlsh::IConsole> console_handler = nullptr);
std::string prompt_new_account_password(
    std::shared_ptr<mysqlsh::IConsole> console_handler);
int prompt_menu(
    const std::vector<std::string> &options, int defopt,
    std::shared_ptr<mysqlsh::IConsole> console_handler);
bool check_admin_account_access_restrictions(
    const mysqlshdk::mysql::IInstance &instance, const std::string &user,
    const std::string &host,
    std::shared_ptr<mysqlsh::IConsole> console_handler);
bool prompt_create_usable_admin_account(
    const std::string &user, const std::string &host,
    std::string *out_create_account,
    std::shared_ptr<mysqlsh::IConsole> console_handler);
bool prompt_super_read_only(
    const mysqlshdk::mysql::IInstance &instance,
    std::shared_ptr<mysqlsh::IConsole> console_handler,
    bool throw_on_error = false);
void dump_table(
    const std::vector<std::string> &column_names,
    const std::vector<std::string> &column_labels,
    shcore::Value::Array_type_ref documents,
    std::shared_ptr<mysqlsh::IConsole> console_handler);
ConfigureInstanceAction get_configure_instance_action(
    const shcore::Value::Map_type &opt_map);
void print_validation_results(
    const shcore::Value::Map_type_ref &result,
    std::shared_ptr<mysqlsh::IConsole> console_handler,
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
void validate_connection_options(const Connection_options &options,
    std::function<shcore::Exception(const std::string &)> factory =
        shcore::Exception::argument_error);

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
