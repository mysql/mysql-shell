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
#include <boost/algorithm/string.hpp>

#include "modules/adminapi/mod_dba_common.h"
#include "utils/utils_general.h"
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
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

const std::set<std::string> _instance_options {"host", "port", "user", "dbUser", "password", "dbPassword", "socket", "sslCa", "sslCert", "sslKey"};

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

void resolve_instance_credentials(const shcore::Value::Map_type_ref& options, shcore::Interpreter_delegate* delegate) {
  // Sets a default user if not specified
  if (!options->has_key("user") && !options->has_key("dbUser"))
    (*options)["dbUser"] = shcore::Value("root");

  if (!options->has_key("password") && !options->has_key("dbPassword")) {
    if (delegate) {
      std::string answer;

      std::string prompt = "Please provide the password for '" + build_connection_string(options, false) + "': ";
      if (delegate->password(delegate->user_data, prompt.c_str(), answer))
        (*options)["password"] = shcore::Value(answer);
    } else
      throw shcore::Exception::argument_error("Missing password for '" + build_connection_string(options, false) + "'");
  }
}

// Parses the argument list to retrieve an instance definition from it
// It handles loading the pass
shcore::Value::Map_type_ref get_instance_options_map(const shcore::Argument_list &args,
                                                     PasswordFormat::Format format) {
  shcore::Value::Map_type_ref options;

  // This validation will be added here but should not be needed if the function is used properly
  // callers are responsible for validating if the number of arguments is correct
  args.ensure_at_least(1, "get_instance_options_map");

  // Attempts getting an instance object
  //auto instance = args.object_at<mysqlsh::dba::Instance>(0);
  //if (instance) {
  //  options = shcore::get_connection_data(instance->get_uri(), false);
  //  (*options)["password"] = shcore::Value(instance->get_password());
  //}

  // Not an instance, tries as URI string
  //else
  if (args[0].type == shcore::String) {
    try {
      options = shcore::get_connection_data(args.string_at(0), false);
    }
    catch (std::exception &e) {
      std::string error(e.what());
      throw shcore::Exception::argument_error("Invalid instance definition, expected a URI. "
                                              "Error: " + error);
    }
  }

  // Finally as a dictionary
  else if (args[0].type == shcore::Map)
    options = args.map_at(0);
  else
    throw shcore::Exception::argument_error("Invalid connection options, expected either a URI or a Dictionary");

  if (options->size() == 0)
    throw shcore::Exception::argument_error("Connection definition is empty");

  // If password override is allowed then we review the second
  // argument if any
  if (format != PasswordFormat::NONE && args.size() > 1) {
    bool override_pwd = false;
    std::string new_password;

    if (format == PasswordFormat::OPTIONS) {
      auto other_options = args.map_at(1);
      shcore::Argument_map other_args(*other_options);

      if (other_args.has_key("password")) {
        new_password = other_args.string_at("password");
        override_pwd = true;
      } else if (other_args.has_key("dbPassword")) {
        new_password = other_args.string_at("dbPassword");
        override_pwd = true;
      }
    } else if (format == PasswordFormat::STRING) {
      new_password = args.string_at(1);
      override_pwd = true;
    }

    if (override_pwd) {
      if (options->has_key("dbPassword"))
        (*options)["dbPassword"] = shcore::Value(new_password);
      else
        (*options)["password"] = shcore::Value(new_password);
    }
  }

  return options;
}

std::string get_mysqlprovision_error_string(const shcore::Value::Array_type_ref &errors) {
  std::vector<std::string> str_errors;

  for (auto error : *errors) {
    auto data = error.as_map();
    auto error_type = data->get_string("type");
    auto error_text = data->get_string("msg");

    str_errors.push_back(error_type + ": " + error_text);
  }

  return shcore::join_strings(str_errors, "\n");
}

ReplicationGroupState check_function_preconditions(const std::string &class_name, const std::string& base_function_name, const std::string &function_name, const std::shared_ptr<MetadataStorage>& metadata) {
  // Retrieves the availability configuration for the given function
  std::string precondition_key = class_name + "." + base_function_name;
  assert(AdminAPI_function_availability.find(precondition_key) != AdminAPI_function_availability.end());
  FunctionAvailability availability = AdminAPI_function_availability.at(precondition_key);

  std::string error;
  ReplicationGroupState state;

  // A classic session is required to perform any of the AdminAPI operations
  auto session = std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(metadata->get_dba()->get_active_session());
  if (!session)
    error = "a Classic Session is required to perform this operation";
  else {
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
    boost::to_upper(ssl_mode);
    if (kMemberSSLModeValues.count(ssl_mode) == 0) {
      std::string valid_values = boost::join(kMemberSSLModeValues, ",");
      throw shcore::Exception::argument_error(
          "Invalid value for memberSslMode option. "
              "Supported values: " + valid_values + ".");
    }
  }
}

void validate_ip_whitelist_option(shcore::Value::Map_type_ref &options) {
  // Validate the value of the ipWhitelist option an issue an exception
  // if invalid.
  shcore::Argument_map opt_map(*options);
  // Just a very simple validation is enough since the GCS layer on top of
  // which group replication runs has proper validation for the values provided
  // to the ipWhitelist option.
  if (opt_map.has_key("ipWhitelist")) {
    std::string ip_whitelist = options->get_string("ipWhitelist");
    boost::trim(ip_whitelist);
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

} // dba
} // mysqlsh
