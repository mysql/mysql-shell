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

#include "modules/adminapi/mod_dba_common.h"

#include <string>
#include <iterator>
#include <algorithm>
#include <utility>

#include "utils/utils_general.h"
#include "utils/utils_string.h"
#include "utils/utils_sqlstring.h"
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/db/connection_options.h"
//#include "mod_dba_instance.h"

namespace mysqlsh {
namespace dba {
std::map<std::string, FunctionAvailability> AdminAPI_function_availability = {
  // The Dba functions
  {"Dba.createCluster", {GRInstanceType::Standalone | GRInstanceType::GroupReplication, ReplicationQuorum::State::Any, ManagedInstance::State::Any}},
  {"Dba.getCluster", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Any, ManagedInstance::State::Any}},
  {"Dba.dropMetadataSchema", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal, ManagedInstance::State::OnlineRW}},
  {"Dba.rebootClusterFromCompleteOutage", {GRInstanceType::Any, ReplicationQuorum::State::Any, ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO}},

  // The Replicaset/Cluster functions
  {"Cluster.addInstance", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal, ManagedInstance::State::OnlineRW}},
  {"Cluster.removeInstance", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal, ManagedInstance::State::OnlineRW}},
  {"Cluster.rejoinInstance", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal, ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO}},
  {"Cluster.describe", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Any, ManagedInstance::State::Any}},
  {"Cluster.status", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Any, ManagedInstance::State::Any}},
  {"Cluster.dissolve", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal, ManagedInstance::State::OnlineRW}},
  {"Cluster.checkInstanceState", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal, ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO}},
  {"Cluster.rescan", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Normal, ManagedInstance::State::OnlineRW}},
  {"ReplicaSet.status", {GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Any, ManagedInstance::State::Any}},
  {"Cluster.forceQuorumUsingPartitionOf", {GRInstanceType::GroupReplication | GRInstanceType::InnoDBCluster, ReplicationQuorum::State::Any, ManagedInstance::State::OnlineRW | ManagedInstance::State::OnlineRO}}
};

namespace ManagedInstance {
std::string describe(State state) {
  std::string ret_val;
  switch (state) {
    case OnlineRW:
      ret_val = "Read/Write";
      break;
    case OnlineRO:
      ret_val = "Read Only";
      break;
    case Recovering:
      ret_val = "Recovering";
      break;
    case Unreachable:
      ret_val = "Unreachable";
      break;
    case Offline:
      ret_val = "Offline";
      break;
    case Error:
      ret_val = "Error";
      break;
    case Missing:
      ret_val = "(Missing)";
      break;
    case Any:
      assert(0);  // FIXME(anyone) avoid using enum as a bitmask
      break;
  }
  return ret_val;
}
};  // namespace ManagedInstance

namespace ReplicaSetStatus {
std::string describe(Status state) {
  std::string ret_val;

  switch (state) {
    case OK:
      ret_val = "OK";
      break;
    case OK_PARTIAL:
      ret_val = "OK_PARTIAL";
      break;
    case OK_NO_TOLERANCE:
      ret_val = "OK_NO_TOLERANCE";
      break;
    case NO_QUORUM:
      ret_val = "NO_QUORUM";
      break;
    case UNKNOWN:
      ret_val = "UNKNOWN";
      break;
  }
  return ret_val;
}
};  // namespace ReplicaSetStatus

std::string get_mysqlprovision_error_string(
    const shcore::Value::Array_type_ref &errors) {
  if (!errors)
    return "mysqlprovision error";
  std::vector<std::string> str_errors;

  if (errors) {
    for (auto error : *errors) {
      auto data = error.as_map();
      auto error_type = data->get_string("type");
      auto error_text = data->get_string("msg");

      str_errors.push_back(error_type + ": " + error_text);
    }
  } else {
    str_errors.push_back("Unexpected error calling mysqlprovision, "
      "see the shell log for more details");
  }

  return shcore::str_join(str_errors, "\n");
}

ReplicationGroupState check_function_preconditions(const std::string &class_name, const std::string& base_function_name, const std::string &function_name, const std::shared_ptr<MetadataStorage>& metadata) {
  // Retrieves the availability configuration for the given function
  std::string precondition_key = class_name + "." + base_function_name;
  assert(AdminAPI_function_availability.find(precondition_key) != AdminAPI_function_availability.end());
  FunctionAvailability availability = AdminAPI_function_availability.at(precondition_key);
  std::string error;
  ReplicationGroupState state;
  auto session = std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(metadata->get_session());

  // A classic session is required to perform any of the AdminAPI operations
  if (!session)
    error = "a Classic Session is required to perform this operation";
  else if (!session->is_open())
      error = "The session was closed. An open session is required to perform this operation";
  else{
    // Retrieves the instance configuration type from the perspective of the active session
    auto instance_type = get_gr_instance_type(session->connection());
    state.source_type = instance_type;

    // Validates availability based on the configuration state
    if (instance_type & availability.instance_config_state) {
      // If it is not a standalone instance, validates the instance state
      if (instance_type != GRInstanceType::Standalone) {
        // Retrieves the instance cluster statues from the perspective of the active session
        // (The Metadata Session)
        state = get_replication_group_state(session->connection(), instance_type);

        // Validates availability based on the instance status
        if (state.source_state & availability.instance_status) {
          // Finally validates availability based on the Cluster quorum
          if ((state.quorum & availability.cluster_status) == 0) {
            switch (state.quorum) {
              case ReplicationQuorum::Quorumless:
                error = "There is no quorum to perform the operation";
                break;
              case ReplicationQuorum::Dead:
                error = "Unable to perform the operation on a dead InnoDB cluster";
                break;
              default:
                // no-op
                break;
            }
          }
        } else {
          error = "This function is not available through a session";
          switch (state.source_state) {
            case ManagedInstance::OnlineRO:
              error += " to a read only instance";
              break;
            case ManagedInstance::Offline:
              error += " to an offline instance";
              break;
            case ManagedInstance::Error:
              error += " to an instance in error state";
              break;
            case ManagedInstance::Recovering:
              error += " to a recovering instance";
              break;
            case ManagedInstance::Unreachable:
              error += " to an unreachable instance";
              break;
            default:
              // no-op
              break;
          }
        }
      }
    } else {
      error = "This function is not available through a session";
      switch (instance_type) {
        case GRInstanceType::Standalone:
          error += " to a standalone instance";
          break;
        case GRInstanceType::GroupReplication:
          error += " to an instance belonging to an unmanaged replication group";
          break;
        case GRInstanceType::InnoDBCluster:
          error += " to an instance already in an InnoDB cluster";
          break;
        default:
          // no-op
          break;
      }
    }
  }

  if (!error.empty())
    throw shcore::Exception::runtime_error(function_name + ": " + error);

  // Returns the function availability in case further validation
  // is required, i.e. for warning processing
  return state;
}

const char *kMemberSSLModeAuto = "AUTO";
const char *kMemberSSLModeRequired = "REQUIRED";
const char *kMemberSSLModeDisabled = "DISABLED";
const std::set<std::string> kMemberSSLModeValues = {kMemberSSLModeAuto,
                                                    kMemberSSLModeDisabled,
                                                    kMemberSSLModeRequired};

void validate_ssl_instance_options(const shcore::Value::Map_type_ref &options) {
  // Validate use of SSL options for the cluster instance and issue an
  // exception if invalid.
  shcore::Argument_map opt_map(*options);
  if (opt_map.has_key("adoptFromGR")){
    bool adopt_from_gr = opt_map.bool_at("adoptFromGR");
    if (adopt_from_gr && (opt_map.has_key("memberSslMode")))
      throw shcore::Exception::argument_error(
          "Cannot use memberSslMode option if adoptFromGR is set to true.");
  }

  if (opt_map.has_key("memberSslMode")) {
    std::string ssl_mode = opt_map.string_at("memberSslMode");
    ssl_mode = shcore::str_upper(ssl_mode);
    if (kMemberSSLModeValues.count(ssl_mode) == 0) {
      std::string valid_values = shcore::str_join(kMemberSSLModeValues, ",");
      throw shcore::Exception::argument_error(
          "Invalid value for memberSslMode option. "
              "Supported values: " + valid_values + ".");
    }
  }
}

void validate_ip_whitelist_option(const shcore::Value::Map_type_ref &options) {
  // Validate the value of the ipWhitelist option an issue an exception
  // if invalid.
  shcore::Argument_map opt_map(*options);
  // Just a very simple validation is enough since the GCS layer on top of
  // which group replication runs has proper validation for the values provided
  // to the ipWhitelist option.
  if (opt_map.has_key("ipWhitelist")) {
    std::string ip_whitelist = options->get_string("ipWhitelist");
    ip_whitelist = shcore::str_strip(ip_whitelist);
    if (ip_whitelist.empty())
      throw shcore::Exception::argument_error(
          "Invalid value for ipWhitelist, string value cannot be empty.");
  }
}

/*
 * Check the existence of replication filters that do not exclude the
 * metadata from being replicated.
 * Raise an exception if invalid replication filters are found.
 */
void validate_replication_filters(mysqlsh::mysql::ClassicSession *session){
  shcore::Value status = get_master_status(session->connection());
  auto status_map = status.as_map();

  std::string binlog_do_db = status_map->get_string("BINLOG_DO_DB");

  if (!binlog_do_db.empty() &&
      binlog_do_db.find("mysql_innodb_cluster_metadata") == std::string::npos)
    throw shcore::Exception::runtime_error(
        "Invalid 'binlog-do-db' settings, metadata cannot be excluded. Remove "
            "binlog filters or include the 'mysql_innodb_cluster_metadata' "
            "database in the 'binlog-do-db' option.");

  std::string binlog_ignore_db = status_map->get_string("BINLOG_IGNORE_DB");

  if (binlog_ignore_db.find("mysql_innodb_cluster_metadata") != std::string::npos)
    throw shcore::Exception::runtime_error(
        "Invalid 'binlog-ignore-db' settings, metadata cannot be excluded. "
            "Remove binlog filters or the 'mysql_innodb_cluster_metadata' "
            "database from the 'binlog-ignore-db' option.");
}


/** Count how many accounts there are with the given username, excluding localhost

  This allows us to know whether there's a wildcarded account that the user may
  have created previously which we can use for management or whether the user
  has created multiple accounts, which would mean that they must know what they're
  doing and also that we can't tell which of these accounts should be validated.

  @return # of wildcarded accounts, # of other accounts
  */
std::pair<int,int> find_cluster_admin_accounts(std::shared_ptr<mysqlsh::mysql::ClassicSession> session,
    const std::string &admin_user, std::vector<std::string> *out_hosts) {

  shcore::sqlstring query;
  // look up the hosts the account is allowed from
  query = shcore::sqlstring(
      "SELECT DISTINCT grantee"
      " FROM information_schema.user_privileges"
      " WHERE grantee like ?", 0) << ((shcore::sqlstring("?", 0) << admin_user).str()+"@%");

  int local = 0;
  int w = 0;
  int nw = 0;
  auto result = session->execute_sql(query);
  if (result) {
    auto row = result->fetch_one();
    while (row) {
      std::string account = row->get_value(0).as_string();
      std::string user, host;
      shcore::split_account(account, &user, &host);
      assert(user == admin_user);

      if (out_hosts)
        out_hosts->push_back(host);
      // ignore localhost accounts
      if (host == "localhost" || host == "127.0.0.1") {
        local++;
      } else {
        if (host.find('%') != std::string::npos) {
          w++;
        } else {
          nw++;
        }
      }
      row = result->fetch_one();
    }
  }
  log_info("%s user has %i accounts with wildcard and %i without",
           admin_user.c_str(), w, nw + local);

  return std::make_pair(w, nw);
}


/*
 * Validate if the user specified for being the cluster admin has all of the
 * requirements:
 * 1) has a host wildcard other than 'localhost' or '127.0.0.1' or the one specified
 * 2) has the necessary privileges assigned
 */

// Global privs needed for managing cluster instances
static const std::set<std::string> k_global_privileges{
  "RELOAD", "SHUTDOWN", "PROCESS", "FILE",
  "SUPER", "REPLICATION SLAVE", "REPLICATION CLIENT",
  "CREATE USER"
};

// Schema privileges needed on the metadata schema
static const std::set<std::string> k_metadata_schema_privileges{
  "ALTER", "ALTER ROUTINE", "CREATE",
  "CREATE ROUTINE", "CREATE TEMPORARY TABLES",
  "CREATE VIEW", "DELETE", "DROP",
  "EVENT", "EXECUTE", "INDEX", "INSERT", "LOCK TABLES",
  "REFERENCES", "SELECT", "SHOW VIEW", "TRIGGER", "UPDATE"
};

// list of (schema, [privilege]) pairs, with the required privileges on each schema
static const std::map<std::string, std::set<std::string>> k_schema_grants{
  {"mysql_innodb_cluster_metadata", k_metadata_schema_privileges},
  {"performance_schema", {"SELECT"}},
  {"mysql", {"SELECT", "INSERT", "UPDATE", "DELETE"}} // need for mysql.plugin, mysql.user others
};

/** Check that the provided account has privileges to manage a cluster.
  */
bool validate_cluster_admin_user_privileges(std::shared_ptr<mysqlsh::mysql::ClassicSession> session,
    const std::string &admin_user, const std::string &admin_host) {

  shcore::sqlstring query;
  log_info("Validating account %s@%s...", admin_user.c_str(), admin_host.c_str());

  // check what global privileges we have
  query = shcore::sqlstring("SELECT privilege_type, is_grantable"
      " FROM information_schema.user_privileges"
      " WHERE grantee = ?", 0) << shcore::make_account(admin_user, admin_host);

  std::set<std::string> global_privs;

  auto result = session->execute_sql(query);
  if (result) {
    auto row = result->fetch_one();
    while (row) {
      global_privs.insert(row->get_value(0).as_string());
      if (row->get_value(1).as_string() == "NO") {
        log_info(" Privilege %s for %s@%s is not grantable",
                 row->get_value(0).as_string().c_str(),
                 admin_user.c_str(), admin_host.c_str());
        return false;
      }
      row = result->fetch_one();
    }
  }

  if (!std::includes(global_privs.begin(), global_privs.end(),
                    k_global_privileges.begin(), k_global_privileges.end())) {
    std::vector<std::string> diff;
    std::set_difference(k_global_privileges.begin(), k_global_privileges.end(),
                        global_privs.begin(), global_privs.end(),
                        std::inserter(diff, diff.begin()));
    log_debug("  missing some global privs");
    for (auto &i : diff)
      log_debug("  - %s", i.c_str());
    return false;
  }
  if (std::includes(global_privs.begin(), global_privs.end(),
      k_metadata_schema_privileges.begin(), k_metadata_schema_privileges.end())) {
    // if the account has global grants for all schema privs
    return true;
  }

  // now check if there are grants for the individual schemas
  query = shcore::sqlstring("SELECT table_schema, privilege_type, is_grantable"
      " FROM information_schema.schema_privileges"
      " WHERE grantee = ?", 0) << shcore::make_account(admin_user, admin_host);

  auto required_privileges(k_schema_grants);
  result = session->execute_sql(query);
  if (result) {
    auto row = result->fetch_one();
    while (row) {
      std::string schema = row->get_value(0).as_string();
      std::string priv = row->get_value(1).as_string();

      if (required_privileges.find(schema) != required_privileges.end()) {
        auto &privs(required_privileges[schema]);
        privs.erase(std::find(privs.begin(), privs.end(), priv));
      }
      if (row->get_value(2).as_string() == "NO") {
        log_info(" %s on %s for %s@%s is not grantable",
                 priv.c_str(), schema.c_str(), admin_user.c_str(), admin_host.c_str());
        return false;
      }
      row = result->fetch_one();
    }

    bool ok = true;
    for (auto &spriv : required_privileges) {
      if (!spriv.second.empty()) {
        log_info(" Account missing schema grants on %s:", spriv.first.c_str());
        for (auto &priv : spriv.second) {
          log_info("  - %s", priv.c_str());
        }
        ok = false;
      }
    }
    return ok;
  }
  return false;
}

static const char *k_admin_user_grants[] = {
  "GRANT RELOAD, SHUTDOWN, PROCESS, FILE, SUPER, REPLICATION SLAVE, REPLICATION CLIENT, CREATE USER ON *.*",
  "GRANT ALL PRIVILEGES ON mysql_innodb_cluster_metadata.*",
  "GRANT SELECT ON performance_schema.*",
  "GRANT SELECT, INSERT, UPDATE, DELETE ON mysql.*"
};

void create_cluster_admin_user(std::shared_ptr<mysqlsh::mysql::ClassicSession> session,
    const std::string &username, const std::string &password) {
#ifndef NDEBUG
  shcore::split_account(username, nullptr, nullptr);
#endif

  shcore::sqlstring query;
  // we need to check if the provided account has the privileges to
  // create the new account
  if (!mysqlsh::dba::validate_cluster_admin_user_privileges
      (session, session->get_user(), session->get_host())) {
    log_error(
        "Account '%s'@'%s' lacks the required privileges to create a new "
            "user account", session->get_user().c_str(),
        session->get_host().c_str());
    std::string msg;
    msg = "Account '" + session->get_user() + "'@'" + session->get_host()
        + "' does not have all the ";
    msg += "privileges to create an user for managing an InnoDB cluster.";
    throw std::runtime_error(msg);
  }
  session->execute_sql("SET sql_log_bin = 0");
  session->execute_sql("START TRANSACTION");
  try {
    log_info("Creating account %s", username.c_str());
    // Create the user
    {
      query = shcore::sqlstring(("CREATE USER " + username + " IDENTIFIED BY ?").c_str(), 0);
      query << password;
      query.done();
      session->execute_sql(query);
    }

    // Give the grants
    for (size_t i = 0; i < sizeof(k_admin_user_grants) / sizeof(char*); i++) {
      std::string grant(k_admin_user_grants[i]);
      grant += " TO " + username + " WITH GRANT OPTION";
      session->execute_sql(grant);
    }
    session->execute_sql("COMMIT");
    session->execute_sql("SET sql_log_bin = 1");
  } catch (...) {
    session->execute_sql("ROLLBACK");
    session->execute_sql("SET sql_log_bin = 1");
    throw;
  }
}

/**
 * Determines the value to use on the cluster for the SSL mode
 * based on:
 * - The instance having SSL available
 * - The instance having require_secure_transport ON
 * - The memberSslMode option given by the user
 *
 * Returns:
 *   REQUIRED if SSL is supported and member_ssl_mode is either AUTO or REQUIRED
 *   DISABLED if SSL is not supported and member_ssl_mode is either unspecified
 *            DISABLED or AUTO.
 *
 * Error:
 *   - If SSL is supported, require_secure_transport is ON and member_ssl_mode
 *     is DISABLED
 *   - If SSL mode is not supported and member_ssl_mode is REQUIRED
 */
std::string resolve_cluster_ssl_mode(mysqlsh::mysql::ClassicSession *session,
                                     const std::string& member_ssl_mode) {
  std::string ret_val;
  std::string have_ssl;

  get_server_variable(session->connection(), "global.have_ssl",
                      have_ssl);

  // The instance supports SSL
  if (!shcore::str_casecmp(have_ssl.c_str(),"YES")) {

    // memberSslMode is DISABLED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "DISABLED")) {
      int require_secure_transport = 0;
      get_server_variable(session->connection(),
                          "GLOBAL.require_secure_transport",
                          require_secure_transport);

      if (require_secure_transport) {
        throw shcore::Exception::argument_error("The instance "
          "'" + session->uri() + "' requires secure connections, to create the "
          "cluster either turn off require_secure_transport or use the "
          "memberSslMode option with 'REQUIRED' value.");
      }

      ret_val = dba::kMemberSSLModeDisabled;
    } else {
      // memberSslMode is either AUTO or REQUIRED
      ret_val = dba::kMemberSSLModeRequired;
    }

  // The instance does not support SSL
  } else {
    // memberSslMode is REQUIRED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "REQUIRED")) {
      throw shcore::Exception::argument_error("The instance "
        "'" + session->uri() + "' does not have SSL enabled, to create the "
        "cluster either use an instance with SSL enabled, remove the "
        "memberSslMode option or use it with any of 'AUTO' or 'DISABLED'.");
    }

    // memberSslMode is either not defined, DISABLED or AUTO
    ret_val = dba::kMemberSSLModeDisabled;
  }

  return ret_val;
}

/**
 * Determines the value to use on the instance for the SSL mode
 * configured for the cluster, based on:
 * - The instance having SSL available
 * - The instance having require_secure_transport ON
 * - The cluster SSL configuration
 * - The memberSslMode option given by the user
 *
 * Returns:
 *   REQUIRED if SSL is supported and member_ssl_mode is either AUTO or REQUIRED
 *            and the cluster uses SSL
 *   DISABLED if SSL is not supported and member_ssl_mode is either unspecified
 *            DISABLED or AUTO and the cluster has SSL disabled.
 *
 * Error:
 *   - Cluster has SSL enabled and member_ssl_mode is DISABLED or not specified
 *   - Cluster has SSL enabled and SSL is not supported on the instance
 *   - Cluster has SSL disabled and member_ssl_mode is REQUIRED
 *   - Cluster has SSL disabled and the instance has require_secure_transport ON
 */
std::string resolve_instance_ssl_mode(mysqlsh::mysql::ClassicSession *session,
                               mysqlsh::mysql::ClassicSession *psession,
                               const std::string& member_ssl_mode) {
  std::string ret_val;
  std::string gr_ssl_mode;

  // Checks how memberSslMode was configured on the cluster
  get_server_variable(psession->connection(),
                      "global.group_replication_ssl_mode",
                      gr_ssl_mode);

  // The cluster REQUIRES SSL
  if (!shcore::str_casecmp(gr_ssl_mode.c_str(), "REQUIRED")) {

    // memberSslMode is DISABLED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "DISABLED"))
      throw shcore::Exception::runtime_error(
          "The cluster has SSL (encryption) enabled. "
          "To add the instance '" + session->uri() + "' to the cluster either "
          "disable SSL on the cluster, remove the memberSslMode option or use "
          "it with any of 'AUTO' or 'REQUIRED'.");

    // Now checks if SSL is actually supported on the instance
    std::string have_ssl;

    get_server_variable(session->connection(), "global.have_ssl",
                        have_ssl);

    // SSL is not supported on the instance
    if (shcore::str_casecmp(have_ssl.c_str(), "YES")) {
      throw shcore::Exception::runtime_error(
          "Instance '" + session->uri() + "' does not support SSL and cannot "
          "join a cluster with SSL (encryption) enabled. "
          "Enable SSL support on the instance and try again, otherwise it can "
          "only be added to a cluster with SSL disabled.");
    }

    // memberSslMode is either AUTO or REQUIRED
    ret_val = dba::kMemberSSLModeRequired;

  // The cluster has SSL DISABLED
  } else if (!shcore::str_casecmp(gr_ssl_mode.c_str(), "DISABLED")) {

    // memberSslMode is REQUIRED
    if (!shcore::str_casecmp(member_ssl_mode.c_str(), "REQUIRED"))
      throw shcore::Exception::runtime_error(
          "The cluster has SSL (encryption) disabled. "
          "To add the instance '" + session->uri() + "' to the cluster either "
          "enable SSL on the cluster, remove the memberSslMode option or use "
          "it with any of 'AUTO' or 'DISABLED'.");

    // If the instance session requires SSL connections, then it can's
    // Join a cluster with SSL disabled
    int secure_transport_required = 0;
    get_server_variable(session->connection(),
                        "global.require_secure_transport",
                        secure_transport_required);
    if (secure_transport_required) {
      throw shcore::Exception::runtime_error(
          "The instance '" + session->uri() + "' is configured to require a "
          "secure transport but the cluster has SSL disabled. To add the "
          "instance to the cluster, either turn OFF the "
          "require_secure_transport option on the instance or enable SSL on "
          "the cluster.");
    }

    ret_val = dba::kMemberSSLModeDisabled;
  } else {
    // Only GR SSL mode "REQUIRED" and "DISABLED" are currently supported.
    throw shcore::Exception::runtime_error(
        "Unsupported Group Replication SSL Mode for the cluster: '"
            + gr_ssl_mode + "'. If the cluster was created using "
            "adoptFromGR:true make sure the group_replication_ssl_mode "
            "variable is set with a supported value (DISABLED or REQUIRED) "
            "for all cluster members.");
  }

  return ret_val;
}

/**
 * Gets the list of instances belonging to the GR group to which the metadata
 * belongs to
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @return a vector of strings with the list of member id's (member_id) of the
 *         instances belonging to the GR group
 */
std::vector<std::string> get_instances_gr(
    const std::shared_ptr<MetadataStorage> &metadata) {
  // Get the list of instances belonging to the GR group
  std::string query =
      "SELECT member_id FROM performance_schema.replication_group_members";

  auto result = metadata->execute_sql(query);
  auto members_ids = result->call("fetchAll", shcore::Argument_list());

  // build the instances array
  auto instances_gr = members_ids.as_array();
  std::vector<std::string> instances_gr_array;

  for (auto value : *instances_gr.get()) {
    auto row = value.as_object<mysqlsh::Row>();
    instances_gr_array.push_back(row->get_member(0).as_string());
  }

  return instances_gr_array;
}

/**
 * Gets the list of instances registered on the metadata for a specific
 * ReplicaSet
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @param rd_id the ReplicaSet id
 * @return a vector of strings with the list of instances uuid's
 *         (mysql_server_uuid) of the instances registered on the metadata, for
 *         the replicaset with the id rs_id
 */
std::vector<std::string> get_instances_md(
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id) {
  // Get the list of instances registered on the Metadata
  shcore::sqlstring query("SELECT mysql_server_uuid FROM " \
                          "mysql_innodb_cluster_metadata.instances " \
                          "WHERE replicaset_id = ?", 0);
  query << rs_id;
  query.done();

  auto result = metadata->execute_sql(query);
  auto mysql_server_uuids = result->call("fetchAll", shcore::Argument_list());

  // build the instances array
  auto instances_md = mysql_server_uuids.as_array();
  std::vector<std::string> instances_md_array;

  for (auto value : *instances_md.get()) {
    auto row = value.as_object<mysqlsh::Row>();
    instances_md_array.push_back(row->get_member(0).as_string());
  }

  return instances_md_array;
}

/**
 * Gets the list of instances belonging to the GR group which are not
 * registered on the metadata for a specific ReplicaSet.
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @param rd_id the ReplicaSet id
 * @return a vector of NewInstanceInfo with the list of instances found
 * as belonging to the GR group but not registered on the Metadata
 */
std::vector<NewInstanceInfo> get_newly_discovered_instances(
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id) {
  std::vector<std::string> instances_gr_array, instances_md_array;

  instances_gr_array = get_instances_gr(metadata);

  instances_md_array = get_instances_md(metadata, rs_id);

  // Check the differences between the two lists
  std::vector<std::string> new_members;

  // Sort the arrays in order to be able to use
  // std::set_difference()
  std::sort(instances_gr_array.begin(), instances_gr_array.end());
  std::sort(instances_md_array.begin(), instances_md_array.end());

  // Check if the instances_gr list has more members than the instances_md lists
  // Meaning that an instance was added to the GR group outside of the AdminAPI
  std::set_difference(instances_gr_array.begin(), instances_gr_array.end(),
                      instances_md_array.begin(), instances_md_array.end(),
                      std::inserter(new_members, new_members.begin()));

  std::vector<NewInstanceInfo> ret;
  for (auto i : new_members) {
    shcore::sqlstring query(
        "SELECT MEMBER_ID, MEMBER_HOST, MEMBER_PORT " \
        "FROM performance_schema.replication_group_members " \
        "WHERE MEMBER_ID = ?", 0);
    query << i;
    query.done();

    auto result = metadata->execute_sql(query);
    auto row = result->fetch_one();

    NewInstanceInfo info;
    info.member_id = row->get_value(0).as_string();
    info.host = row->get_value(1).as_string();
    info.port = row->get_value(2).as_int();

    ret.push_back(info);
  }

  return ret;
}

/**
 * Gets the list of instances registered on the Metadata, for a specific
 * ReplicaSet, which do not belong in the GR group.
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage
 * @param rd_id the ReplicaSet id
 * @return a vector of MissingInstanceInfo with the list of instances found
 * as registered on the Metadatada but not present in the GR group.
 */
std::vector<MissingInstanceInfo> get_unavailable_instances(
    const std::shared_ptr<MetadataStorage> &metadata, uint64_t rs_id) {
  std::vector<std::string> instances_gr_array, instances_md_array;

  instances_gr_array = get_instances_gr(metadata);
  instances_md_array = get_instances_md(metadata, rs_id);

  // Check the differences between the two lists
  std::vector<std::string> removed_members;

  // Sort the arrays in order to be able to use
  // std::set_difference()
  std::sort(instances_gr_array.begin(), instances_gr_array.end());
  std::sort(instances_md_array.begin(), instances_md_array.end());

  // Check if the instances_md list has more members than the instances_gr
  // lists. Meaning that an instance was removed from the GR group outside
  // of the AdminAPI
  std::set_difference(instances_md_array.begin(), instances_md_array.end(),
                      instances_gr_array.begin(), instances_gr_array.end(),
                      std::inserter(removed_members, removed_members.begin()));

  std::vector<MissingInstanceInfo> ret;
  for (auto i : removed_members) {
    shcore::sqlstring query(
        "SELECT mysql_server_uuid, instance_name, " \
        "JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) AS host " \
        "FROM mysql_innodb_cluster_metadata.instances " \
        "WHERE mysql_server_uuid = ?", 0);
    query << i;
    query.done();

    auto result = metadata->execute_sql(query);
    auto row = result->fetch_one();
    MissingInstanceInfo info;
    info.id = row->get_value(0).as_string();
    info.label = row->get_value(1).as_string();
    info.host = row->get_value(2).as_string();
    ret.push_back(info);
  }

  return ret;
}

/**
* Validate the given Cluster name.
* Cluster name must be non-empty and no greater than 40 characters long and
* start with an alphabetic or '_' character and just contain alphanumeric or
* the '_' character.
* @param name of the cluster
* @throws shcore::Exception if the name is not valid.
*/
void SHCORE_PUBLIC validate_cluster_name(const std::string &name) {
  bool valid = false;

  if (!name.empty()) {
    if (name.length() > 40) {
      throw shcore::Exception::argument_error(
        "The Cluster name can not be greater than 40 characters.");
    }

    std::locale locale;

    valid = std::isalpha(name[0], locale) || name[0] == '_';
    if (!valid) {
      throw shcore::Exception::argument_error(
        "The Cluster name can only start with an alphabetic or the '_' "
        "character.");
    }

    size_t index = 1;
    while (valid && index < name.size()) {
      valid = std::isalnum(name[index], locale) || name[index] == '_';
      if (!valid) {
        std::string msg =
          "The Cluster name can only contain alphanumerics or the '_' "
          "character."
          " Invalid character '";
        msg.append(&(name.at(index)), 1);
        msg.append("' found.");
        throw shcore::Exception::argument_error(msg);
      }
      index++;
    }
  }
  else {
    throw shcore::Exception::argument_error(
      "The Cluster name cannot be empty.");
  }

  return;
}

/**
* Validate the given label name.
* Cluster name must be non-empty and no greater than 256 characters long and
* start with an alphanumeric or '_' character and can only contain alphanumerics,
* Period ".", Underscore "_", Hyphen "-" or Colon ":" characters.
* @param name of the cluster
* @throws shcore::Exception if the name is not valid.
*/
void SHCORE_PUBLIC validate_label(const std::string &label) {
  bool valid = false;

  if (!label.empty()) {
    if (label.length() > 256) {
      throw shcore::Exception::argument_error(
        "The label can not be greater than 256 characters.");
    }

    std::locale locale;

    valid = std::isalnum(label[0], locale) || label[0] == '_';
    if (!valid) {
      throw shcore::Exception::argument_error(
        "The label can only start with an alphanumeric or the '_' "
        "character.");
    }

    size_t index = 1;
    while (valid && index < label.size()) {
      valid = std::isalnum(label[index], locale) || label[index] == '_' ||
        label[index] == '.' || label[index] == '-' || label[index] == ':';
      if (!valid) {
        std::string msg =
          "The label can only contain alphanumerics or the '_', '.', '-', "
          "':' characters. Invalid character '";
        msg.append(&(label.at(index)), 1);
        msg.append("' found.");
        throw shcore::Exception::argument_error(msg);
      }
      index++;
    }
  }
  else {
    throw shcore::Exception::argument_error("The label can not be empty.");
  }

  return;
}

/*
 * Gets Group Replication Group Name variable: group_replication_group_name
 *
 * @param session object which represents the session to the instance
 * @return string with the value of @@group_replication_group_name
 */
std::string get_gr_replicaset_group_name(
      mysqlsh::mysql::ClassicSession *session) {
  std::string group_name;

  std::string query("SELECT @@group_replication_group_name");

  // Any error will bubble up right away
  auto result = session->execute_sql(query);
  auto row = result->fetch_one();
  if (row) {
    group_name = row->get_value_as_string(0);
  }
  return group_name;
}

/**
 * Validates if the current session instance 'group_replication_group_name'
 * differs from the one registered in the corresponding ReplicaSet table of the
 * Metadata
 *
 * @param metadata metadata object which represents the session to the metadata
 *                 storage, i.e. the current instance session on this case
 * @param rd_id the ReplicaSet id
 * @return a boolean value indicating whether the replicaSet
 * 'group_replication_group_name' is the same as the one registered in the
 * corresponding replicaset in the Metadata
 */
bool validate_replicaset_group_name(
    const std::shared_ptr<MetadataStorage> &metadata,
    mysqlsh::mysql::ClassicSession *session,
    uint64_t rs_id) {
  std::string gr_group_name = get_gr_replicaset_group_name(session);
  std::string md_group_name = metadata->get_replicaset_group_name(rs_id);

  log_info("Group Replication 'group_name' value: %s", gr_group_name.c_str());
  log_info("Metadata 'group_name' value: %s", md_group_name.c_str());

  // If the value is NULL it means the instance does not belong to any GR group
  // so it can be considered valid
  if (gr_group_name == "NULL")
    return true;

  return (gr_group_name == md_group_name);
}

/*
 * Check for the value of super-read-only and any open sessions
 * to the instance and raise an exception if super_read_only is turned on.
 *
 * @param session object which represents the session to the instance
 * @param clear_read_only boolean value used to confirm that
 * super_read_only must be disabled
 *
 * @return a boolean value indicating the status of super_read_only
 */
bool validate_super_read_only(
    mysqlsh::mysql::ClassicSession *session, bool clear_read_only) {
  int super_read_only = 0;

  get_server_variable(session->connection(), "super_read_only",
                      super_read_only, false);

  if (super_read_only) {
    if (clear_read_only) {
      auto session_address =
          session->uri(mysqlshdk::db::uri::formats::only_transport());
      log_info("Disabling super_read_only on the instance '%s'",
               session_address.c_str());
      set_global_variable(session->connection(), "super_read_only", "OFF");
    } else {
      auto session_options = session->get_connection_options();

      auto session_address =
        session_options.as_uri(mysqlshdk::db::uri::formats::only_transport());

      std::string error_msg;

      error_msg =
        "The MySQL instance at '" + session_address + "' currently has "
        "the super_read_only system variable set to protect it from "
        "inadvertent updates from applications. You must first unset it to be "
        "able to perform any changes to this instance. "
        "For more information see: https://dev.mysql.com/doc/refman/en/"
        "server-system-variables.html#sysvar_super_read_only. ";

      // Get the list of open session to the instance
      std::vector<std::pair<std::string, int>> open_sessions;
      open_sessions = get_open_sessions(session->connection());

      if (!open_sessions.empty()) {
        error_msg +=
          "If you unset super_read_only you should consider closing the "
          "following: ";

        for (auto &value : open_sessions) {
          std::string account = value.first;
          int open_sessions = value.second;

          error_msg +=
            std::to_string(open_sessions) + " open session(s) of '" +
            account + "'. ";
        }
      }
      throw shcore::Exception::runtime_error(error_msg);
    }
  }
  return super_read_only;
}

}  // namespace dba
}  // namespace mysqlsh
