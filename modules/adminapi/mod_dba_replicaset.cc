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
#include "modules/adminapi/mod_dba_replicaset.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "mod_dba_replicaset.h"
#include "modules/adminapi/dba/dissolve.h"
#include "modules/adminapi/dba/remove_instance.h"
#include "modules/adminapi/dba/replicaset_check_instance_state.h"
#include "modules/adminapi/dba/replicaset_rescan.h"
#include "modules/adminapi/dba/replicaset_set_instance_option.h"
#include "modules/adminapi/dba/replicaset_set_primary_instance.h"
#include "modules/adminapi/dba/replicaset_switch_to_multi_primary_mode.h"
#include "modules/adminapi/dba/replicaset_switch_to_single_primary_mode.h"
#include "modules/adminapi/dba/validations.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "modules/adminapi/mod_dba_sql.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_shell.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "shellcore/base_session.h"
#include "utils/utils_general.h"
#include "utils/utils_net.h"
#include "utils/utils_sqlstring.h"
#include "utils/utils_string.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::dba;
using namespace shcore;
using mysqlshdk::db::uri::formats::only_transport;
using mysqlshdk::db::uri::formats::user_transport;
std::set<std::string> ReplicaSet::_rejoin_instance_opts = {
    "label",       "password",     "memberSslMode",
    "ipWhitelist", "localAddress", "groupSeeds"};

// TODO(nelson): update the values to sm and mp respectively on the next version
// bump
char const *ReplicaSet::kTopologySinglePrimary = "pm";
char const *ReplicaSet::kTopologyMultiPrimary = "mm";

static const std::string kSandboxDatadir = "sandboxdata";

const char *kWarningDeprecateSslMode =
    "Option 'memberSslMode' is deprecated for this operation and it will be "
    "removed in a future release. This option is not needed because the SSL "
    "mode is automatically obtained from the cluster. Please do not use it "
    "here.";

enum class Gr_seeds_change_type {
  ADD,
  REMOVE,
  OVERRIDE,
};

ReplicaSet::ReplicaSet(const std::string &name,
                       const std::string &topology_type,
                       const std::string &group_name,
                       std::shared_ptr<MetadataStorage> metadata_storage)
    : _name(name),
      _topology_type(topology_type),
      _group_name(group_name),
      _metadata_storage(metadata_storage) {
  assert(topology_type == kTopologyMultiPrimary ||
         topology_type == kTopologySinglePrimary);
}

ReplicaSet::~ReplicaSet() {}

std::string &ReplicaSet::append_descr(std::string &s_out, int UNUSED(indent),
                                      int UNUSED(quote_strings)) const {
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

void ReplicaSet::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();
  dumper.append_string("class", class_name());
  dumper.append_string("name", _name);
  dumper.end_object();
}

bool ReplicaSet::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the
 * scripting languages. \param prop : A string containing the name of the member
 * to be returned
 *
 * This function returns a Value that wraps the object returned by this
 * function. The content of the returned value depends on the property being
 * requested. The next list shows the valid properties as well as the returned
 * value for each of them:
 *
 * \li name: returns a String object with the name of this ReplicaSet object.
 */
#else
/**
 * Returns the name of this ReplicaSet object.
 * \return the name as an String object.
 */
#if DOXYGEN_JS
String ReplicaSet::getName() {}
#elif DOXYGEN_PY
str ReplicaSet::get_name() {}
#endif
#endif
shcore::Value ReplicaSet::get_member(const std::string &prop) const {
  shcore::Value ret_val;
  if (prop == "name")
    ret_val = shcore::Value(_name);
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);

  return ret_val;
}

void ReplicaSet::sanity_check() const { verify_topology_type_change(); }

/*
 * Verify if the topology type changed and issue an error if needed.
 */
void ReplicaSet::verify_topology_type_change() const {
  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  std::shared_ptr<Cluster> cluster(_cluster.lock());

  std::string gr_primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      cluster->get_group_session(), nullptr);

  // Check if the topology type matches the real settings used by the
  // cluster instance, otherwise an error is issued.
  // NOTE: The GR primary mode is guaranteed (by GR) to be the same for all
  // instance of the same group.
  if (!gr_primary_uuid.empty() && _topology_type == kTopologyMultiPrimary)
    throw shcore::Exception::runtime_error(
        "The InnoDB Cluster topology type (Multi-Primary) does not match the "
        "current Group Replication configuration (Single-Primary). Please "
        "use <cluster>.rescan() or change the Group Replication "
        "configuration accordingly.");
  else if (gr_primary_uuid.empty() && _topology_type == kTopologySinglePrimary)
    throw shcore::Exception::runtime_error(
        "The InnoDB Cluster topology type (Single-Primary) does not match the "
        "current Group Replication configuration (Multi-Primary). Please "
        "use <cluster>.rescan() or change the Group Replication "
        "configuration accordingly.");
}

void ReplicaSet::set_instance_option(const Connection_options &instance_def,
                                     const std::string &option,
                                     const shcore::Value &value) {
  std::shared_ptr<Cluster> cluster(_cluster.lock());

  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");

  // Set ReplicaSet configuration option

  // Create the Replicaset_set_instance_option object and execute it.
  std::unique_ptr<Replicaset_set_instance_option> op_set_instance_option;

  // Validation types due to a limitation on the expose() framework.
  // Currently, it's not possible to do overloading of functions that overload
  // an argument of type string/int since the type int is convertible to
  // string, thus overloading becomes ambiguous. As soon as that limitation is
  // gone, this type checking shall go away too.
  if (value.type == shcore::String) {
    std::string value_str = value.as_string();
    op_set_instance_option =
        shcore::make_unique<Replicaset_set_instance_option>(this, instance_def,
                                                            option, value_str);
  } else if (value.type == shcore::Integer || value.type == shcore::UInteger) {
    int64_t value_int = value.as_int();
    op_set_instance_option =
        shcore::make_unique<Replicaset_set_instance_option>(this, instance_def,
                                                            option, value_int);
  } else {
    throw shcore::Exception::argument_error(
        "Argument #2 is expected to be a string or an Integer.");
  }

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_set_instance_option]() { op_set_instance_option->finish(); });

  // Prepare the Replicaset_set_instance_option command execution (validations).
  op_set_instance_option->prepare();

  // Execute Replicaset_set_instance_option operations.
  op_set_instance_option->execute();
}

void ReplicaSet::adopt_from_gr() {
  shcore::Value ret_val;

  auto newly_discovered_instances_list(
      get_newly_discovered_instances(_metadata_storage, _id));

  // Add all instances to the cluster metadata
  for (NewInstanceInfo &instance : newly_discovered_instances_list) {
    mysqlshdk::db::Connection_options newly_discovered_instance;

    newly_discovered_instance.set_host(instance.host);
    newly_discovered_instance.set_port(instance.port);

    log_info("Adopting member %s:%d from existing group", instance.host.c_str(),
             instance.port);

    // TODO(somebody): what if the password is different on each server?
    // And what if is different from the current session?
    auto session = _metadata_storage->get_session();

    auto session_data = session->get_connection_options();

    newly_discovered_instance.set_user(session_data.get_user());
    newly_discovered_instance.set_password(session_data.get_password());

    add_instance_metadata(newly_discovered_instance);
  }
}

/**
 * Auxiliary function to update the group_replication_group_seeds variable.
 *
 * @param gr_address address to override or add/remove from the
 * instance's group_replication_group_seeds variable
 * @param Change_type Type of change we want to do to the group_seeds
 * variable. If Add or Remove, we respectively add or remove the gr_address
 * value received as argument to the group_replication_group_seeds variable of
 * the instance whose session we got. If Override, we set the
 * group_replication_group_seeds with the value of gr_address received as
 * argument.
 * @param session to the instance whose group_replication_group_seeds variable
 * is to be changed.
 * @param naming_style Naming style being used, required for the logged
 * warning messages.
 */
static void update_group_replication_group_seeds(
    const std::string &gr_address, const Gr_seeds_change_type change_type,
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const NamingStyle naming_style) {
  std::string gr_group_seeds_new_value;
  std::string address =
      session->get_connection_options().as_uri(only_transport());
  // create an instance object for the provided session
  auto instance = mysqlshdk::mysql::Instance(session);
  auto gr_group_seeds = instance.get_sysvar_string(
      "group_replication_group_seeds", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  auto gr_group_seeds_vector = shcore::split_string(*gr_group_seeds, ",");

  switch (change_type) {
    case Gr_seeds_change_type::ADD:
      // get the group_replication_group_seeds value from the instance
      if (!gr_group_seeds->empty()) {
        // if the group_seeds value is not empty, add the gr_address to it.
        // if it is not already there.
        if (std::find(gr_group_seeds_vector.begin(),
                      gr_group_seeds_vector.end(),
                      gr_address) == gr_group_seeds_vector.end()) {
          gr_group_seeds_vector.push_back(gr_address);
        }
        gr_group_seeds_new_value = shcore::str_join(gr_group_seeds_vector, ",");
      } else {
        // If the instance had no group_seeds yet defined, just set it as the
        // value the gr_address argument.
        gr_group_seeds_new_value = gr_address;
      }
      break;
    case Gr_seeds_change_type::REMOVE:
      gr_group_seeds_vector.erase(
          std::remove(gr_group_seeds_vector.begin(),
                      gr_group_seeds_vector.end(), gr_address),
          gr_group_seeds_vector.end());
      gr_group_seeds_new_value = shcore::str_join(gr_group_seeds_vector, ",");
      break;
    case Gr_seeds_change_type::OVERRIDE:
      gr_group_seeds_new_value = gr_address;
      break;
  }

  auto console = mysqlsh::current_console();

  // Update group_replication_group_seeds variable
  // If server version >= 8.0.11 use set persist, otherwise use set global
  // and warn users that they should use configureLocalInstance to persist
  // the value of the variables
  if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 11)) {
    bool persist_load = *instance.get_sysvar_bool(
        "persisted_globals_load", mysqlshdk::mysql::Var_qualifier::GLOBAL);
    if (!persist_load) {
      std::string warn_msg =
          "On instance '" + address +
          "' the persisted cluster configuration will not be loaded upon "
          "reboot since 'persisted-globals-load' is set "
          "to 'OFF'. Please use the <Dba>." +
          get_member_name("configureLocalInstance", naming_style) +
          " command locally to persist the changes or set "
          "'persisted-globals-load' to 'ON' on the configuration file.";
      console->print_warning(warn_msg);
    }
    instance.set_sysvar("group_replication_group_seeds",
                        gr_group_seeds_new_value,
                        mysqlshdk::mysql::Var_qualifier::PERSIST);
  } else {
    instance.set_sysvar("group_replication_group_seeds",
                        gr_group_seeds_new_value,
                        mysqlshdk::mysql::Var_qualifier::GLOBAL);
    std::string warn_msg =
        "On instance '" + address +
        "' membership change cannot be persisted since MySQL version " +
        instance.get_version().get_base() +
        " does not support the SET PERSIST command "
        "(MySQL version >= 8.0.11 required). Please use the <Dba>." +
        get_member_name("configureLocalInstance", naming_style) +
        " command locally to persist the changes.";
    console->print_warning(warn_msg);
  }
}

/**
 * Adds a Instance to the ReplicaSet
 * \param conn The Connection String or URI of the Instance to be added
 */
#if DOXYGEN_JS
Undefined ReplicaSet::addInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::add_instance(InstanceDef instance, doptions) {}
#endif
shcore::Value ReplicaSet::add_instance_(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, 2, get_function_name("addInstance").c_str());

  // Check if the ReplicaSet is empty
  if (_metadata_storage->is_replicaset_empty(get_id()))
    throw shcore::Exception::runtime_error(
        "ReplicaSet not initialized. Please add the Seed Instance using: "
        "addSeedInstance().");

  // Add the Instance to the Default ReplicaSet
  try {
    auto instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    shcore::Argument_list rest;
    if (args.size() == 2) rest.push_back(args.at(1));

    ret_val = add_instance(instance_def, rest);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  return ret_val;
}

void ReplicaSet::validate_instance_address(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &hostname, int port) {
  if (mysqlshdk::utils::Net::is_loopback(hostname)) {
    // if the address is local (localhost or 127.0.0.1), we know it's local and
    // so can be used with sandboxes only
    std::string datadir =
        session->query("SELECT @@datadir")->fetch_one()->get_as_string(0);
    if (!datadir.empty() && (datadir[datadir.size() - 1] == '/' ||
                             datadir[datadir.size() - 1] == '\\'))
      datadir.pop_back();
    if (datadir.length() < kSandboxDatadir.length() ||
        datadir.compare(datadir.length() - kSandboxDatadir.length(),
                        kSandboxDatadir.length(), kSandboxDatadir) != 0) {
      log_info("'%s' is a local address but not in a sandbox (datadir %s)",
               hostname.c_str(), datadir.c_str());
      throw shcore::Exception::runtime_error(
          "To add an instance to the cluster, please use a valid, non-local "
          "hostname or IP. " +
          hostname + " can only be used with sandbox MySQL instances.");
    }
  }
}

void set_group_replication_member_options(
    std::shared_ptr<mysqlshdk::db::ISession> session,
    const std::string &ssl_mode) {
  if (session->get_server_version() >= mysqlshdk::utils::Version(8, 0, 5) &&
      ssl_mode == dba::kMemberSSLModeDisabled) {
    // We need to install the GR plugin to have GR sysvars available
    mysqlshdk::mysql::Instance instance(session);
    mysqlshdk::gr::install_plugin(instance, nullptr);

    // This option required to connect using the new caching_sha256_password
    // authentication method without SSL
    session->query("SET PERSIST group_replication_recovery_get_public_key=1");
  }
}

shcore::Value ReplicaSet::add_instance(
    const mysqlshdk::db::Connection_options &connection_options,
    const shcore::Argument_list &args,
    const std::string &existing_replication_user,
    const std::string &existing_replication_password, bool overwrite_seed,
    const std::string &group_name, bool skip_instance_check,
    bool skip_rpl_user) {
  shcore::Value ret_val;

  bool seed_instance = false;
  std::string ssl_mode = dba::kMemberSSLModeAuto;  // SSL Mode AUTO by default
  std::string ip_whitelist, instance_label, local_address, group_seeds,
      exit_state_action, failover_consistency;
  mysqlshdk::utils::nullable<int64_t> member_weight, expel_timeout;

  std::shared_ptr<Cluster> cluster(_cluster.lock());
  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");

  // NOTE: This function is called from either the add_instance_ on this class
  //       or the add_instance in Cluster class, hence this just throws
  //       exceptions and the proper handling is done on the caller functions
  //       (to append the called function name)

  // Check if we're on a addSeedInstance or not
  if (_metadata_storage->is_replicaset_empty(_id)) seed_instance = true;

  // Check if we need to overwrite the seed instance
  if (overwrite_seed) seed_instance = true;

  auto console = mysqlsh::current_console();

  std::shared_ptr<mysqlshdk::db::ISession> session{establish_mysql_session(
      connection_options, current_shell_options()->get().wizards)};
  mysqlshdk::mysql::Instance target_instance(session);
  target_instance.cache_global_sysvars();

  // Retrieves the add options
  if (args.size() == 1) {
    auto add_options = args.map_at(0);
    shcore::Argument_map add_instance_map(*add_options);

    // Retrieves optional options if exists
    Unpack_options(add_options)
        .optional("memberSslMode", &ssl_mode)
        .optional("ipWhitelist", &ip_whitelist)
        .optional("label", &instance_label)
        .optional("localAddress", &local_address)
        .optional("groupSeeds", &group_seeds)
        .optional("exitStateAction", &exit_state_action)
        .optional_exact("memberWeight", &member_weight)
        .optional("failoverConsistency", &failover_consistency)
        .optional_exact("expelTimeout", &expel_timeout)
        .end();

    // Validate SSL options for the cluster instance
    validate_ssl_instance_options(add_options);

    // Validate local address option
    validate_local_address_option(add_options);

    // Validate group seeds option
    validate_group_seeds_option(add_options);

    // Validate if the exitStateAction option is supported on the target
    // instance and if is not empty.
    // The validation for the value set is handled at the group-replication
    // level
    if (add_options->has_key("exitStateAction")) {
      if (shcore::str_strip(exit_state_action).empty())
        throw shcore::Exception::argument_error(
            "Invalid value for exitStateAction, string value cannot be empty.");

      validate_exit_state_action_supported(session);
    }

    // Validate if the memberWeight option is supported on the target
    // instance and if it used in the optional parameters.
    // The validation for the value set is handled at the group-replication
    // level
    if (!member_weight.is_null()) validate_member_weight_supported(session);

    if (add_options->has_key("memberSslMode")) {
      if (!seed_instance) {
        console->print_warning(kWarningDeprecateSslMode);
        console->println();
      }
    }

    if (add_options->has_key("label")) {
      mysqlsh::dba::validate_label(instance_label);

      if (!_metadata_storage->is_instance_label_unique(get_id(),
                                                       instance_label))
        throw shcore::Exception::argument_error(
            "An instance with label '" + instance_label +
            "' is already part of this InnoDB cluster");
    }
  }

  // BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC
  // - exitStateAction default value must be READ_ONLY
  // - exitStateAction default value should only be set if supported in
  // the target instance
  if (exit_state_action.empty() &&
      is_group_replication_option_supported(session, kExpelTimeout)) {
    exit_state_action = "READ_ONLY";
  }

  // Retrieves the instance definition
  auto target_coptions = session->get_connection_options();

  // Check whether the address being used is not in a known not-good case
  validate_instance_address(session, target_coptions.get_host(),
                            target_coptions.get_port());

  // Check instance configuration and state, like dba.checkInstance
  // But don't do it if it was already done by the caller
  if (!skip_instance_check) {
    ensure_instance_configuration_valid(&target_instance,
                                        cluster->get_provisioning_interface());
  }

  // Validate ip whitelist option if used
  if (!ip_whitelist.empty()) {
    bool hostnames_supported = false;

    if (session->get_server_version() >= mysqlshdk::utils::Version(8, 0, 4))
      hostnames_supported = true;

    validate_ip_whitelist_option(ip_whitelist, hostnames_supported);
  }

  // Check replication filters before creating the Metadata.
  validate_replication_filters(session);

  // Resolve the SSL Mode to use to configure the instance.
  std::string new_ssl_mode;
  std::string target;
  if (seed_instance) {
    new_ssl_mode = resolve_cluster_ssl_mode(session, ssl_mode);
    target = "cluster";
  } else {
    auto peer_session = _metadata_storage->get_session();
    new_ssl_mode = resolve_instance_ssl_mode(session, peer_session, ssl_mode);
    target = "instance";
  }

  if (new_ssl_mode != ssl_mode) {
    ssl_mode = new_ssl_mode;
    log_warning("SSL mode used to configure the %s: '%s'", target.c_str(),
                ssl_mode.c_str());
  }

  // TODO(alfredo) move these checks to a target instance preconditions check
  GRInstanceType type = get_gr_instance_type(session);

  if (type != GRInstanceType::Standalone &&
      type != GRInstanceType::StandaloneWithMetadata) {
    // Retrieves the new instance UUID
    std::string uuid;
    get_server_variable(session, "server_uuid", uuid);

    // Verifies if the instance is part of the cluster replication group
    auto cluster_session = cluster->get_group_session();

    // Verifies if this UUID is part of the current replication group
    if (is_server_on_replication_group(cluster_session, uuid)) {
      if (type == GRInstanceType::InnoDBCluster) {
        log_debug("Instance '%s' already managed by InnoDB cluster",
                  target_coptions.uri_endpoint().c_str());
        throw shcore::Exception::runtime_error(
            "The instance '" + target_coptions.uri_endpoint() +
            "' is already part of this InnoDB cluster");
      } else {
        current_console()->print_error(
            "Instance " + target_coptions.uri_endpoint() +
            " is part of the GR group but is not in the metadata. Please use "
            "<Cluster>.rescan() to update the metadata.");
        throw shcore::Exception::runtime_error("Metadata inconsistent");
      }
    } else {
      if (type == GRInstanceType::InnoDBCluster)
        throw shcore::Exception::runtime_error(
            "The instance '" + target_coptions.uri_endpoint() +
            "' is already part of another InnoDB cluster");
      else
        throw shcore::Exception::runtime_error(
            "The instance '" + target_coptions.uri_endpoint() +
            "' is already part of another Replication Group");
    }
  }

  // Check instance server UUID (must be unique among the cluster members).
  validate_server_uuid(session);

  bool is_instance_on_md = _metadata_storage->is_instance_on_replicaset(
      get_id(), target_coptions.uri_endpoint());

  log_debug("RS %s: Adding instance '%s' to replicaset%s",
            std::to_string(_id).c_str(), target_coptions.uri_endpoint().c_str(),
            is_instance_on_md ? " (already in MD)" : "");

  bool success = false;

  std::string replication_user(existing_replication_user);
  std::string replication_user_password(existing_replication_password);

  // Handle the replication user
  if (seed_instance) {
    // Creates the replication user ONLY if not already given and if
    // skip_rpl_user was not set to true.
    // directly at the instance
    if (!skip_rpl_user && replication_user.empty()) {
      mysqlshdk::gr::create_replication_random_user_pass(
          target_instance, &replication_user,
          convert_ipwhitelist_to_netmask(ip_whitelist),
          &replication_user_password);

      log_debug("Created replication user '%s'", replication_user.c_str());
    }

    log_info("Joining '%s' to group with user %s",
             target_coptions.uri_endpoint().c_str(),
             target_coptions.get_user().c_str());
  } else {
    mysqlshdk::db::Connection_options peer(pick_seed_instance());
    std::shared_ptr<mysqlshdk::db::ISession> peer_session;
    if (peer.uri_endpoint() !=
        cluster->get_group_session()->get_connection_options().uri_endpoint()) {
      peer_session =
          establish_mysql_session(peer, current_shell_options()->get().wizards);
    } else {
      peer_session = cluster->get_group_session();
    }

    // Creates the replication user ONLY if not already given and if
    // skip_rpl_user was not set to true.
    // at the instance that will serve as the seed for this one
    if (!skip_rpl_user && replication_user.empty()) {
      mysqlshdk::gr::create_replication_random_user_pass(
          mysqlshdk::mysql::Instance(peer_session), &replication_user,
          convert_ipwhitelist_to_netmask(ip_whitelist),
          &replication_user_password);

      log_debug("Created replication user '%s'", replication_user.c_str());
    }
  }

  // If this is not seed instance, then we should try to read the
  // failoverConsistency and expelTimeout values from a a cluster member
  if (!seed_instance) {
    auto conn_opt = session->get_connection_options();
    // loop though all members to check if there is any member that doesn't
    // have support for the group_replication_consistency option (null value)
    // or a member that doesn't have the default value. The default value
    // Eventual has the same behavior as members had before the option was
    // introduced. As such, having that value or having no support for the
    // group_replication_consistency is the same.
    bool found_non_default = false;
    bool found_not_supported = false;
    auto get_gr_consistency_func =
        [&found_non_default, &found_not_supported](
            std::shared_ptr<mysqlshdk::db::ISession> session) -> bool {
      mysqlshdk::mysql::Instance instance(session);
      mysqlshdk::utils::nullable<std::string> value;
      value =
          instance.get_sysvar_string("group_replication_consistency",
                                     mysqlshdk::mysql::Var_qualifier::GLOBAL);

      if (value.is_null())
        found_not_supported = true;
      else if (*value != "EVENTUAL" && *value != "0")
        found_non_default = true;
      // if we have found both a instance that doesnt have support for the
      // option and an instance that doesn't have the default value, then we
      // don't need to look at any other instance on the cluster.
      return !(found_not_supported && found_non_default);
    };
    execute_in_members({"'ONLINE'", "'RECOVERING'"}, conn_opt, {},
                       get_gr_consistency_func);
    if (target_instance.get_version() < mysqlshdk::utils::Version(8, 0, 14)) {
      if (found_non_default) {
        console->print_warning(
            "The BEFORE_ON_PRIMARY_FAILOVER consistency value of the cluster "
            "is not supported by the instance '" +
            target_instance.get_connection_options().uri_endpoint() +
            "' (version >= 8.0.14 is required). In single-primary mode, upon "
            "failover, the member with the lowest version is the one elected as"
            " primary.");
      }
    } else {
      failover_consistency = "EVENTUAL";
      if (found_non_default) {
        // if we found any non default group_replication_consistency value, then
        // we use that value on the instance being added
        failover_consistency = "BEFORE_ON_PRIMARY_FAILOVER";
        if (found_not_supported) {
          console->print_warning(
              "The instance '" +
              target_instance.get_connection_options().uri_endpoint() +
              "' inherited the BEFORE_ON_PRIMARY_FAILOVER consistency "
              "value from the cluster, however some instances on the group do "
              "not support this feature (version < 8.0.14). In single-primary "
              "mode, upon failover, the member with the lowest version will be"
              "the one elected and it doesn't support this option.");
        }
      }
    }
    // loop though all members to check if there is any member that doesn't
    // have support for the group_replication_member_expel_timeout option
    // (null value) or a member that doesn't have the default value. The default
    // value 0 has the same behavior as members had before the option was
    // introduced. As such, having the 0 value or having no support for the
    // group_replication_member_expel_timeout is the same.
    int64_t non_default_val = 0;
    found_not_supported = false;
    auto get_gr_expel_timeout_func =
        [&non_default_val, &found_not_supported](
            std::shared_ptr<mysqlshdk::db::ISession> session) -> bool {
      mysqlshdk::mysql::Instance instance(session);
      mysqlshdk::utils::nullable<std::int64_t> value;
      value = instance.get_sysvar_int("group_replication_member_expel_timeout",
                                      mysqlshdk::mysql::Var_qualifier::GLOBAL);

      if (value.is_null())
        found_not_supported = true;
      else if (*value != 0)
        non_default_val = *value;
      // if we have found both a instance that doesnt have support for the
      // option and an instance that doesn't have the default value, then we
      // don't need to look at any other instance on the cluster.
      return !(found_not_supported && non_default_val != 0);
    };
    execute_in_members({"'ONLINE'", "'RECOVERING'"}, conn_opt, {},
                       get_gr_expel_timeout_func);
    if (target_instance.get_version() < mysqlshdk::utils::Version(8, 0, 13)) {
      if (non_default_val != 0) {
        console->print_warning(
            "The expelTimeout value of the cluster '" +
            std::to_string(non_default_val) +
            "' is not supported by the instance '" +
            target_instance.get_connection_options().uri_endpoint() +
            "' (version >= 8.0.13 is required). A member "
            "that doesn't have support for the expelTimeout option has the "
            "same behavior as a member with expelTimeout=0.");
      }
    } else {
      int64_t default_expel_timeout = 0;
      expel_timeout =
          mysqlshdk::utils::nullable<int64_t>(default_expel_timeout);
      if (non_default_val != 0) {
        // if we found any non default group_replication_member_expel_timeout
        // value, then we use that value on the instance being added
        expel_timeout = non_default_val;
        if (found_not_supported) {
          console->print_warning(
              "The instance '" +
              target_instance.get_connection_options().uri_endpoint() +
              "' inherited the '" + std::to_string(non_default_val) + +
              "' consistency value from the cluster, however some instances on "
              "the group do not support this feature (version < 8.0.13). There "
              "is a possibility that the cluster member (killer node), "
              "responsible for expelling the member suspected of having "
              "failed, does not support the expelTimeout option. In "
              "this case the behavior would be the same as if having "
              "expelTimeout=0.");
        }
      }
    }
  };

  // Set the ssl mode
  set_group_replication_member_options(session, ssl_mode);

  // Common informative logging
  if (!local_address.empty()) {
    log_info("Using Group Replication local address: %s",
             local_address.c_str());
  }

  if (!group_seeds.empty()) {
    log_info("Using Group Replication group seeds: %s", group_seeds.c_str());
  }

  if (!exit_state_action.empty()) {
    log_info("Using Group Replication exit state action: %s",
             exit_state_action.c_str());
  }

  if (!member_weight.is_null()) {
    log_info("Using Group Replication member weight: %s",
             std::to_string(*member_weight).c_str());
  }

  // Call MP
  if (seed_instance) {
    if (!group_name.empty()) {
      log_info("Using Group Replication group name: %s", group_name.c_str());
    }

    // Call mysqlprovision to bootstrap the group using "start"
    success = do_join_replicaset(
        target_coptions, nullptr, replication_user, replication_user_password,
        ssl_mode, ip_whitelist, member_weight, expel_timeout, group_name,
        local_address, group_seeds, exit_state_action, failover_consistency,
        skip_rpl_user);
  } else {
    mysqlshdk::db::Connection_options peer(pick_seed_instance());

    // if no group_seeds value was provided by the user, then,
    // before joining instance to cluster, get the values of the
    // gr_local_address from all the active members of the cluster
    if (group_seeds.empty()) group_seeds = get_cluster_group_seeds();
    log_info("Joining '%s' to group using account %s to peer '%s'",
             target_coptions.uri_endpoint().c_str(), peer.get_user().c_str(),
             peer.uri_endpoint().c_str());
    // Call mysqlprovision to do the work
    success = do_join_replicaset(
        target_coptions, &peer, replication_user, replication_user_password,
        ssl_mode, ip_whitelist, member_weight, expel_timeout, group_name,
        local_address, group_seeds, exit_state_action, failover_consistency,
        skip_rpl_user);
  }

  if (success) {
    // If the instance is not on the Metadata, we must add it
    if (!is_instance_on_md)
      add_instance_metadata(target_coptions, instance_label);

    // Get the gr_address of the instance being added
    auto added_instance = mysqlshdk::mysql::Instance(session);
    std::string added_instance_gr_address = *added_instance.get_sysvar_string(
        "group_replication_local_address",
        mysqlshdk::mysql::Var_qualifier::GLOBAL);

    // Update the group_seeds of the instance that was just added
    // If the groupSeeds option was used (not empty), we use
    // that value, otherwise we use the value of all the
    // group_replication_local_address of all the active instances
    update_group_replication_group_seeds(
        group_seeds, Gr_seeds_change_type::OVERRIDE, session, naming_style);
    // Update the group_replication_group_seeds of the members that
    // already belonged to the cluster and are either ONLINE or recovering
    // by adding the gr_local_address of the instance that was just added.
    std::vector<std::string> ignore_instances_vec = {
        target_coptions.uri_endpoint()};
    Gr_seeds_change_type change_type = Gr_seeds_change_type::ADD;

    execute_in_members(
        {"'ONLINE'", "'RECOVERING'"}, target_coptions, ignore_instances_vec,
        [added_instance_gr_address, change_type,
         this](std::shared_ptr<mysqlshdk::db::ISession> session) -> bool {
          update_group_replication_group_seeds(added_instance_gr_address,
                                               change_type, session,
                                               this->naming_style);
          return true;
        });
    log_debug("Instance add finished");
  }

  return ret_val;
}

bool ReplicaSet::do_join_replicaset(
    const mysqlshdk::db::Connection_options &instance,
    mysqlshdk::db::Connection_options *peer, const std::string &repl_user,
    const std::string &repl_user_password, const std::string &ssl_mode,
    const std::string &ip_whitelist,
    mysqlshdk::utils::nullable<int64_t> member_weight,
    mysqlshdk::utils::nullable<int64_t> expel_timeout,
    const std::string &group_name, const std::string &local_address,
    const std::string &group_seeds, const std::string &exit_state_action,
    const std::string &failover_consistency, bool skip_rpl_user) {
  shcore::Value ret_val;
  int exit_code = -1;

  bool is_seed_instance = peer ? false : true;

  shcore::Value::Array_type_ref errors, warnings;

  std::shared_ptr<Cluster> cluster(_cluster.lock());
  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");

  if (is_seed_instance) {
    exit_code = cluster->get_provisioning_interface()->start_replicaset(
        instance, repl_user, repl_user_password,
        _topology_type == kTopologyMultiPrimary, ssl_mode, ip_whitelist,
        group_name, local_address, group_seeds, exit_state_action,
        member_weight, expel_timeout, failover_consistency, skip_rpl_user,
        &errors);
  } else {
    exit_code = cluster->get_provisioning_interface()->join_replicaset(
        instance, *peer, repl_user, repl_user_password, ssl_mode, ip_whitelist,
        local_address, group_seeds, exit_state_action, member_weight,
        expel_timeout, failover_consistency, skip_rpl_user, &errors);
  }

  if (exit_code == 0) {
    auto instance_url = instance.as_uri(user_transport());
    // If the exit_code is zero but there are errors
    // it means they're warnings and we must log them first
    if (errors) {
      for (auto error_object : *errors) {
        auto map = error_object.as_map();
        std::string error_str = map->get_string("msg");
        log_warning("DBA: %s : %s", instance_url.c_str(), error_str.c_str());
      }
    }
    if (is_seed_instance)
      ret_val = shcore::Value(
          "The instance '" + instance_url +
          "' was successfully added as seeding instance to the MySQL Cluster.");
    else
      ret_val = shcore::Value("The instance '" + instance_url +
                              "' was successfully added to the MySQL Cluster.");
  } else {
    throw shcore::Exception::runtime_error(
        get_mysqlprovision_error_string(errors));
  }

  return exit_code == 0;
}

#if DOXYGEN_CPP
/**
 * Use this function to rejoin an Instance to the ReplicaSet
 * \param args : A list of values to be used to add a Instance to the
 * ReplicaSet.
 *
 * This function returns an empty Value.
 */
#else
/**
 * Rejoin a Instance to the ReplicaSet
 * \param name The name of the Instance to be rejoined
 */
#if DOXYGEN_JS
Undefined ReplicaSet::rejoinInstance(String name, Dictionary options) {}
#elif DOXYGEN_PY
None ReplicaSet::rejoin_instance(str name, Dictionary options) {}
#endif
#endif  // DOXYGEN_CPP
shcore::Value ReplicaSet::rejoin_instance_(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, 2, get_function_name("rejoinInstance").c_str());

  // Check if the ReplicaSet is empty
  if (_metadata_storage->is_replicaset_empty(get_id()))
    throw shcore::Exception::runtime_error(
        "ReplicaSet not initialized. Please add the Seed Instance using: "
        "addSeedInstance().");

  // Rejoin the Instance to the Default ReplicaSet
  try {
    auto instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    shcore::Value::Map_type_ref options;

    if (args.size() == 2) options = args.map_at(1);

    ret_val = rejoin_instance(&instance_def, options);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  return ret_val;
}

/**
 * Get an up-to-date group seeds value based on the current list of active
 * members.
 *
 * An optional instance_session parameter can be provide that will be used to
 * get its current GR group seeds value and add the local address from the
 * active members (avoiding duplicates) to that initial value, allowing to
 * preserve the GR group seeds of the specified instance. If no
 * instance_session is given (nullptr) then the returned groups seeds value
 * will only be based on the currently active members, disregarding any current
 * GR group seed value on any instance (allowing to reset the group seed only
 * based on the currently active members).
 *
 * @param instance_session Session to the target instance to get the group
 *                         seeds for.
 * @return a string with a comma separated list of all the GR local address
 *         values of the currently active (ONLINE or RECOVERING) instances in
 *         the replicaset.
 */
std::string ReplicaSet::get_cluster_group_seeds(
    std::shared_ptr<mysqlshdk::db::ISession> instance_session) {
  // Get connection option for the metadata.
  std::shared_ptr<Cluster> cluster(_cluster.lock());
  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");
  std::shared_ptr<mysqlshdk::db::ISession> cluster_session =
      cluster->get_group_session();
  Connection_options cluster_cnx_opt =
      cluster_session->get_connection_options();

  // Get list of active instances (ONLINE or RECOVERING)
  std::vector<Instance_definition> active_instances =
      _metadata_storage->get_replicaset_active_instances(_id);

  std::vector<std::string> gr_group_seeds_list;
  // If the target instance is provided, use its current GR group seed variable
  // value as starting point to append new (missing) values to it.
  if (instance_session) {
    auto instance = mysqlshdk::mysql::Instance(instance_session);
    // Get the instance GR group seeds and save it to the GR group seeds list.
    std::string gr_group_seeds =
        instance.get_sysvar_string("group_replication_group_seeds",
                                   mysqlshdk::mysql::Var_qualifier::GLOBAL);
    if (!gr_group_seeds.empty()) {
      gr_group_seeds_list = shcore::split_string(gr_group_seeds, ",");
    }
  }

  // Get the update GR group seed from local address of all active instances.
  for (Instance_definition &instance_def : active_instances) {
    std::string instance_address = instance_def.endpoint;
    Connection_options target_coptions =
        shcore::get_connection_options(instance_address, false);
    // It is assumed that the same user and password is used by all members.
    if (cluster_cnx_opt.has_user())
      target_coptions.set_user(cluster_cnx_opt.get_user());
    if (cluster_cnx_opt.has_password())
      target_coptions.set_password(cluster_cnx_opt.get_password());
    // Connect to the instance.
    std::shared_ptr<mysqlshdk::db::ISession> session;
    try {
      log_debug(
          "Connecting to instance '%s' to get its value for the "
          "group_replication_local_address variable.",
          instance_address.c_str());
      session = establish_mysql_session(target_coptions,
                                        current_shell_options()->get().wizards);
    } catch (std::exception &e) {
      // Do not issue an error if we are unable to connect to the instance,
      // it might have failed in the meantime, just skip the use of its GR
      // local address.
      log_info(
          "Could not connect to instance '%s', its local address will not "
          "be used for the group seeds: %s",
          instance_address.c_str(), e.what());
      break;
    }
    auto instance = mysqlshdk::mysql::Instance(session);
    // Get the instance GR local address and add it to the GR group seeds list.
    std::string local_address =
        instance.get_sysvar_string("group_replication_local_address",
                                   mysqlshdk::mysql::Var_qualifier::GLOBAL);
    if (std::find(gr_group_seeds_list.begin(), gr_group_seeds_list.end(),
                  local_address) == gr_group_seeds_list.end()) {
      // Only add the local address if not already in the group seed list,
      // avoiding duplicates.
      gr_group_seeds_list.push_back(local_address);
    }
    session->close();
  }
  return shcore::str_join(gr_group_seeds_list, ",");
}

shcore::Value ReplicaSet::rejoin_instance(
    mysqlshdk::db::Connection_options *instance_def,
    const shcore::Value::Map_type_ref &rejoin_options) {
  std::shared_ptr<Cluster> cluster(_cluster.lock());
  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");

  shcore::Value ret_val;
  // SSL Mode AUTO by default
  std::string ssl_mode = mysqlsh::dba::kMemberSSLModeAuto;
  std::string ip_whitelist, user, password;
  shcore::Value::Array_type_ref errors;
  std::shared_ptr<mysqlshdk::db::ISession> session, seed_session;

  auto console = mysqlsh::current_console();

  // Retrieves the options
  if (rejoin_options) {
    shcore::Argument_map rejoin_instance_map(*rejoin_options);
    rejoin_instance_map.ensure_keys({}, _rejoin_instance_opts, " options");

    // Validate SSL options for the cluster instance
    validate_ssl_instance_options(rejoin_options);

    if (rejoin_options->has_key("memberSslMode")) {
      ssl_mode = rejoin_options->get_string("memberSslMode");
      console->print_warning(kWarningDeprecateSslMode);
      console->println();
    }

    if (rejoin_options->has_key("ipWhitelist"))
      ip_whitelist = rejoin_options->get_string("ipWhitelist");
  }

  if (!instance_def->has_port())
    instance_def->set_port(mysqlshdk::db::k_default_mysql_port);

  instance_def->set_default_connection_data();

  // Check if the instance is part of the Metadata
  if (!_metadata_storage->is_instance_on_replicaset(
          get_id(), instance_def->uri_endpoint())) {
    std::string message = "The instance '" + instance_def->uri_endpoint() +
                          "' " + "does not belong to the ReplicaSet: '" +
                          get_member("name").get_string() + "'.";

    throw shcore::Exception::runtime_error(message);
  }

  // Before rejoining an instance we must verify if the instance's
  // 'group_replication_group_name' matches the one registered in the
  // Metadata (BUG #26159339)
  //
  // Before rejoining an instance we must also verify if the group has quorum
  // and if the gr plugin is active otherwise we may end up hanging the system

  // Validate 'group_replication_group_name'
  {
    try {
      log_info("Opening a new session to the rejoining instance %s",
               instance_def->uri_endpoint().c_str());
      session = establish_mysql_session(*instance_def,
                                        current_shell_options()->get().wizards);
    } catch (std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_def->uri_endpoint().c_str(), e.what());
      throw;
    }

    // Validate ip whitelist option if used
    if (!ip_whitelist.empty()) {
      bool hostnames_supported = false;

      if (session->get_server_version() >= mysqlshdk::utils::Version(8, 0, 4))
        hostnames_supported = true;

      validate_ip_whitelist_option(ip_whitelist, hostnames_supported);
    }

    if (!validate_replicaset_group_name(session, get_group_name())) {
      std::string nice_error =
          "The instance '" + instance_def->uri_endpoint() +
          "' "
          "may belong to a different ReplicaSet as the one registered "
          "in the Metadata since the value of "
          "'group_replication_group_name' does not match the one "
          "registered in the ReplicaSet's Metadata: possible split-brain "
          "scenario. Please remove the instance from the cluster.";

      session->close();

      throw shcore::Exception::runtime_error(nice_error);
    }
  }

  // In order to be able to rejoin the instance to the cluster we need the seed
  // instance.

  // Get the seed instance
  mysqlshdk::db::Connection_options seed_instance(pick_seed_instance());

  // To be able to establish a session to the seed instance we need a username
  // and password. Taking into consideration the assumption that all instances
  // of the cluster use the same credentials we can obtain the ones of the
  // current target group session

  seed_instance.set_login_options_from(
      cluster->get_group_session()->get_connection_options());

  // Establish a session to the seed instance
  try {
    log_info("Opening a new session to seed instance: %s",
             seed_instance.uri_endpoint().c_str());
    seed_session = establish_mysql_session(
        seed_instance, current_shell_options()->get().wizards);
  } catch (std::exception &e) {
    throw Exception::runtime_error("Could not open a connection to " +
                                   seed_instance.uri_endpoint() + ": " +
                                   e.what() + ".");
  }

  // Verify if the group_replication plugin is active on the seed instance
  {
    log_info(
        "Verifying if the group_replication plugin is active on the seed "
        "instance %s",
        seed_instance.uri_endpoint().c_str());

    std::string plugin_status =
        get_plugin_status(seed_session, "group_replication");

    if (plugin_status != "ACTIVE") {
      throw shcore::Exception::runtime_error(
          "Cannot rejoin instance. The seed instance doesn't have "
          "group-replication active.");
    }
  }

  // Verify if the instance being added is MISSING, otherwise throw an error
  // Bug#26870329
  {
    // get server_uuid from the instance that we're trying to rejoin
    if (!validate_instance_rejoinable(session, _metadata_storage, _id)) {
      // instance not missing, so throw an error
      auto instance = mysqlshdk::mysql::Instance(session);
      auto member_state =
          mysqlshdk::gr::to_string(mysqlshdk::gr::get_member_state(instance));
      std::string nice_error_msg = "Cannot rejoin instance '" +
                                   instance.descr() + "' to the ReplicaSet '" +
                                   get_member("name").get_string() +
                                   "' since it is an active (" + member_state +
                                   ") member of the ReplicaSet.";
      session->close();
      throw shcore::Exception::runtime_error(nice_error_msg);
    }
  }

  // Get the up-to-date GR group seeds values (with the GR local address from
  // all currently active instances).
  std::string gr_group_seeds = get_cluster_group_seeds(session);

  // join Instance to cluster
  {
    int exit_code;
    std::string replication_user, replication_user_pwd;

    // Check replication filters before creating the Metadata.
    validate_replication_filters(session);

    std::string new_ssl_mode;
    // Resolve the SSL Mode to use to configure the instance.
    new_ssl_mode = resolve_instance_ssl_mode(session, seed_session, ssl_mode);
    if (new_ssl_mode != ssl_mode) {
      ssl_mode = new_ssl_mode;
      log_warning("SSL mode used to configure the instance: '%s'",
                  ssl_mode.c_str());
    }

    // Get SSL values to connect to peer instance
    auto seed_instance_def = seed_session->get_connection_options();

    // Stop group-replication
    log_info("Stopping group-replication at instance %s",
             seed_instance.uri_endpoint().c_str());
    session->execute("STOP GROUP_REPLICATION");

    // F4. When a valid 'ipWhitelist' is used on the .rejoinInstance() command,
    // the previously existing "replication-user" must be removed from all the
    // cluster members and a new one created to match the 'ipWhitelist' defined
    // filter.
    bool keep_repl_user = ip_whitelist.empty();

    if (!keep_repl_user) {
      mysqlshdk::mysql::Instance instance(seed_session);

      log_info("Recreating replication accounts due to 'ipWhitelist' change.");

      // Remove all the replication users of the instance and the
      // replication-user of the rejoining instance on all the members of the
      // replicaSet
      remove_replication_users(mysqlshdk::mysql::Instance(session), true);

      // Create a new replication user to match the ipWhitelist filter
      mysqlshdk::gr::create_replication_random_user_pass(
          instance, &replication_user,
          convert_ipwhitelist_to_netmask(ip_whitelist), &replication_user_pwd);

      log_debug("Created replication user '%s'", replication_user.c_str());
    }

    // Get the seed session connection data
    // use mysqlprovision to rejoin the cluster.
    exit_code = cluster->get_provisioning_interface()->join_replicaset(
        session->get_connection_options(), seed_instance_def, replication_user,
        replication_user_pwd, ssl_mode, ip_whitelist, "", gr_group_seeds, "",
        mysqlshdk::utils::nullable<int64_t>(),
        mysqlshdk::utils::nullable<int64_t>(), "", keep_repl_user, &errors);

    if (exit_code == 0) {
      log_info("The instance '%s' was successfully rejoined on the cluster.",
               seed_instance.uri_endpoint().c_str());
    } else {
      throw shcore::Exception::runtime_error(
          get_mysqlprovision_error_string(errors));
    }
  }
  return ret_val;
}

#if DOXYGEN_CPP
/**
 * Use this function to remove a Instance from the ReplicaSet object
 * \param args : A list of values to be used to remove a Instance to the
 * Cluster.
 *
 * This function returns an empty Value.
 */
#else
/**
 * Removes a Instance from the ReplicaSet
 * \param name The name of the Instance to be removed
 */
#if DOXYGEN_JS
Undefined ReplicaSet::removeInstance(String name) {}
#elif DOXYGEN_PY
None ReplicaSet::remove_instance(str name) {}
#endif

/**
 * Removes a Instance from the ReplicaSet
 * \param doc The Document representing the Instance to be removed
 */
#if DOXYGEN_JS
Undefined ReplicaSet::removeInstance(Document doc) {}
#elif DOXYGEN_PY
None ReplicaSet::remove_instance(Document doc) {}
#endif
#endif

shcore::Value ReplicaSet::remove_instance(const shcore::Argument_list &args) {
  mysqlshdk::utils::nullable<bool> force;
  bool interactive;
  std::string password;
  mysqlshdk::db::Connection_options target_coptions;

  // Get target instance connection options.
  target_coptions =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

  std::shared_ptr<Cluster> cluster(_cluster.lock());
  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");
  interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (args.size() == 2) {
    Unpack_options(args.map_at(1))
        .optional("force", &force)
        .optional("interactive", &interactive)
        .optional_ci("password", &password)
        .end();
  }

  // Overide password if provided in options dictionary.
  if (!password.empty()) {
    target_coptions.set_password(password);
  }

  // Remove the Instance from the ReplicaSet
  try {
    // Create the remove_instance command and execute it.
    Remove_instance op_remove_instance(target_coptions, interactive, force,
                                       cluster, shared_from_this(),
                                       this->naming_style);
    // Always execute finish when leaving "try catch".
    auto finally = shcore::on_leave_scope(
        [&op_remove_instance]() { op_remove_instance.finish(); });
    // Prepare the remove_instance command execution (validations).
    op_remove_instance.prepare();
    // Execute remove_instance operations.
    op_remove_instance.execute();
  } catch (...) {
    throw;
  }

  return shcore::Value();
}

/**
 * Auxiliary function to re-enable super_read_only.
 *
 * @param super_read_only boolean with the initial super_read_only value.
 * @param instance target instance object.
 * @param instance_address string with the target instance address.
 */
static void reenable_super_read_only(
    const mysqlshdk::utils::nullable<bool> &super_read_only,
    const mysqlshdk::mysql::Instance &instance,
    const std::string &instance_address) {
  // Re-enable super_read_only if previously enabled.
  if (*super_read_only) {
    log_debug("Re-enabling super_read_only on instance '%s'.",
              instance_address.c_str());
    instance.set_sysvar("super_read_only", true,
                        mysqlshdk::mysql::Var_qualifier::GLOBAL);
  }
}

void ReplicaSet::update_group_members_for_removed_member(
    const std::string &local_gr_address,
    const mysqlshdk::mysql::Instance &instance, bool remove_rpl_user_on_group) {
  // Iterate through all ONLINE and RECOVERING cluster members and update their
  // group_replication_group_seeds value by removing the gr_local_address
  // of the instance that was removed
  std::vector<std::string> ignore_instances_vec;
  Gr_seeds_change_type change_type = Gr_seeds_change_type::REMOVE;

  // It is assumed that the same connection credentials can be used to all the
  // instances in the cluster.
  Connection_options instances_cnx_opts = instance.get_connection_options();

  log_debug("Updating group_replication_group_seeds of cluster members");
  execute_in_members(
      {"'ONLINE'", "'RECOVERING'"}, instances_cnx_opts, ignore_instances_vec,
      [local_gr_address, change_type,
       this](std::shared_ptr<mysqlshdk::db::ISession> session) -> bool {
        update_group_replication_group_seeds(local_gr_address, change_type,
                                             session, this->naming_style);
        return true;
      });

  // Remove the replication users on the instance and members if
  // remove_rpl_user_on_group = true.
  if (remove_rpl_user_on_group) {
    log_debug("Removing replication user on instance and replicaset members");
  } else {
    log_debug("Removing replication user on instance");
  }
  remove_replication_users(instance, remove_rpl_user_on_group);
}

void ReplicaSet::remove_replication_users(
    const mysqlshdk::mysql::Instance &instance, bool remove_rpl_user_on_group) {
  std::string instance_address =
      instance.get_connection_options().as_uri(only_transport());
  // Check if super_read_only is enabled and disable it to remove replication
  // users and metadata.
  mysqlshdk::utils::nullable<bool> super_read_only = instance.get_sysvar_bool(
      "super_read_only", mysqlshdk::mysql::Var_qualifier::GLOBAL);
  if (*super_read_only) {
    log_debug(
        "Disabling super_read_only to remove replication users on "
        "instance '%s'.",
        instance_address.c_str());
    instance.set_sysvar("super_read_only", false,
                        mysqlshdk::mysql::Var_qualifier::GLOBAL);
  }

  // Remove all replication (recovery users) on the removed instance,
  // disabling binary logging (avoid being replicated).
  try {
    // Re-enable super_read_only if previously enabled when leaving "try catch".
    auto finally =
        shcore::on_leave_scope([super_read_only, instance, instance_address]() {
          reenable_super_read_only(super_read_only, instance, instance_address);
        });
    instance.set_sysvar("sql_log_bin", static_cast<const int64_t>(0),
                        mysqlshdk::mysql::Var_qualifier::SESSION);

    log_debug("Removing InnoDB Cluster replication users on instance '%s'.",
              instance_address.c_str());
    instance.drop_users_with_regexp("'mysql_innodb_cluster_r[0-9]{10}.*");

    instance.set_sysvar("sql_log_bin", static_cast<const int64_t>(1),
                        mysqlshdk::mysql::Var_qualifier::SESSION);
  } catch (shcore::Exception &err) {
    throw;
  }

  if (remove_rpl_user_on_group) {
    // Get replication user (recovery) used by the instance to remove
    // on remaining members.
    std::string rpl_user = mysqlshdk::gr::get_recovery_user(instance);
    std::shared_ptr<Cluster> cluster(_cluster.lock());
    if (!cluster)
      throw shcore::Exception::runtime_error(
          "Cluster object is no longer valid");

    mysqlshdk::mysql::Instance primary_instance(cluster->get_group_session());

    // Remove the replication user used by the removed instance on all
    // cluster members through the primary (using replication).
    primary_instance.drop_users_with_regexp(rpl_user + ".*");
  }
}

shcore::Value ReplicaSet::dissolve(const shcore::Argument_list &args) {
  mysqlshdk::utils::nullable<bool> force;
  bool interactive;

  std::shared_ptr<Cluster> cluster(_cluster.lock());
  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");
  interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (args.size() == 1) {
    Unpack_options(args.map_at(0))
        .optional("force", &force)
        .optional("interactive", &interactive)
        .end();
  }

  // Dissolve the ReplicaSet
  try {
    // Create the Dissolve command and execute it.
    Dissolve op_dissolve(interactive, force, cluster, shared_from_this());
    // Always execute finish when leaving "try catch".
    auto finally =
        shcore::on_leave_scope([&op_dissolve]() { op_dissolve.finish(); });
    // Prepare the dissolve command execution (validations).
    op_dissolve.prepare();
    // Execute dissolve operations.
    op_dissolve.execute();
  } catch (...) {
    throw;
  }

  return shcore::Value();
}

namespace {
void unpack_auto_instances_list(
    mysqlsh::Unpack_options *opts_unpack, const std::string &option_name,
    bool *out_auto,
    std::vector<mysqlshdk::db::Connection_options> *instances_list) {
  // Extract value for addInstances, it can be a string "auto" or a list.
  shcore::Array_t instances_array;
  try {
    // Try to extract the "auto" string.
    mysqlshdk::utils::nullable<std::string> auto_option_str;
    opts_unpack->optional(option_name.c_str(), &auto_option_str);

    // Validate if "auto" was specified (case insensitive).
    if (!auto_option_str.is_null() &&
        shcore::str_casecmp(auto_option_str, "auto") == 0) {
      *out_auto = true;
    } else if (!auto_option_str.is_null()) {
      throw shcore::Exception::argument_error(
          "Option '" + option_name +
          "' only accepts 'auto' as a valid string "
          "value, otherwise a list of instances is expected.");
    }
  } catch (const shcore::Exception &err) {
    // Try to extract a list of instances (will fail with a TypeError when
    // trying to read it as a string previously).
    if (std::string(err.type()).compare("TypeError") == 0) {
      opts_unpack->optional(option_name.c_str(), &instances_array);
    } else {
      throw;
    }
  }

  if (instances_array) {
    if (instances_array.get()->empty()) {
      throw shcore::Exception::argument_error("The list for '" + option_name +
                                              "' option cannot be empty.");
    }

    // Process values from addInstances list (must be valid connection data).
    for (const shcore::Value &value : *instances_array.get()) {
      shcore::Argument_list args;
      args.push_back(shcore::Value(value));

      try {
        mysqlshdk::db::Connection_options cnx_opt =
            mysqlsh::get_connection_options(args,
                                            mysqlsh::PasswordFormat::NONE);

        if (cnx_opt.get_host().empty()) {
          throw shcore::Exception::argument_error("host cannot be empty.");
        }

        if (!cnx_opt.has_port()) {
          throw shcore::Exception::argument_error("port is missing.");
        }

        instances_list->push_back(cnx_opt);
      } catch (std::exception &err) {
        std::string error(err.what());
        throw shcore::Exception::argument_error(
            "Invalid value '" + value.descr() + "' for '" + option_name +
            "' option: " + error);
      }
    }
  }
}
}  // namespace

void ReplicaSet::rescan(const shcore::Dictionary_t &options) {
  bool interactive, auto_add_instance = false, auto_remove_instance = false;
  mysqlshdk::utils::nullable<bool> update_topology_mode;
  std::vector<mysqlshdk::db::Connection_options> add_instances_list,
      remove_instances_list;

  std::shared_ptr<Cluster> cluster(_cluster.lock());
  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");

  interactive = current_shell_options()->get().wizards;

  // Get optional options.
  if (options) {
    shcore::Array_t add_instances_array, remove_instances_array;

    auto opts_unpack = Unpack_options(options);
    opts_unpack.optional("updateTopologyMode", &update_topology_mode)
        .optional("interactive", &interactive);

    // Extract value for addInstances, it can be a string "auto" or a list.
    unpack_auto_instances_list(&opts_unpack, "addInstances", &auto_add_instance,
                               &add_instances_list);

    // Extract value for removeInstances, it can be a string "auto" or a list.
    unpack_auto_instances_list(&opts_unpack, "removeInstances",
                               &auto_remove_instance, &remove_instances_list);

    opts_unpack.end();
  }

  // Rescan replicaset.
  {
    // Create the rescan command and execute it.
    Rescan op_rescan(interactive, update_topology_mode, auto_add_instance,
                     auto_remove_instance, add_instances_list,
                     remove_instances_list, cluster, shared_from_this(),
                     this->naming_style);

    // Always execute finish when leaving "try catch".
    auto finally =
        shcore::on_leave_scope([&op_rescan]() { op_rescan.finish(); });

    // Prepare the rescan command execution (validations).
    op_rescan.prepare();

    // Execute rescan operation.
    op_rescan.execute();
  }
}

mysqlshdk::db::Connection_options ReplicaSet::pick_seed_instance() {
  std::shared_ptr<Cluster> cluster(_cluster.lock());
  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");

  bool single_primary;
  std::string primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      cluster->get_group_session(), &single_primary);
  if (single_primary) {
    if (!primary_uuid.empty()) {
      mysqlshdk::utils::nullable<mysqlshdk::innodbcluster::Instance_info> info(
          _metadata_storage->get_new_metadata()->get_instance_info_by_uuid(
              primary_uuid));
      if (info) {
        mysqlshdk::db::Connection_options coptions(info->classic_endpoint);
        mysqlshdk::db::Connection_options group_session_target(
            cluster->get_group_session()->get_connection_options());

        coptions.set_login_options_from(group_session_target);
        coptions.set_ssl_connection_options_from(
            group_session_target.get_ssl_options());

        return coptions;
      }
    }
    throw shcore::Exception::runtime_error(
        "Unable to determine a suitable peer instance to join the group");
  } else {
    // instance we're connected to should be OK if we're multi-master
    return cluster->get_group_session()->get_connection_options();
  }
}

shcore::Value ReplicaSet::check_instance_state(
    const Connection_options &instance_def) {
  // Create the Replicaset_check_instance_state object and execute it.
  Replicaset_check_instance_state op_check_instance_state(*this, instance_def);

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_check_instance_state]() { op_check_instance_state.finish(); });

  // Prepare the Replicaset_check_instance_state command execution
  // (validations).
  op_check_instance_state.prepare();

  // Execute Replicaset_check_instance_state operations.
  return op_check_instance_state.execute();
}

void ReplicaSet::add_instance_metadata(
    const mysqlshdk::db::Connection_options &instance_definition,
    const std::string &label) {
  log_debug("Adding instance to metadata");

  MetadataStorage::Transaction tx(_metadata_storage);

  int port = -1;
  int xport = -1;
  std::string local_gr_address;

  std::string joiner_host;

  // Check if the instance was already added
  std::string instance_address = instance_definition.as_uri(only_transport());

  std::string mysql_server_uuid;
  std::string mysql_server_address;

  log_debug("Connecting to '%s' to query for metadata information...",
            instance_address.c_str());
  // get the server_uuid from the joining instance
  {
    std::shared_ptr<mysqlshdk::db::ISession> classic;
    std::string joiner_user;
    try {
      classic = establish_mysql_session(instance_definition,
                                        current_shell_options()->get().wizards);

      auto options = classic->get_connection_options();
      port = options.get_port();
      joiner_host = options.get_host();
      instance_address = options.as_uri(only_transport());
      joiner_user = options.get_user();
    } catch (Exception &e) {
      std::stringstream ss;
      ss << "Error opening session to '" << instance_address
         << "': " << e.what();
      log_warning("%s", ss.str().c_str());

      // Check if we're adopting a GR cluster, if so, it could happen that
      // we can't connect to it because root@localhost exists but root@hostname
      // doesn't (GR keeps the hostname in the members table)
      if (e.is_mysql() && e.code() == 1045) {  // access denied
        std::stringstream se;
        se << "Access denied connecting to new instance " << instance_address
           << ".\n"
           << "Please ensure all instances in the same group/replicaset have"
              " the same password for account '"
              ""
           << joiner_user
           << "' and that it is accessible from the host mysqlsh is running "
              "from.";
        throw Exception::runtime_error(se.str());
      }
      throw Exception::runtime_error(ss.str());
    }
    {
      // Query UUID of the member and its public hostname
      auto result = classic->query("SELECT @@server_uuid");
      auto row = result->fetch_one();
      if (row) {
        mysql_server_uuid = row->get_as_string(0);
      } else {
        throw Exception::runtime_error("@@server_uuid could not be queried");
      }
    }
    try {
      auto result = classic->query("SELECT @@mysqlx_port");
      auto xport_row = result->fetch_one();
      if (xport_row) xport = xport_row->get_int(0);
    } catch (std::exception &e) {
      log_info(
          "The X plugin is not enabled on instance '%s'. No value will be "
          "assumed for the X protocol address.",
          classic->get_connection_options().uri_endpoint().c_str());
    }

    // Loads the local HR host data
    get_server_variable(classic, "group_replication_local_address",
                        local_gr_address, false);

    // NOTE(rennox): This validation is completely weird, mysql_server_address
    // has not been used so far...
    if (!mysql_server_address.empty() && mysql_server_address != joiner_host) {
      log_info("Normalized address of '%s' to '%s'", joiner_host.c_str(),
               mysql_server_address.c_str());

      instance_address = mysql_server_address + ":" + std::to_string(port);
    } else {
      mysql_server_address = joiner_host;
    }
  }
  std::string instance_xaddress;
  if (xport != -1)
    instance_xaddress = mysql_server_address + ":" + std::to_string(xport);

  Instance_definition instance;

  instance.role = "HA";
  instance.endpoint = instance_address;
  instance.xendpoint = instance_xaddress;
  instance.grendpoint = local_gr_address;
  instance.uuid = mysql_server_uuid;

  instance.label = label.empty() ? instance_address : label;

  // update the metadata with the host
  // NOTE(rennox): the old insert_host function was reading from the map
  // the keys location and id_host (<--- no, is not a typo, id_host)
  // where does those values come from????
  uint32_t host_id = _metadata_storage->insert_host(joiner_host, "", "");

  instance.host_id = host_id;
  instance.replicaset_id = get_id();

  // And the instance
  _metadata_storage->insert_instance(instance);

  tx.commit();
}

void ReplicaSet::remove_instance_metadata(
    const mysqlshdk::db::Connection_options &instance_def) {
  log_debug("Removing instance from metadata");

  MetadataStorage::Transaction tx(_metadata_storage);

  std::string port = std::to_string(instance_def.get_port());

  std::string host = instance_def.get_host();

  // Check if the instance was already added
  std::string instance_address = host + ":" + port;

  _metadata_storage->remove_instance(instance_address);

  tx.commit();
}

std::vector<std::string> ReplicaSet::get_online_instances() {
  std::vector<std::string> online_instances_array;

  auto online_instances =
      _metadata_storage->get_replicaset_online_instances(_id);

  for (auto &instance : online_instances) {
    // TODO: Review if end point is the right thing
    std::string instance_host = instance.endpoint;
    online_instances_array.push_back(instance_host);
  }

  return online_instances_array;
}

#if DOXYGEN_CPP
/**
 * Use this function to restore a ReplicaSet from a Quorum loss scenario
 * \param args : A list of values to be used to use the partition from
 * an Instance to restore the ReplicaSet.
 *
 * This function returns an empty Value.
 */
#else
/**
 * Forces the quorum on ReplicaSet with Quorum loss
 * \param name The name of the Instance to be used as partition to force the
 * quorum on the ReplicaSet
 */
#if DOXYGEN_JS
Undefined ReplicaSet::forceQuorumUsingPartitionOf(InstanceDef instance);
#elif DOXYGEN_PY
None ReplicaSet::force_quorum_using_partition_of(InstanceDef instance);
#endif
#endif  // DOXYGEN_CPP
shcore::Value ReplicaSet::force_quorum_using_partition_of_(
    const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, 2,
                    get_function_name("forceQuorumUsingPartitionOf").c_str());

  // Check if the ReplicaSet is empty
  if (_metadata_storage->is_replicaset_empty(get_id()))
    throw shcore::Exception::runtime_error("ReplicaSet not initialized.");

  // Rejoin the Instance to the Default ReplicaSet
  try {
    ret_val = force_quorum_using_partition_of(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("forceQuorumUsingPartitionOf"));

  return ret_val;
}

shcore::Value ReplicaSet::force_quorum_using_partition_of(
    const shcore::Argument_list &args) {
  shcore::Value ret_val;
  uint64_t rset_id = get_id();
  std::shared_ptr<mysqlshdk::db::ISession> session;

  auto instance_def =
      mysqlsh::get_connection_options(args, PasswordFormat::STRING);

  validate_connection_options(instance_def);

  if (!instance_def.has_port())
    instance_def.set_port(mysqlshdk::db::k_default_mysql_port);

  instance_def.set_default_connection_data();

  std::string instance_address = instance_def.as_uri(only_transport());

  // TODO(miguel): test if there's already quorum and add a 'force' option to be
  // used if so

  // TODO(miguel): test if the instance if part of the current cluster, for the
  // scenario of restoring a cluster quorum from another

  // Check if the instance belongs to the ReplicaSet on the Metadata
  if (!_metadata_storage->is_instance_on_replicaset(rset_id,
                                                    instance_address)) {
    std::string message = "The instance '" + instance_address + "'";

    message.append(" does not belong to the ReplicaSet: '" +
                   get_member("name").get_string() + "'.");

    throw shcore::Exception::runtime_error(message);
  }

  // Before rejoining an instance we must verify if the instance's
  // 'group_replication_group_name' matches the one registered in the
  // Metadata (BUG #26159339)
  {
    try {
      log_info("Opening a new session to the partition instance %s",
               instance_address.c_str());
      session = establish_mysql_session(instance_def,
                                        current_shell_options()->get().wizards);
      instance_def = session->get_connection_options();
    } catch (std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_address.c_str(), e.what());
      throw;
    }

    if (!validate_replicaset_group_name(session, get_group_name())) {
      std::string nice_error =
          "The instance '" + instance_address +
          "' "
          "cannot be used to restore the cluster as it "
          "may belong to a different ReplicaSet as the one registered "
          "in the Metadata since the value of "
          "'group_replication_group_name' does not match the one "
          "registered in the ReplicaSet's Metadata: possible split-brain "
          "scenario.";

      session->close();

      throw shcore::Exception::runtime_error(nice_error);
    }
  }

  // Get the instance state
  Cluster_check_info state;

  auto instance_type = get_gr_instance_type(session);

  if (instance_type != GRInstanceType::Standalone &&
      instance_type != GRInstanceType::StandaloneWithMetadata) {
    state = get_replication_group_state(session, instance_type);

    if (state.source_state != ManagedInstance::OnlineRW &&
        state.source_state != ManagedInstance::OnlineRO) {
      std::string message = "The instance '" + instance_address + "'";
      message.append(" cannot be used to restore the cluster as it is on a ");
      message.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      message.append(" state, and should be ONLINE");

      session->close();

      throw shcore::Exception::runtime_error(message);
    }
  } else {
    std::string message = "The instance '" + instance_address + "'";
    message.append(
        " cannot be used to restore the cluster as it is not an active member "
        "of replication group.");

    session->close();

    throw shcore::Exception::runtime_error(message);
  }

  // Check if there is quorum to issue an error.
  mysqlshdk::mysql::Instance target_instance(session);
  if (mysqlshdk::gr::has_quorum(target_instance, nullptr, nullptr)) {
    mysqlsh::current_console()->print_error(
        "Cannot perform operation on an healthy cluster because it can only "
        "be used to restore a cluster from quorum loss.");

    target_instance.close_session();

    throw shcore::Exception::runtime_error(
        "The cluster has quorum according to instance '" + instance_address +
        "'");
  }

  // Get the online instances of the ReplicaSet to user as group_peers
  auto online_instances =
      _metadata_storage->get_replicaset_online_instances(rset_id);

  if (online_instances.empty()) {
    session->close();
    throw shcore::Exception::logic_error(
        "No online instances are visible from the given one.");
  }

  std::string group_peers;

  for (auto &instance : online_instances) {
    std::string instance_host = instance.endpoint;
    auto target_coptions = shcore::get_connection_options(instance_host, false);
    // We assume the login credentials are the same on all instances
    target_coptions.set_login_options_from(instance_def);

    std::shared_ptr<mysqlshdk::db::ISession> instance_session;
    try {
      log_info(
          "Opening a new session to a group_peer instance to obtain the XCOM "
          "address %s",
          instance_host.c_str());
      instance_session = establish_mysql_session(
          target_coptions, current_shell_options()->get().wizards);
    } catch (std::exception &e) {
      log_error("Could not open connection to %s: %s", instance_address.c_str(),
                e.what());
      session->close();
      throw;
    }

    std::string group_peer_instance_xcom_address;

    // Get @@group_replication_local_address
    get_server_variable(instance_session, "group_replication_local_address",
                        group_peer_instance_xcom_address);

    group_peers.append(group_peer_instance_xcom_address);
    group_peers.append(",");

    instance_session->close();
  }

  // Force the reconfiguration of the GR group
  {
    // Remove the trailing comma of group_peers
    if (group_peers.back() == ',') group_peers.pop_back();

    log_info("Setting the group_replication_force_members at instance %s",
             instance_address.c_str());

    // Setting the group_replication_force_members will force a new group
    // membership, triggering the necessary actions from GR upon being set to
    // force the quorum. Therefore, the variable can be cleared immediately
    // after it is set.
    set_global_variable(session, "group_replication_force_members",
                        group_peers);

    // Clear group_replication_force_members at the end to allow GR to be
    // restarted later on the instance (without error).
    set_global_variable(session, "group_replication_force_members", "");

    session->close();
  }

  return ret_val;
}

void ReplicaSet::switch_to_single_primary_mode(
    const Connection_options &instance_def) {
  std::shared_ptr<Cluster> cluster(_cluster.lock());

  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");

  // Switch to single-primary mode

  // Create the Switch_to_single_primary_mode object and execute it.
  Switch_to_single_primary_mode op_switch_to_single_primary_mode(
      instance_def, cluster, shared_from_this(), this->naming_style);

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope([&op_switch_to_single_primary_mode]() {
    op_switch_to_single_primary_mode.finish();
  });

  // Prepare the Switch_to_single_primary_mode command execution (validations).
  op_switch_to_single_primary_mode.prepare();

  // Execute Switch_to_single_primary_mode operation.
  op_switch_to_single_primary_mode.execute();
}

void ReplicaSet::switch_to_multi_primary_mode(void) {
  std::shared_ptr<Cluster> cluster(_cluster.lock());

  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");

  // Switch to multi-primary mode

  // Create the Switch_to_multi_primary_mode object and execute it.
  Switch_to_multi_primary_mode op_switch_to_multi_primary_mode(
      cluster, shared_from_this(), this->naming_style);

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope([&op_switch_to_multi_primary_mode]() {
    op_switch_to_multi_primary_mode.finish();
  });

  // Prepare the Switch_to_multi_primary_mode command execution (validations).
  op_switch_to_multi_primary_mode.prepare();

  // Execute Switch_to_multi_primary_mode operation.
  op_switch_to_multi_primary_mode.execute();
}

void ReplicaSet::set_primary_instance(const Connection_options &instance_def) {
  std::shared_ptr<Cluster> cluster(_cluster.lock());

  if (!cluster)
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");

  // Set primary instance

  // Create the Set_primary_instance object and execute it.
  Set_primary_instance op_set_primary_instance(
      instance_def, cluster, shared_from_this(), this->naming_style);

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_set_primary_instance]() { op_set_primary_instance.finish(); });

  // Prepare the Set_primary_instance command execution (validations).
  op_set_primary_instance.prepare();

  // Execute Set_primary_instance operation.
  op_set_primary_instance.execute();
}

Cluster_check_info ReplicaSet::check_preconditions(
    std::shared_ptr<mysqlshdk::db::ISession> group_session,
    const std::string &function_name) const {
  try {
    return check_function_preconditions("ReplicaSet." + function_name,
                                        group_session);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name(function_name));
  return Cluster_check_info{};
}

shcore::Value ReplicaSet::get_description() const {
  shcore::Value ret_val = shcore::Value::new_map();
  auto description = ret_val.as_map();

  // First we identify the master instance
  auto instances = _metadata_storage->get_replicaset_instances(_id);

  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  std::string gr_primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      _metadata_storage->get_session(), nullptr);

  std::string topology_mode =
      !gr_primary_uuid.empty()
          ? mysqlshdk::gr::to_string(
                mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY)
          : mysqlshdk::gr::to_string(
                mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

  (*description)["name"] = shcore::Value(_name);
  (*description)["topologyMode"] = shcore::Value(topology_mode);
  (*description)["topology"] = shcore::Value::new_array();

  auto instance_list = description->get_array("topology");

  for (const auto &value : instances) {
    auto instance = shcore::Value::new_map();
    auto instance_obj = instance.as_map();

    (*instance_obj)["label"] = shcore::Value(value.label);
    (*instance_obj)["address"] = shcore::Value(value.endpoint);
    (*instance_obj)["role"] = shcore::Value(value.role);

    instance_list->push_back(instance);
  }

  return ret_val;
}

void ReplicaSet::remove_instances(
    const std::vector<std::string> &remove_instances) {
  if (!remove_instances.empty()) {
    for (auto instance : remove_instances) {
      // verify if the instance is on the metadata
      if (_metadata_storage->is_instance_on_replicaset(_id, instance)) {
        Value::Map_type_ref options(new shcore::Value::Map_type);

        auto connection_options =
            shcore::get_connection_options(instance, false);
        remove_instance_metadata(connection_options);
      } else {
        std::string message = "The instance '" + instance + "'";
        message.append(" does not belong to the ReplicaSet: '" +
                       get_member("name").get_string() + "'.");
        throw shcore::Exception::runtime_error(message);
      }
    }
  }
}

void ReplicaSet::rejoin_instances(
    const std::vector<std::string> &rejoin_instances,
    const shcore::Value::Map_type_ref &options) {
  auto instance_session(_metadata_storage->get_session());
  auto instance_data = instance_session->get_connection_options();

  if (!rejoin_instances.empty()) {
    // Get the user and password from the options
    // or from the instance session
    if (options) {
      // Check if the password is specified on the options and if not prompt it
      mysqlsh::set_user_from_map(&instance_data, options);
      mysqlsh::set_password_from_map(&instance_data, options);
    }

    for (auto instance : rejoin_instances) {
      // verify if the instance is on the metadata
      if (_metadata_storage->is_instance_on_replicaset(_id, instance)) {
        auto connection_options =
            shcore::get_connection_options(instance, false);

        connection_options.set_user(instance_data.get_user());
        connection_options.set_password(instance_data.get_password());

        // If rejoinInstance fails we don't want to stop the execution of the
        // function, but to log the error.
        try {
          std::string msg = "Rejoining the instance '" + instance +
                            "' to the "
                            "cluster's default replicaset.";
          log_warning("%s", msg.c_str());
          rejoin_instance(&connection_options, shcore::Value::Map_type_ref());
        } catch (shcore::Exception &e) {
          log_error("Failed to rejoin instance: %s", e.what());
        }
      } else {
        std::string msg =
            "The instance '" + instance +
            "' does not "
            "belong to the cluster. Skipping rejoin to the Cluster.";
        throw shcore::Exception::runtime_error(msg);
      }
    }
  }
}

/**
 * Check the instance server UUID of the specified intance.
 *
 * The server UUID must be unique for all instances in a cluster. This function
 * checks if the server_uuid of the target instance is unique among all active
 * members of the cluster.
 *
 * @param instance_session Session to the target instance to check its server
 *                         UUID.
 */
void ReplicaSet::validate_server_uuid(
    std::shared_ptr<mysqlshdk::db::ISession> instance_session) {
  // Get the server_uuid of the target instance.
  auto instance = mysqlshdk::mysql::Instance(instance_session);
  std::string server_uuid = instance.get_sysvar_string(
      "server_uuid", mysqlshdk::mysql::Var_qualifier::GLOBAL);

  // Get connection option for the metadata.
  std::shared_ptr<Cluster> cluster(_cluster.lock());
  if (!cluster) {
    throw shcore::Exception::runtime_error("Cluster object is no longer valid");
  }
  std::shared_ptr<mysqlshdk::db::ISession> cluster_session =
      cluster->get_group_session();
  Connection_options cluster_cnx_opt =
      cluster_session->get_connection_options();

  // Get list of instances in the metadata
  std::vector<Instance_definition> metadata_instances =
      _metadata_storage->get_replicaset_active_instances(_id);

  // Get and compare the server UUID of all instances with the one of
  // the target instance.
  for (Instance_definition &instance_def : metadata_instances) {
    if (server_uuid == instance_def.uuid) {
      // Raise an error if the server uuid is the same of a cluster member.
      throw Exception::runtime_error(
          "Cannot add an instance with the same server UUID (" + server_uuid +
          ") of an active member of the cluster '" + instance_def.endpoint +
          "'. Please change the server UUID of the instance to add, all "
          "members must have a unique server UUID.");
    }
  }
}

std::vector<Instance_definition> ReplicaSet::get_instances_from_metadata() {
  return _metadata_storage->get_replicaset_instances(get_id());
}

std::vector<ReplicaSet::Instance_info> ReplicaSet::get_instances() {
  return _metadata_storage->get_new_metadata()->get_replicaset_instances(
      get_id());
}

/** Iterates through all the cluster members in a given state calling the given
 * function on each of then.
 * @param states Vector of strings with the states of members on which the
 * functor will be called.
 * @param cnx_opt Connection options to be used to connect to the cluster
 * members
 * @param ignore_instances_vector Vector with addresses of instances to be
 * ignored even if their state is specified in the states vector.
 * @param functor Function that is called on each member of the cluster whose
 * state is specified in the states vector.
 */

void ReplicaSet::execute_in_members(
    const std::vector<std::string> &states,
    const mysqlshdk::db::Connection_options &cnx_opt,
    const std::vector<std::string> &ignore_instances_vector,
    std::function<bool(std::shared_ptr<mysqlshdk::db::ISession> session)>
        functor,
    bool ignore_network_conn_errors) {
  const int kNetworkConnRefused = 2003;
  std::shared_ptr<mysqlshdk::db::ISession> instance_session;
  // Note (nelson): should we handle the super_read_only behavior here or should
  // it be the responsibility of the functor?
  auto instance_definitions =
      _metadata_storage->get_replicaset_instances(_id, false, states);

  for (auto &instance_def : instance_definitions) {
    std::string instance_address = instance_def.endpoint;
    // if instance is on the list of instances to be ignored, skip it
    if (std::find(ignore_instances_vector.begin(),
                  ignore_instances_vector.end(),
                  instance_address) != ignore_instances_vector.end())
      continue;
    auto target_coptions =
        shcore::get_connection_options(instance_address, false);

    target_coptions.set_login_options_from(cnx_opt);
    try {
      log_debug(
          "Opening a new session to instance '%s' while iterating "
          "cluster members",
          instance_address.c_str());
      instance_session = establish_mysql_session(
          target_coptions, current_shell_options()->get().wizards);
    } catch (mysqlshdk::db::Error &e) {
      if (ignore_network_conn_errors && e.code() == kNetworkConnRefused) {
        log_error("Could not open connection to '%s': %s, but ignoring it.",
                  instance_address.c_str(), e.what());
        continue;
      } else {
        log_error("Could not open connection to '%s': %s",
                  instance_address.c_str(), e.what());
        throw;
      }
    } catch (std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_address.c_str(), e.what());
      throw;
    }
    bool continue_loop = functor(instance_session);
    instance_session->close();
    if (!continue_loop) {
      log_debug("Cluster iteration stopped because functor returned false.");
      break;
    }
  }
}

void ReplicaSet::set_group_name(const std::string &group_name) {
  _group_name = group_name;
  _metadata_storage->set_replicaset_group_name(shared_from_this(), group_name);
}
