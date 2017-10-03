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

#include "modules/adminapi/mod_dba_replicaset.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
//#include "modules/adminapi/mod_dba_instance.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_sql.h"

#include "modules/mod_mysql_session.h"
#include "modules/base_session.h"

#include "common/uuid/include/uuid_gen.h"
#include "utils/utils_general.h"
#include "modules/mysqlxtest_utils.h"
#include "xerrmsg.h"
#include "shellcore/shell_core_options.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_mysql_resultset.h"
#include "utils/utils_time.h"
#include "logger/logger.h"
#include "utils/utils_sqlstring.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#ifdef _WIN32
#define strerror_r(errno,buf,len) strerror_s(buf,len,errno)
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::dba;
using namespace shcore;

std::set<std::string> ReplicaSet::_add_instance_opts = {"label", "password", "dbPassword", "memberSslMode", "ipWhitelist"};
std::set<std::string> ReplicaSet::_remove_instance_opts = {"password", "dbPassword", "force"};

char const *ReplicaSet::kTopologyPrimaryMaster = "pm";
char const *ReplicaSet::kTopologyMultiMaster = "mm";

static const std::string kSandboxDatadir = "sandboxdata";

ReplicaSet::ReplicaSet(const std::string &name, const std::string &topology_type,
                       std::shared_ptr<MetadataStorage> metadata_storage) :
  _name(name), _topology_type(topology_type), _metadata_storage(metadata_storage) {
  assert(topology_type == kTopologyMultiMaster || topology_type == kTopologyPrimaryMaster);
  init();
}

ReplicaSet::~ReplicaSet() {}

std::string &ReplicaSet::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const {
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

static void append_member_status(const shcore::Value::Map_type_ref& node,
                                 std::shared_ptr<mysqlsh::Row> member_row,
                                 bool read_write,
                                 bool active_session_instance) {
  (*node)["address"] = member_row->get_member(4);

  auto status = member_row->get_member(3);
  (*node)["status"] = status ? status : shcore::Value("(MISSING)");
  (*node)["role"] = member_row->get_member(2);
  (*node)["mode"] = shcore::Value(read_write ? "R/W" : "R/O");
}

bool ReplicaSet::operator == (const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function.
 * The content of the returned value depends on the property being requested.
 * The next list shows the valid properties as well as the returned value for each of them:
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

void ReplicaSet::init() {
  add_property("name", "getName");
  add_varargs_method("addInstance", std::bind(&ReplicaSet::add_instance_, this, _1));
  add_varargs_method("rejoinInstance", std::bind(&ReplicaSet::rejoin_instance_, this, _1));
  add_varargs_method("removeInstance", std::bind(&ReplicaSet::remove_instance_, this, _1));
  add_varargs_method("disable", std::bind(&ReplicaSet::disable, this, _1));
  add_varargs_method("dissolve", std::bind(&ReplicaSet::dissolve, this, _1));
  add_varargs_method("checkInstanceState", std::bind(&ReplicaSet::check_instance_state, this, _1));
  add_varargs_method("forceQuorumUsingPartitionOf", std::bind(&ReplicaSet::force_quorum_using_partition_of_, this, _1));
}

/*
 * Verify if the topology type changed and issue an error if needed.
 */
void ReplicaSet::verify_topology_type_change() const {
  // Get GR single primary mode value.
  int gr_primary_mode;
  auto instance_session(_metadata_storage->get_session());
  auto classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(instance_session.get());
  get_server_variable(classic->connection(),
                      "group_replication_single_primary_mode", gr_primary_mode);

  // Check if the topology type matches the real settings used by the
  // cluster instance, otherwise an error is issued.
  // NOTE: The GR primary mode is guaranteed (by GR) to be the same for all
  // instance of the same group.
  if (gr_primary_mode == 1 && _topology_type == kTopologyMultiMaster)
    throw shcore::Exception::runtime_error(
        "The InnoDB Cluster topology type (Multi-Master) does not match the "
            "current Group Replication configuration (Single-Master). Please "
            "use <cluster>.rescan() or change the Group Replication "
            "configuration accordingly.");
  else if (gr_primary_mode == 0 && _topology_type == kTopologyPrimaryMaster)
    throw shcore::Exception::runtime_error(
        "The InnoDB Cluster topology type (Single-Master) does not match the "
            "current Group Replication configuration (Multi-Master). Please "
            "use <cluster>.rescan() or change the Group Replication "
            "configuration accordingly.");
}

void ReplicaSet::adopt_from_gr() {
  shcore::Value ret_val;

  auto newly_discovered_instances_list(
      get_newly_discovered_instances(_metadata_storage, _id));

  // Add all instances to the cluster metadata
  for (NewInstanceInfo &instance : newly_discovered_instances_list) {
    Value::Map_type_ref newly_discovered_instance(new shcore::Value::Map_type);
    (*newly_discovered_instance)["host"] = shcore::Value(instance.host);
    (*newly_discovered_instance)["port"] = shcore::Value(instance.port);

    log_info("Adopting member %s:%d from existing group",
              instance.host.c_str(),
              instance.port);

    // TODO: what if the password is different on each server?
    // And what if is different from the current session?
    auto session = _metadata_storage->get_session();
    (*newly_discovered_instance)["user"] = shcore::Value(session->get_user());
    (*newly_discovered_instance)["password"] = shcore::Value(session->get_password());

    add_instance_metadata(newly_discovered_instance);
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
    throw shcore::Exception::runtime_error("ReplicaSet not initialized. Please add the Seed Instance using: addSeedInstance().");

  // Add the Instance to the Default ReplicaSet
  try {
    ret_val = add_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  return ret_val;
}

/**
 * Validate whether the hostname cannot be used for setting up a cluster.
 * Basically, a local address can only be used if it's a sandbox.
 */
static bool check_if_local_host(const std::string &hostname) {
  if (is_local_host(hostname, false)) {
    return true;
  } else {
    struct hostent *he = NULL;
    // if the host is not local, we try to resolve it and see if it points to
    // a loopback
    he = gethostbyname(hostname.c_str());
    if (he) {
      for (struct in_addr **h = (struct in_addr**)he->h_addr_list; *h; ++h) {
        const char *addr = inet_ntoa(**h);
        if (strncmp(addr, "127.", 4) == 0) {
          log_info("'%s' is a loopback address '%s'",
                   hostname.c_str(), addr);
          return true;
        }
      }
    }
    // we can't be sure that the address is actually valid here (unless we
    // traverse DNS explicitly), but we'll assume it is and check if the
    // server has something different configured
    return false;
  }
}

void ReplicaSet::validate_instance_address(std::shared_ptr<mysqlsh::mysql::ClassicSession> session,
    const std::string &hostname, int port) {
  if (check_if_local_host(hostname)) {
    // if the address is local (localhost or 127.0.0.1), we know it's local and so
    // can be used with sandboxes only
    std::string datadir = session->execute_sql("SELECT @@datadir")->fetch_one()->get_value_as_string(0);
    if (!datadir.empty() && (datadir[datadir.size() - 1] == '/' || datadir[datadir.size() - 1] == '\\'))
      datadir.pop_back();
    if (datadir.length() < kSandboxDatadir.length() ||
        datadir.compare(datadir.length() - kSandboxDatadir.length(), kSandboxDatadir.length(), kSandboxDatadir) != 0) {
      log_info("'%s' is a local address but not in a sandbox (datadir %s)",
               hostname.c_str(), datadir.c_str());
      throw shcore::Exception::runtime_error(
         "To add an instance to the cluster, please use a valid, non-local hostname or IP. "
         + hostname + " can only be used with sandbox MySQL instances.");
    } else {
      log_warning(
          "'%s' (%s) detected as local sandbox. Sandbox instances are only "
          "suitable for deploying and running on your local machine for "
          "testing purposes and are not accessible from external networks.",
          hostname.c_str(), datadir.c_str());
    }
  } else {
    auto result = session->execute_sql("select @@report_host, @@hostname");
    auto row = result->fetch_one();
    // host is not set explicitly by the user, so GR will pick hostname by default
    // now we check if this is a loopback address
    if (!row->get_value(0)) {
      if (check_if_local_host(row->get_value_as_string(1))) {
        std::string msg = "MySQL server reports hostname as being '" + row->get_value_as_string(1) + "', which may cause the cluster to be inaccessible externally. Please set report_host in MySQL to fix this.";
        log_warning("%s", msg.c_str());
      }
    }
  }
}

shcore::Value ReplicaSet::add_instance(const shcore::Argument_list &args,
                                       const std::string &existing_replication_user,
                                       const std::string &existing_replication_password,
                                       bool overwrite_seed,
                                       const std::string &group_name) {
  shcore::Value ret_val;

  bool seed_instance = false;
  std::string ssl_mode = dba::kMemberSSLModeAuto; //SSL Mode AUTO by default
  std::string ip_whitelist;
  std::string instance_label;

  // NOTE: This function is called from either the add_instance_ on this class
  //       or the add_instance in Cluster class, hence this just throws exceptions
  //       and the proper handling is done on the caller functions (to append the called function name)

  // Check if we're on a addSeedInstance or not
  if (_metadata_storage->is_replicaset_empty(_id)) seed_instance = true;

  // Check if we need to overwrite the seed instance
  if (overwrite_seed) seed_instance = true;

  // Retrieves the instance definition
  auto instance_def = get_instance_options_map(args, mysqlsh::dba::PasswordFormat::OPTIONS);
  shcore::Argument_map instance_map(*instance_def);
  instance_map.ensure_keys({"host"},
                           shcore::connection_attributes,
                           "instance definition");

  // Retrieves the add options
  if (args.size() == 2 ) {
    auto add_options = args.map_at(1);
    shcore::Argument_map add_instance_map(*add_options);
    add_instance_map.ensure_keys({}, _add_instance_opts, " options");

    // Validate SSL options for the cluster instance
    validate_ssl_instance_options(add_options);

    //Validate ip whitelist option
    validate_ip_whitelist_option(add_options);

    if (add_options->has_key("memberSslMode"))
      ssl_mode = add_options->get_string("memberSslMode");

    if (add_options->has_key("ipWhitelist"))
      ip_whitelist = add_options->get_string("ipWhitelist");

    if (add_options->has_key("label")) {
      instance_label = add_options->get_string("label");
      mysqlsh::dba::validate_label(instance_label);
    }
  }

  if (!instance_def->has_key("port"))
    (*instance_def)["port"] = shcore::Value(get_default_port());

  // Sets a default user if not specified
  resolve_instance_credentials(instance_def, nullptr);
  std::string
      user = instance_def->get_string(instance_def->has_key("user") ? "user" : "dbUser");
  std::string super_user_password =
      instance_def->get_string(instance_def->has_key("password") ? "password"
                                                       : "dbPassword");
  std::string joiner_host = instance_def->get_string("host");

  std::string instance_address = joiner_host + ":" + std::to_string(instance_def->get_int("port"));
  bool is_instance_on_md = _metadata_storage->is_instance_on_replicaset(get_id(), instance_address);
  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_def));
  auto session = Dba::get_session(new_args);

  // Check whether the address being used is not in a known not-good case
  validate_instance_address(session, joiner_host, instance_def->get_int("port"));

  // Check replication filters before creating the Metadata.
  validate_replication_filters(session.get());

  // Resolve the SSL Mode to use to configure the instance.
  boost::to_upper(ssl_mode);
  std::string new_ssl_mode;
  std::string target;
  if (seed_instance) {
    new_ssl_mode = resolve_cluster_ssl_mode(session.get(), ssl_mode);
    target = "cluster";
  } else {
    auto peer_session = dynamic_cast<mysqlsh::mysql::ClassicSession*>(
        _metadata_storage->get_session().get());
    new_ssl_mode = resolve_instance_ssl_mode(session.get(),
                                             peer_session,
                                             ssl_mode);
    target = "instance";
  }

  if (new_ssl_mode != ssl_mode) {
    ssl_mode = new_ssl_mode;
    log_warning("SSL mode used to configure the %s: '%s'",
                target.c_str(),
                ssl_mode.c_str());
  }


  GRInstanceType type = get_gr_instance_type(session->connection());

  if (type != GRInstanceType::Standalone) {
    // Retrieves the new instance UUID
    std::string uuid;
    get_server_variable(session->connection(), "server_uuid", uuid);
    session->close(shcore::Argument_list());

    // Verifies if the instance is part of the cluster replication group
    auto cluster_session = _metadata_storage->get_session();
    auto cluster_classic_session = std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(cluster_session);

    // Verifies if this UUID is part of the current replication group
    if (is_server_on_replication_group(cluster_classic_session->connection(), uuid)) {
      if (type == GRInstanceType::InnoDBCluster) {
        log_debug("Instance '%s' already managed by InnoDB cluster", instance_address.c_str());
        throw shcore::Exception::runtime_error("The instance '" + instance_address + "' is already part of this InnoDB cluster");
      }
      else
        log_debug("Instance '%s' is already part of a Replication Group, but not managed", instance_address.c_str());
    }
    else {
      if (type == GRInstanceType::InnoDBCluster)
        throw shcore::Exception::runtime_error("The instance '" + instance_address + "' is already part of another InnoDB cluster");
      else
        throw shcore::Exception::runtime_error("The instance '" + instance_address + "' is already part of another Replication Group");
    }
  } else {
    session->close(shcore::Argument_list());
  }

  log_debug("RS %lu: Adding instance '%s' to replicaset%s",
            static_cast<unsigned long>(_id), instance_address.c_str(),
            is_instance_on_md ? " (already in MD)" : "");

  if (type == GRInstanceType::Standalone) {
    log_debug("Instance '%s' is not yet in the cluster", instance_address.c_str());

    std::string replication_user(existing_replication_user);
    std::string replication_user_password(existing_replication_password);

    // Creates the replication user ONLY if not already given
    if (replication_user.empty()) {
      _metadata_storage->create_repl_account(replication_user, replication_user_password);
      log_debug("Created replication user '%s'", replication_user.c_str());
    }

      // Get SSL values to connect to instance
      Value::Map_type_ref instance_ssl_opts(new shcore::Value::Map_type);
      if (instance_def->has_key("sslCa"))
        (*instance_ssl_opts)["sslCa"] = Value(instance_def->get_string("sslCa"));
      if (instance_def->has_key("sslCert"))
        (*instance_ssl_opts)["sslCert"] = Value(instance_def->get_string("sslCert"));
      if (instance_def->has_key("sslKey"))
        (*instance_ssl_opts)["sslKey"] = Value(instance_def->get_string("sslKey"));

    // Call the gadget to bootstrap the group with this instance
    if (seed_instance) {
      log_info("Joining '%s' to group using account %s@%s",
          instance_address.c_str(),
          user.c_str(), instance_address.c_str());
      log_info("Using 'group_replication_group_name': %s",
          group_name.c_str());
      // Call mysqlprovision to bootstrap the group using "start"
      do_join_replicaset(user + "@" + instance_address,
                         instance_ssl_opts,
                         "",
                         nullptr,
                         super_user_password,
                         replication_user, replication_user_password,
                         ssl_mode, ip_whitelist, group_name);
    } else {
      // We need to retrieve a peer instance, so let's use the Seed one
      std::string peer_instance = get_peer_instance();

        //Get SSL values to connect to peer instance
        Value::Map_type_ref peer_instance_ssl_opts(new shcore::Value::Map_type);
        auto peer_session = _metadata_storage->get_session();
        std::string peer_ssl_ca = peer_session->get_ssl_ca();
        std::string peer_ssl_cert = peer_session->get_ssl_cert();
        std::string peer_ssl_key = peer_session->get_ssl_key();
        if (!peer_ssl_ca.empty())
          (*peer_instance_ssl_opts)["sslCa"] = Value(peer_ssl_ca);
        if (!peer_ssl_cert.empty())
          (*peer_instance_ssl_opts)["sslCert"] = Value(peer_ssl_cert);
        if (!peer_ssl_key.empty())
          (*peer_instance_ssl_opts)["sslKey"] = Value(peer_ssl_key);

      log_info("Joining '%s' to group using account %s@%s to peer '%s'",
          instance_address.c_str(),
          user.c_str(), instance_address.c_str(), peer_instance.c_str());
      // Call mysqlprovision to do the work
      do_join_replicaset(user + "@" + instance_address,
                           instance_ssl_opts,
                          user + "@" + peer_instance,
                           peer_instance_ssl_opts,
                          super_user_password,
                          replication_user, replication_user_password,
                           ssl_mode, ip_whitelist);
    }
  }

  // If the instance is not on the Metadata, we must add it
  if (!is_instance_on_md)
    add_instance_metadata(instance_def, instance_label);

  log_debug("Instance add finished");

  return ret_val;
}

bool ReplicaSet::do_join_replicaset(const std::string &instance_url,
                                    const shcore::Value::Map_type_ref &instance_ssl,
                                    const std::string &peer_instance_url,
                                    const shcore::Value::Map_type_ref &peer_instance_ssl,
                                    const std::string &super_user_password,
                                    const std::string &repl_user,
                                    const std::string &repl_user_password,
                                    const std::string &ssl_mode,
                                    const std::string &ip_whitelist,
                                    const std::string &group_name) {
  shcore::Value ret_val;
  int exit_code = -1;

  bool is_seed_instance = peer_instance_url.empty();

  std::string command;
  shcore::Value::Array_type_ref errors, warnings;

  if (is_seed_instance) {
    exit_code = _cluster->get_provisioning_interface()->start_replicaset(instance_url,
                instance_ssl,
                repl_user, super_user_password,
                repl_user_password,
                _topology_type == kTopologyMultiMaster,
                ssl_mode, ip_whitelist,
                group_name,
                errors);
  } else {
    exit_code = _cluster->get_provisioning_interface()->join_replicaset(instance_url,
                instance_ssl,
                repl_user, peer_instance_url, peer_instance_ssl,
                super_user_password, repl_user_password,
                ssl_mode, ip_whitelist, "", false,
                errors);
  }

  if (exit_code == 0) {
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
      ret_val = shcore::Value("The instance '" + instance_url + "' was successfully added as seeding instance to the MySQL Cluster.");
    else
      ret_val = shcore::Value("The instance '" + instance_url + "' was successfully added to the MySQL Cluster.");
  } else
    throw shcore::Exception::runtime_error(get_mysqlprovision_error_string(errors));

  return exit_code == 0;
}

#if DOXYGEN_CPP
/**
 * Use this function to rejoin an Instance to the ReplicaSet
 * \param args : A list of values to be used to add a Instance to the ReplicaSet.
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
      "ReplicaSet not initialized. Please add the Seed Instance using: addSeedInstance().");

  // Rejoin the Instance to the Default ReplicaSet
  try {
    ret_val = rejoin_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  return ret_val;
}

shcore::Value ReplicaSet::rejoin_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string instance_host;
  int instance_port = 0;
  // SSL Mode AUTO by default
  std::string ssl_mode = mysqlsh::dba::kMemberSSLModeAuto;
  std::string ip_whitelist;
  shcore::Value::Array_type_ref errors;
  mysqlsh::mysql::ClassicSession *classic;
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> session;

  // Get the instance definition
  auto instance_def = get_instance_options_map(args,
                        mysqlsh::dba::PasswordFormat::OPTIONS);
  shcore::Argument_map instance_map(*instance_def);
  instance_map.ensure_keys({"host"},
                           shcore::connection_attributes,
                           "instance definition");

  // Retrieves the options
  if (args.size() == 2) {
    auto rejoin_options = args.map_at(1);
    shcore::Argument_map rejoin_instance_map(*rejoin_options);
    rejoin_instance_map.ensure_keys({}, _add_instance_opts, " options");

    // Validate SSL options for the cluster instance
    validate_ssl_instance_options(rejoin_options);

    // Validate ip whitelist option
    validate_ip_whitelist_option(rejoin_options);

    if (rejoin_options->has_key("memberSslMode"))
      ssl_mode = rejoin_options->get_string("memberSslMode");

    if (rejoin_options->has_key("ipWhitelist"))
      ip_whitelist = rejoin_options->get_string("ipWhitelist");

  }

  if (!instance_def->has_key("port"))
    (*instance_def)["port"] = shcore::Value(get_default_port());

  instance_port = instance_def->get_int("port");
  instance_host = instance_def->get_string("host");

  std::string instance_address = instance_host + ":" +
                                 std::to_string(instance_port);

  // Check if the instance is part of the Metadata
  if (!_metadata_storage->is_instance_on_replicaset(get_id(),
                                                    instance_address)) {
    std::string message = "The instance '" + instance_address + "' " +
                          "does not belong to the ReplicaSet: '" +
                          get_member("name").as_string() + "'.";

    throw shcore::Exception::runtime_error(message);
  }

  // Before rejoining an instance we must verify if the instance's
  // 'group_replication_group_name' matches the one registered in the
  // Metadata (BUG #26159339)
  //
  // Before rejoining an instance we must also verify if the group has quorum
  // and if the gr plugin is active otherwise we may end up hanging the system

  // In single-primary mode, the active session must be established to the seed
  // instance in order to be able to make changes on the cluster.
  // So we must obtain the active session credentials in order to do the follow
  // validations.
  // In multi-primary mode, any instance is suitable, so we can also use the
  // active session.

  // Get the current cluster session from the metadata
  auto seed_session(_metadata_storage->get_session());

  // Get the rejoining instance definition
  // Sets a default user if not specified
  resolve_instance_credentials(instance_def, nullptr);
  std::string instance_password = instance_def->get_string(
    instance_def->has_key("password") ? "password" : "dbPassword");
  std::string instance_user = instance_def->get_string(
    instance_def->has_key("user") ? "user" : "dbUser");

  // Validate 'group_replication_group_name'
  {
    try {
      log_info("Opening a new session to the rejoining instance %s",
               instance_address.c_str());
      shcore::Argument_list slave_args;
      slave_args.push_back(shcore::Value(instance_def));
      session = mysqlsh::connect_session(slave_args,
                                         mysqlsh::SessionType::Classic);
      classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
    } catch (std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_address.c_str(), e.what());
      throw;
    }

    if (!validate_replicaset_group_name(
            _metadata_storage, classic, _id)) {
      std::string nice_error =
          "The instance '" + instance_address + "' "\
          "may belong to a different ReplicaSet as the one registered "\
          "in the Metadata since the value of "\
          "'group_replication_group_name' does not match the one "\
          "registered in the ReplicaSet's Metadata: possible split-brain "\
          "scenario. Please remove the instance from the cluster.";

      session->close(shcore::Argument_list());

      throw shcore::Exception::runtime_error(nice_error);
    }
  }

  // Verify if the group_replication plugin is active on the seed instance
  {
    log_info("Verifying if the group_replication plugin is active on the seed "
             "instance %s",
             instance_address.c_str());

    classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(seed_session.get());
    std::string plugin_status = get_plugin_status(classic->connection(),
                                "group_replication");

    if (plugin_status != "ACTIVE") {
      throw shcore::Exception::runtime_error(
        "Cannot rejoin instance. The seed instance doesn't have "
        "group-replication active.");
    }
  }

  // Get @@group_replication_local_address
  std::string seed_instance_xcom_address;
  get_server_variable(classic->connection(), "group_replication_local_address",
                      seed_instance_xcom_address);

  // join Instance to cluster
  {
    int exit_code;
    shcore::Argument_list temp_args;

    try {
      log_info("Opening a new session to the rejoining instance %s",
               instance_address.c_str());
      shcore::Argument_list slave_args;
      slave_args.push_back(shcore::Value(instance_def));
      session = mysqlsh::connect_session(slave_args,
                                         mysqlsh::SessionType::Classic);
      classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
    } catch (std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_address.c_str(), e.what());
      throw;
    }

    // Check replication filters before creating the Metadata.
    validate_replication_filters(classic);

    auto peer_session = dynamic_cast<mysqlsh::mysql::ClassicSession*>(
      _metadata_storage->get_session().get());

    boost::to_upper(ssl_mode);
    std::string new_ssl_mode;
    // Resolve the SSL Mode to use to configure the instance.
    new_ssl_mode = resolve_instance_ssl_mode(classic, peer_session, ssl_mode);
    if (new_ssl_mode != ssl_mode) {
      ssl_mode = new_ssl_mode;
      log_warning("SSL mode used to configure the instance: '%s'",
                  ssl_mode.c_str());
    }

    //Get SSL values to connect to peer instance
    Value::Map_type_ref peer_instance_ssl_opts(new shcore::Value::Map_type);
    std::string peer_ssl_ca = peer_session->get_ssl_ca();
    std::string peer_ssl_cert = peer_session->get_ssl_cert();
    std::string peer_ssl_key = peer_session->get_ssl_key();
    if (!peer_ssl_ca.empty())
      (*peer_instance_ssl_opts)["sslCa"] = Value(peer_ssl_ca);
    if (!peer_ssl_cert.empty())
      (*peer_instance_ssl_opts)["sslCert"] = Value(peer_ssl_cert);
    if (!peer_ssl_key.empty())
      (*peer_instance_ssl_opts)["sslKey"] = Value(peer_ssl_key);

    // Stop group-replication
    log_info("Stopping group-replication at instance %s",
             instance_address.c_str());
    temp_args.clear();
    temp_args.push_back(shcore::Value("STOP GROUP_REPLICATION"));
    classic->run_sql(temp_args);

    // Get SSL values to connect to instance
    Value::Map_type_ref instance_ssl_opts(new shcore::Value::Map_type);
    if (instance_def->has_key("sslCa")) {
      (*instance_ssl_opts)["sslCa"] = Value(instance_def->get_string("sslCa"));
    }
    if (instance_def->has_key("sslCert")) {
      (*instance_ssl_opts)["sslCert"] = Value(
        instance_def->get_string("sslCert"));
    }
    if (instance_def->has_key("sslKey")) {
      (*instance_ssl_opts)["sslKey"] = Value(
        instance_def->get_string("sslKey"));
    }

    // Get the seed session connection data
    auto seed_session_options =
          get_connection_data(seed_session->uri(), false);
    std::string seed_session_port =
          std::to_string(seed_session_options->get_int("port"));
    std::string seed_session_host =
          seed_session_options->get_string("host");
    std::string seed_session_address =
          seed_session_host + ":" + seed_session_port;
    std::string seed_session_user = seed_session->get_user();

    // Get SSL values to connect to the seed instance
    Value::Map_type_ref seed_instance_ssl_opts(new shcore::Value::Map_type);
    std::string seed_ssl_ca = seed_session->get_ssl_ca();
    std::string seed_ssl_cert = seed_session->get_ssl_cert();
    std::string seed_ssl_key = seed_session->get_ssl_key();
    if (!seed_ssl_ca.empty())
      (*seed_instance_ssl_opts)["sslCa"] = Value(seed_ssl_ca);
    if (!seed_ssl_cert.empty())
      (*seed_instance_ssl_opts)["sslCert"] = Value(seed_ssl_cert);
    if (!seed_ssl_key.empty())
      (*seed_instance_ssl_opts)["sslKey"] = Value(seed_ssl_key);

    // use mysqlprovision to rejoin the cluster.
    exit_code = _cluster->get_provisioning_interface()->join_replicaset(
                          instance_user + "@" + instance_address,
                          instance_ssl_opts,
                          "",
                          seed_session_user + "@" + seed_session_address,
                          seed_instance_ssl_opts,
                          instance_password, "",
                          ssl_mode, ip_whitelist,
                          seed_instance_xcom_address, true,
                          errors);
    if (exit_code == 0) {
        ret_val = shcore::Value("The instance '" + instance_address +
                                "' was successfully added to the MySQL "
                                "Cluster.");
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
 * \param args : A list of values to be used to remove a Instance to the Cluster.
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

shcore::Value ReplicaSet::remove_instance_(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, get_function_name("removeInstance").c_str());

  // Remove the Instance from the Default ReplicaSet
  try {
    ret_val = remove_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeInstance"));

  return ret_val;
}

shcore::Value ReplicaSet::remove_instance(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("removeInstance").c_str());

  std::string port;
  bool force = false;  // By default force is false.

  auto instance_def = get_instance_options_map(args, mysqlsh::dba::PasswordFormat::OPTIONS);

  // Retrieve and validate instance definitions.
  shcore::Argument_map opt_map(*instance_def);
  opt_map.ensure_keys({}, shcore::connection_attributes, "instance definition");

  if (!instance_def->has_key("port"))
    (*instance_def)["port"] = shcore::Value(get_default_port());

  port = std::to_string(instance_def->get_int("port"));

  // The instance definition received may or may not contain user/password
  // If provided then those are used, if not, we use the defaults from the active session
  bool get_default_user = false;
  bool get_default_password = false;
  std::string instance_admin_user;
  std::string instance_admin_user_password;

  if (instance_def->has_key("user"))
    instance_admin_user = instance_def->get_string("user");
  else if (instance_def->has_key("dbUser"))
    instance_admin_user = instance_def->get_string("dbUser");
  else
    get_default_user = true;

  if (instance_def->has_key("password"))
    instance_admin_user_password = instance_def->get_string("password");
  else if (instance_def->has_key("dbPassword"))
    instance_admin_user_password = instance_def->get_string("dbPassword");
  else
    get_default_password = true;

  // Retrieve and validate options.
  if (args.size() == 2) {
    auto remove_options = args.map_at(1);
    shcore::Argument_map remove_options_map(*remove_options);
    remove_options_map.ensure_keys({}, _remove_instance_opts, "options");

    if (remove_options->has_key("force"))
      force = remove_options->get_bool("force");
  }

  // get instance admin and user information from the metadata session which is the session saved on the cluster
  if (get_default_user || get_default_password) {
    auto instance_session(_metadata_storage->get_session());

    if (get_default_user)
      instance_admin_user = instance_session->get_user();

    if (get_default_password)
      instance_admin_user_password = instance_session->get_password();
  }

  std::string host = instance_def->get_string("host");

  // Check if the instance was already added
  std::string instance_address = host + ":" + port;

  bool is_instance_on_md = _metadata_storage->is_instance_on_replicaset(get_id(), instance_address);

  // Check if the instance exists on the ReplicaSet
  //std::string instance_address = options->get_string("host") + ":" + std::to_string(options->get_int("port"));
  if (!is_instance_on_md) {
    std::string message = "The instance '" + instance_address + "'";

    message.append(" does not belong to the ReplicaSet: '" + get_member("name").as_string() + "'.");

    throw shcore::Exception::runtime_error(message);
  }

  // Check if it is the last instance in the ReplicaSet and issue an error.
  // NOTE: When multiple replicasets are supported this check needs to be moved
  //       to a higher level (to check if the instance is the last one of the
  //       last replicaset, which should be the default replicaset).
  if (_metadata_storage->get_replicaset_count(get_id()) == 1) {
    throw Exception::logic_error(
        "The instance '" + instance_address + "' cannot be removed because it "
        "is the only member of the Cluster. "
        "Please use <Cluster>." + get_member_name("dissolve", naming_style) +
        "() instead to remove the last instance and dissolve the Cluster.");
  }

  auto session = _metadata_storage->get_session();
  mysqlsh::mysql::ClassicSession *classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());

  GRInstanceType type = get_gr_instance_type(classic->connection());

  // TODO: do we remove the host? we check if is the last instance of that host and them remove?
  // auto result = _metadata_storage->remove_host(args);

  // TODO: the instance_label can be actually a name, check TODO above
  // NOTE: When applicable, removal of the replication user and the instance metadata must be done
  //       before leaving the replicaset, doing it after lets the next inconsistencies:
  //       - both changes are applied on an instance that is no longer in a cluster
  //         which means the changes are not replicated
  //       - Also causes the gtid to differ from the cluster instances which lets this instance
  //         out of the game: can't be added again because of the gtid diverge
  //       - If removing the master instance, a new master will be promoted but this instance will never
  //         be removed from the cluster

  // Get the instance row details (required later to add back the instance if
  // needed)
  shcore::Value::Map_type_ref instance =
        _metadata_storage->get_instance(instance_address);

  if (type == GRInstanceType::InnoDBCluster ||
      type == GRInstanceType::GroupReplication) {

    // Remove instance from the MD (metadata).
    // NOTE: This operation MUST be performed before leave-replicaset to ensure
    // that the MD change is also propagated to the target instance to
    // remove (if ONLINE). This avoid issues removing and adding an instance
    // again (error adding the instance because it is already in the MD).
    MetadataStorage::Transaction tx(_metadata_storage);
    remove_instance_metadata(instance_def);
    tx.commit();

    // Call provisioning to remove the instance from the replicaset
    // NOTE: We always try (best effort) to execute leave_replicaset(), but
    // ignore any error in that if force is used (even if the instance is not
    // reachable, since it is already expected to fail).
    int exit_code = -1;
    std::string instance_url;
    shcore::Value::Array_type_ref errors;

    instance_url = instance_admin_user + "@";
    instance_url.append(host);
    instance_url.append(":" + port);

    // Get SSL values to connect to instance
    Value::Map_type_ref instance_ssl_opts(new shcore::Value::Map_type);
    if (instance_def->has_key("sslCa"))
      (*instance_ssl_opts)["sslCa"] = Value(instance_def->get_string("sslCa"));
    if (instance_def->has_key("sslCert"))
      (*instance_ssl_opts)["sslCert"] = Value(instance_def->get_string("sslCert"));
    if (instance_def->has_key("sslKey"))
      (*instance_ssl_opts)["sslKey"] = Value(instance_def->get_string("sslKey"));

    exit_code = _cluster->get_provisioning_interface()->leave_replicaset(instance_url,
                                                                         instance_ssl_opts,
                                                                         instance_admin_user_password,
                                                                         errors);

    // Only add the metadata back if the force option was not used.
    if (exit_code != 0) {

      if (!force) {
        // If the the removal of the instance from the replicaset failed
        // We must add it back to the MD if force is not used
        // NOTE: This is a temporary fix for the issue caused by the
        // API not being atomic.
        // As soon as the API becomes atomic, the following solution can
        // be dropped
        _metadata_storage->insert_instance(instance);

        // If leave replicaset failed and force was not used then check the
        // state of the instance to assess the possible cause of the failure.
        ManagedInstance::State state = get_instance_state(classic->connection(),
                                                          instance_address);
        if (state == ManagedInstance::Unreachable ||
            state == ManagedInstance::Missing) {
          // Send a diferent error if the instance is not reachable
          // (and the force option was not used).
          std::string message = "The instance '" + instance_address + "'";
          message.append(" cannot be removed because it is on a '");
          message.append(ManagedInstance::describe(
              static_cast<ManagedInstance::State>(state)));
          message.append("' state. Please bring the instance back ONLINE and "
                         "try to remove it again. If the instance is "
                         "permanently not reachable, then please use "
                         "<Cluster>.");
          message.append(get_member_name("removeInstance", naming_style));
          message.append(
              "() with the force option set to true to proceed with the "
                  "operation and only remove the instance from the Cluster "
                  "Metadata.");
          throw shcore::Exception::runtime_error(message);
        } else {
          throw shcore::Exception::runtime_error(
              get_mysqlprovision_error_string(errors));
        }
      }
      // If force is used do not add the instance back to the metadata,
      // and ignore any leave-replicaset error.
    }
  } else {
    // Remove instance from the MD anyway in case it is standalone.
    // NOTE: Added for safety and completness, this situation should not happen
    //       (covered by preconditions).
    MetadataStorage::Transaction tx(_metadata_storage);
    remove_instance_metadata(instance_def);
    tx.commit();
  }

  return shcore::Value();
}

shcore::Value ReplicaSet::dissolve(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(0, 1, get_function_name("dissolve").c_str());

  try {
    bool force = false;
    shcore::Value::Map_type_ref options;

    if (args.size() == 1)
      options = args.map_at(0);

    if (options) {
      shcore::Argument_map opt_map(*options);

      opt_map.ensure_keys({}, {"force"}, "dissolve options");

      if (options->has_key("force"))
        force = opt_map.bool_at("force");
    }

    if (!force && _metadata_storage->is_replicaset_active(get_id()))
      throw shcore::Exception::runtime_error("Cannot dissolve the ReplicaSet: the ReplicaSet is active.");

    MetadataStorage::Transaction tx(_metadata_storage);

    uint64_t rset_id = get_id();

    // remove all the instances from the ReplicaSet
    auto instances = _metadata_storage->get_replicaset_instances(rset_id);

    _metadata_storage->drop_replicaset(rset_id);

    tx.commit();

    remove_instances_from_gr(instances);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dissolve"));

  return ret_val;
}

void ReplicaSet::remove_instances_from_gr(const shcore::Value::Array_type_ref &instances) {
  MetadataStorage::Transaction tx(_metadata_storage);
  int exit_code;

  auto instance_session(_metadata_storage->get_session());
  auto classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(instance_session.get());
  std::string instance_admin_user = instance_session->get_user();
  std::string instance_admin_user_password = instance_session->get_password();

  /* This function usually starts by removing from the replicaset the R/W instance, which
   * usually is the first on the instances list, and on primary-master mode that implies
   * a new master election. So to avoid GR BUG#24818604 , we must leave the R/W instance for last.
   */

  // Get the R/W instance
  std::string master_uuid, master_instance;
  get_status_variable(classic->connection(), "group_replication_primary_member", master_uuid, false);

  if (!master_uuid.empty()) {
    for (auto value : *instances.get()) {
      auto row = value.as_object<mysqlsh::Row>();
      if (row->get_member(0).as_string() == master_uuid) {
        master_instance = row->get_member("host").as_string();
      }
    }
  }

  //Get SSL values to connect to the instances
  //NOTE: It is assumed that the same SSL settings (CA, Cert and Key) are used
  //      to connect to all the cluster instances. Therefore, the SSL settings
  //      can be obtained from the cluster session.
  Value::Map_type_ref instance_ssl_opts(new shcore::Value::Map_type);
  auto cluster_session = _metadata_storage->get_session();
  std::string ssl_ca = cluster_session->get_ssl_ca();
  std::string ssl_cert = cluster_session->get_ssl_cert();
  std::string ssl_key = cluster_session->get_ssl_key();
  if (!ssl_ca.empty())
    (*instance_ssl_opts)["sslCa"] = Value(ssl_ca);
  if (!ssl_cert.empty())
    (*instance_ssl_opts)["sslCert"] = Value(ssl_cert);
  if (!ssl_key.empty())
    (*instance_ssl_opts)["sslKey"] = Value(ssl_key);

  for (auto value : *instances.get()) {
    auto row = value.as_object<mysqlsh::Row>();

    std::string instance = row->get_member("host").as_string();

    if (instance != master_instance) {
      std::string instance_url = instance_admin_user + "@" + instance;
      shcore::Value::Array_type_ref errors;

      // Leave the replicaset
      exit_code = _cluster->get_provisioning_interface()->leave_replicaset(instance_url,
                                                                           instance_ssl_opts,
                                                                           instance_admin_user_password,
                                                                           errors);
      if (exit_code != 0)
        throw shcore::Exception::runtime_error(get_mysqlprovision_error_string(errors));
    }
  }

  // Remove the master instance
  if (!master_uuid.empty()) {
    std::string instance_url = instance_admin_user + "@" + master_instance;
    shcore::Value::Array_type_ref errors;

    // Leave the replicaset
    exit_code = _cluster->get_provisioning_interface()->leave_replicaset(instance_url,
                                                                         instance_ssl_opts,
                                                                         instance_admin_user_password,
                                                                         errors);
    if (exit_code != 0)
      throw shcore::Exception::runtime_error(get_mysqlprovision_error_string(errors));
  }

  tx.commit();
}

shcore::Value ReplicaSet::disable(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(0, get_function_name("disable").c_str());

  try {
    MetadataStorage::Transaction tx(_metadata_storage);

    // Get all instances of the replicaset
    auto instances = _metadata_storage->get_replicaset_instances(get_id());

    // Update the metadata to turn 'active' off
    _metadata_storage->disable_replicaset(get_id());

    tx.commit();

    remove_instances_from_gr(instances);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("disable"));

  return ret_val;
}

shcore::Value ReplicaSet::rescan(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  try {
    ret_val = shcore::Value(_rescan(args));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rescan"));

  return ret_val;
}

shcore::Value::Map_type_ref ReplicaSet::_rescan(const shcore::Argument_list &args) {
  shcore::Value::Map_type_ref ret_val(new shcore::Value::Map_type());

  // Set the ReplicaSet name on the result map
  (*ret_val)["name"] = shcore::Value(_name);

  std::vector<NewInstanceInfo> newly_discovered_instances_list =
      get_newly_discovered_instances(_metadata_storage, _id);

  // Creates the newlyDiscoveredInstances map
  shcore::Value::Array_type_ref newly_discovered_instances(new shcore::Value::Array_type());

  for (auto &instance : newly_discovered_instances_list) {
    shcore::Value::Map_type_ref newly_discovered_instance(new shcore::Value::Map_type());
    (*newly_discovered_instance)["member_id"] = shcore::Value(instance.member_id);
    (*newly_discovered_instance)["name"] = shcore::Value::Null();

    std::string instance_address = instance.host + ":" + std::to_string(instance.port);

    (*newly_discovered_instance)["host"] = shcore::Value(instance_address);
    newly_discovered_instances->push_back(shcore::Value(newly_discovered_instance));
  }
  // Add the newly_discovered_instances list to the result Map
  (*ret_val)["newlyDiscoveredInstances"] = shcore::Value(newly_discovered_instances);

  shcore::Value unavailable_instances_result;

  std::vector<MissingInstanceInfo> unavailable_instances_list =
      get_unavailable_instances(_metadata_storage, _id);

  // Creates the unavailableInstances array
  shcore::Value::Array_type_ref unavailable_instances(new shcore::Value::Array_type());

  for (auto &instance : unavailable_instances_list) {
    shcore::Value::Map_type_ref unavailable_instance(new shcore::Value::Map_type());
    (*unavailable_instance)["member_id"] = shcore::Value(instance.id);
    (*unavailable_instance)["label"] = shcore::Value(instance.label);
    (*unavailable_instance)["host"] = shcore::Value(instance.host);

    unavailable_instances->push_back(shcore::Value(unavailable_instance));
  }
  // Add the missing_instances list to the result Map
  (*ret_val)["unavailableInstances"] = shcore::Value(unavailable_instances);

  return ret_val;
}

std::string ReplicaSet::get_peer_instance() {
  std::string master_uuid, master_instance;

  // We need to retrieve a peer instance, so let's use the Seed one
  // If using single-primary mode the Seed instance is the primary
  auto instance_session = _metadata_storage->get_session();
  auto classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(
                              instance_session.get());
  get_status_variable(classic->connection(),
                      "group_replication_primary_member",
                      master_uuid, false);

  auto instances = _metadata_storage->get_replicaset_online_instances(get_id());

  if (!master_uuid.empty()) {
    for (auto value : *instances.get()) {
      auto row = value.as_object<mysqlsh::Row>();
      if (row->get_member(0).as_string() == master_uuid) {
        master_instance = row->get_member("host").as_string();
      }
    }
  } else {
    // If in multi-master mode, any instance works
    // so we can get the first one that is online
    auto value = instances.get()->front();
    auto row = value.as_object<mysqlsh::Row>();
    master_instance = row->get_member("host").as_string();
  }

  return master_instance;
}

shcore::Value ReplicaSet::check_instance_state(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, 2, get_function_name("checkInstanceState").c_str());

  // Verifies the transaction state of the instance ins relation to the cluster
  try {
    ret_val = retrieve_instance_state(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getInstanceState"));

  return ret_val;
}

shcore::Value ReplicaSet::retrieve_instance_state(const shcore::Argument_list &args) {
  auto instance_def = get_instance_options_map(args, PasswordFormat::STRING);

  shcore::Argument_map opt_map(*instance_def);
  opt_map.ensure_keys({"host"},
                      shcore::connection_attributes,
                      "instance definition");

  if (!instance_def->has_key("port"))
    (*instance_def)["port"] = shcore::Value(get_default_port());

  // Sets a default user if not specified
  resolve_instance_credentials(instance_def, nullptr);

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_def));
  auto instance_session = Dba::get_session(new_args);

  // We will work with the session saved on the metadata which points to the cluster
  // Assuming it is the R/W instance
  auto master_dev_session = _metadata_storage->get_session();
  auto master_session = std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(master_dev_session);

  // We have to retrieve these variables to do the actual state validation
  std::string master_gtid_executed;
  std::string master_gtid_purged;
  std::string instance_gtid_executed;
  std::string instance_gtid_purged;

  get_gtid_state_variables(master_session->connection(), master_gtid_executed, master_gtid_purged);
  get_gtid_state_variables(instance_session->connection(), instance_gtid_executed, instance_gtid_purged);

  // Now we perform the validation
  SlaveReplicationState state = get_slave_replication_state(master_session->connection(), instance_gtid_executed);

  std::string reason;
  std::string status;
  switch (state) {
    case SlaveReplicationState::Diverged:
      status = "error";
      reason = "diverged";
      break;
    case SlaveReplicationState::Irrecoverable:
      status = "error";
      reason = "lost_transactions";
      break;
    case SlaveReplicationState::Recoverable:
      status = "ok";
      reason = "recoverable";
      break;
    case SlaveReplicationState::New:
      status = "ok";
      reason = "new";
      break;
  }

  shcore::Value::Map_type_ref ret_val(new shcore::Value::Map_type());

  (*ret_val)["state"] = shcore::Value(status);
  (*ret_val)["reason"] = shcore::Value(reason);

  return shcore::Value(ret_val);
}

void ReplicaSet::add_instance_metadata(const shcore::Value::Map_type_ref &instance_definition, const std::string& label) {
  log_debug("Adding instance to metadata");

  MetadataStorage::Transaction tx(_metadata_storage);

  int xport = instance_definition->get_int("port") * 10;
  std::string local_gr_address;

  std::string joiner_host = instance_definition->get_string("host");

  // Check if the instance was already added
  std::string instance_address = joiner_host + ":" + std::to_string(instance_definition->get_int("port"));
  std::string mysql_server_uuid;
  std::string mysql_server_address;

  log_debug("Connecting to '%s' to query for metadata information...",
             instance_address.c_str());
  // get the server_uuid from the joining instance
  {
    std::shared_ptr<mysqlsh::mysql::ClassicSession> classic;
    try {
      shcore::Argument_list new_args;
      new_args.push_back(shcore::Value(instance_definition));
      classic = std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(
            mysqlsh::connect_session(new_args, mysqlsh::SessionType::Classic));
    } catch (Exception &e) {
      std::stringstream ss;
      ss << "Error opening session to '" << instance_address << "': " << e.what();
      log_warning("%s", ss.str().c_str());

      // Check if we're adopting a GR cluster, if so, it could happen that
      // we can't connect to it because root@localhost exists but root@hostname
      // doesn't (GR keeps the hostname in the members table)
      if (e.is_mysql() && e.code() == 1045) { // access denied
        std::stringstream se;
        se << "Access denied connecting to new instance " << instance_address << ".\n"
            << "Please ensure all instances in the same group/replicaset have"
            " the same password for account '" << instance_definition->get_string("user")
            << "' and that it is accessible from the host mysqlsh is running from.";
        throw Exception::runtime_error(se.str());
      }
      throw Exception::runtime_error(ss.str());
    }
    {
      // Query UUID of the member and its public hostname
      auto result = classic->execute_sql("SELECT @@server_uuid");
      auto row = result->fetch_one();
      if (row) {
        mysql_server_uuid = row->get_value_as_string(0);
      } else
        throw Exception::runtime_error("@@server_uuid could not be queried");
    }
    try {
      auto result = classic->execute_sql("SELECT @@mysqlx_port");
      auto xport_row = result->fetch_one();
      if (xport_row)
        xport = (int)xport_row->get_value(0).as_int();
    } catch (std::exception &e) {
      log_info("Could not query xplugin port, using default value: %s", e.what());
    }

    // Loads the local HR host data
    get_server_variable(classic->connection(), "group_replication_local_address", local_gr_address, false);

    if (!mysql_server_address.empty() && mysql_server_address != joiner_host) {
      log_info("Normalized address of '%s' to '%s'", joiner_host.c_str(), mysql_server_address.c_str());
      instance_address = mysql_server_address + ":" + std::to_string(instance_definition->get_int("port"));
    } else {
      mysql_server_address = joiner_host;
    }
  }
  std::string instance_xaddress;
  instance_xaddress = mysql_server_address + ":" + std::to_string(xport);

  (*instance_definition)["role"] = shcore::Value("HA");
  (*instance_definition)["endpoint"] = shcore::Value(instance_address);
  (*instance_definition)["xendpoint"] = shcore::Value(instance_xaddress);
  (*instance_definition)["grendpoint"] = shcore::Value(local_gr_address);
  (*instance_definition)["mysql_server_uuid"] = shcore::Value(mysql_server_uuid);

  (*instance_definition)["label"] = shcore::Value(label.empty() ? instance_address : label);

  // update the metadata with the host
  uint32_t host_id = _metadata_storage->insert_host(instance_definition);

  // And the instance
  _metadata_storage->insert_instance(instance_definition, host_id, get_id());

  tx.commit();
}

void ReplicaSet::remove_instance_metadata(const shcore::Value::Map_type_ref& instance_def) {
  log_debug("Removing instance from metadata");

  MetadataStorage::Transaction tx(_metadata_storage);

  std::string port = std::to_string(instance_def->get_int("port"));

  std::string host = instance_def->get_string("host");

  // Check if the instance was already added
  std::string instance_address = host + ":" + port;

  _metadata_storage->remove_instance(instance_address);

  tx.commit();
}

std::vector<std::string> ReplicaSet::get_online_instances() {
  std::vector<std::string> online_instances_array;

  auto online_instances = _metadata_storage->get_replicaset_online_instances(_id);

  for (auto value : *online_instances.get()) {
    auto row = value.as_object<mysqlsh::Row>();

    std::string instance_host = row->get_member(3).as_string();
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
* \param name The name of the Instance to be used as partition to force the quorum on the ReplicaSet
*/
#if DOXYGEN_JS
Undefined ReplicaSet::forceQuorumUsingPartitionOf(InstanceDef instance);
#elif DOXYGEN_PY
None ReplicaSet::force_quorum_using_partition_of(InstanceDef instance);
#endif
#endif  // DOXYGEN_CPP
shcore::Value ReplicaSet::force_quorum_using_partition_of_(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, 2, get_function_name("forceQuorumUsingPartitionOf").c_str());

  // Check if the ReplicaSet is empty
  if (_metadata_storage->is_replicaset_empty(get_id()))
    throw shcore::Exception::runtime_error("ReplicaSet not initialized.");

  // Rejoin the Instance to the Default ReplicaSet
  try {
    ret_val = force_quorum_using_partition_of(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("forceQuorumUsingPartitionOf"));

  return ret_val;
}

shcore::Value ReplicaSet::force_quorum_using_partition_of(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  std::string host;
  int port = 0;
  uint64_t rset_id = get_id();
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> session;
  mysqlsh::mysql::ClassicSession *classic;

  auto instance_def = get_instance_options_map(args, PasswordFormat::STRING);
  shcore::Argument_map opt_map(*instance_def);
  opt_map.ensure_keys({"host"},
                      shcore::connection_attributes,
                      "instance definition");

  if (!instance_def->has_key("port"))
    (*instance_def)["port"] = shcore::Value(get_default_port());

  port = instance_def->get_int("port");
  host = instance_def->get_string("host");

  std::string instance_address = host + ":" + std::to_string(port);

  // Sets a default user if not specified
  resolve_instance_credentials(instance_def, nullptr);
  std::string password = instance_def->get_string(instance_def->has_key("password") ? "password" : "dbPassword");

  // TODO: test if there's already quorum and add a 'force' option to be used if so

  // TODO: test if the instance if part of the current cluster, for the scenario of restoring
  // a cluster quorum from another

  // Check if the instance belongs to the ReplicaSet on the Metadata
  if (!_metadata_storage->is_instance_on_replicaset(rset_id, instance_address)) {
    std::string message = "The instance '" + instance_address + "'";

    message.append(" does not belong to the ReplicaSet: '" + get_member("name").as_string() + "'.");

    throw shcore::Exception::runtime_error(message);
  }

  // Before rejoining an instance we must verify if the instance's
  // 'group_replication_group_name' matches the one registered in the
  // Metadata (BUG #26159339)
  {
    try {
      log_info("Opening a new session to the partition instance %s",
               instance_address.c_str());
      shcore::Argument_list slave_args;
      slave_args.push_back(shcore::Value(instance_def));
      session = mysqlsh::connect_session(slave_args,
                                         mysqlsh::SessionType::Classic);
      classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
    } catch (std::exception &e) {
      log_error("Could not open connection to '%s': %s",
                instance_address.c_str(), e.what());
      throw;
    }

    if (!validate_replicaset_group_name(
            _metadata_storage, classic, _id)) {
      std::string nice_error =
          "The instance '" + instance_address + "' "\
          "cannot be used to restore the cluster as it "\
          "may belong to a different ReplicaSet as the one registered "\
          "in the Metadata since the value of "\
          "'group_replication_group_name' does not match the one "\
          "registered in the ReplicaSet's Metadata: possible split-brain "\
          "scenario.";

      session->close(shcore::Argument_list());

      throw shcore::Exception::runtime_error(nice_error);
    }
  }

  // Get the instance state
  ReplicationGroupState state;

  try {
    log_info("Opening a new session to the partition instance %s",
             instance_address.c_str());
    shcore::Argument_list partition_instance_args;
    partition_instance_args.push_back(shcore::Value(instance_def));
    session = mysqlsh::connect_session(partition_instance_args, mysqlsh::SessionType::Classic);
    classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
  } catch (std::exception &e) {
    log_error("Could not open connection to '%s': %s", instance_address.c_str(),
              e.what());
    throw;
  }

  auto instance_type = get_gr_instance_type(classic->connection());

  if (instance_type != GRInstanceType::Standalone) {
    state = get_replication_group_state(classic->connection(), instance_type);

    if (state.source_state != ManagedInstance::OnlineRW &&
        state.source_state != ManagedInstance::OnlineRO) {
      std::string message = "The instance '" + instance_address + "'";
      message.append(" cannot be used to restore the cluster as it is on a ");
      message.append(ManagedInstance::describe(static_cast<ManagedInstance::State>(state.source_state)));
      message.append(" state, and should be ONLINE");

      throw shcore::Exception::runtime_error(message);
    }
  } else {
    std::string message = "The instance '" + instance_address + "'";
    message.append(" cannot be used to restore the cluster as it is not an active member of replication group.");
    throw shcore::Exception::runtime_error(message);
  }

  session->close(shcore::Argument_list());

  // Get the online instances of the ReplicaSet to user as group_peers
  auto online_instances = _metadata_storage->get_replicaset_online_instances(rset_id);

  if (!online_instances)
    throw shcore::Exception::logic_error("No online instances are visible from the given one.");

  std::string group_peers;

  for (auto value : *online_instances.get()) {
    auto row = value.as_object<mysqlsh::Row>();

    std::string instance_host = row->get_member(3).as_string();

    shcore::Argument_list session_args;
    Value::Map_type_ref group_peer_options(new shcore::Value::Map_type);

    std::string delimiter = ":";
    std::string group_peer_host = instance_host.substr(0, instance_host.find(delimiter));
    std::string group_peer_port = instance_host.substr(instance_host.find(delimiter) + 1, instance_host.length());

    (*group_peer_options)["host"] = shcore::Value(group_peer_host);
    (*group_peer_options)["port"] = shcore::Value(atoi(group_peer_port.c_str()));
    // We assume the root password is the same on all instances
    (*group_peer_options)["password"] = shcore::Value(password);
    (*group_peer_options)["user"] = shcore::Value("root");
    session_args.push_back(shcore::Value(group_peer_options));

    try {
      log_info("Opening a new session to a group_peer instance to obtain the XCOM address %s",
               instance_host.c_str());
      session = mysqlsh::connect_session(session_args, mysqlsh::SessionType::Classic);
      classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
    } catch (std::exception &e) {
      log_error("Could not open connection to %s: %s", instance_address.c_str(),
                e.what());
      throw;
    }

    std::string group_peer_instance_xcom_address;

    // Get @@group_replication_local_address
    get_server_variable(classic->connection(), "group_replication_local_address", group_peer_instance_xcom_address);

    group_peers.append(group_peer_instance_xcom_address);
    group_peers.append(",");
  }

  session->close(shcore::Argument_list());

  // Force the reconfiguration of the GR group
  {
    try {
      log_info("Opening a new session to the partition instance %s",
               instance_address.c_str());
      shcore::Argument_list partition_instance_args;
      partition_instance_args.push_back(shcore::Value(instance_def));
      session = mysqlsh::connect_session(partition_instance_args, mysqlsh::SessionType::Classic);
      classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
    } catch (std::exception &e) {
      log_error("Could not open connection to '%s': %s", instance_address.c_str(),
                e.what());
      throw;
    }

    // Remove the trailing comma of group_peers
    if (group_peers.back() == ',')
      group_peers.pop_back();

    log_info("Setting the group_replication_force_members at instance %s",
              instance_address.c_str());

    set_global_variable(classic->connection(), "group_replication_force_members", group_peers);

    session->close(shcore::Argument_list());
  }

  return ret_val;
}

ReplicationGroupState ReplicaSet::check_preconditions(const std::string& function_name) const {
  return check_function_preconditions(class_name(), function_name, get_function_name(function_name), _metadata_storage);
}

shcore::Value ReplicaSet::get_description() const {
  shcore::Value ret_val = shcore::Value::new_map();
  auto description = ret_val.as_map();

  shcore::sqlstring query(
      "SELECT mysql_server_uuid, instance_name, role, " \
      "JSON_UNQUOTE(JSON_EXTRACT(addresses, '$.mysqlClassic')) AS host "
      "FROM mysql_innodb_cluster_metadata.instances "
      "WHERE replicaset_id = ?", 0);
  query << _id;
  query.done();

  auto result = _metadata_storage->execute_sql(query);

  auto raw_instances = result->call("fetchAll", shcore::Argument_list());

  // First we identify the master instance
  auto instances = raw_instances.as_array();

  (*description)["name"] = shcore::Value(_name);
  (*description)["instances"] = shcore::Value::new_array();

  auto instance_list = description->get_array("instances");

  for (auto value : *instances.get()) {
    auto row = value.as_object<mysqlsh::Row>();
    auto instance = shcore::Value::new_map();
    auto instance_obj = instance.as_map();

    (*instance_obj)["label"] = row->get_member(1);
    (*instance_obj)["host"] = row->get_member(3);
    (*instance_obj)["role"] = row->get_member(2);

    instance_list->push_back(instance);
  }

  return ret_val;
}

shcore::Value ReplicaSet::get_status(const mysqlsh::dba::ReplicationGroupState &state) const {
  shcore::Value ret_val = shcore::Value::new_map();
  auto status = ret_val.as_map();

  // First, check if the topology type matchs the current state in order to
  // retrieve the status correctly, otherwise issue an error.
  this->verify_topology_type_change();

  bool single_primary_mode = _topology_type == kTopologyPrimaryMaster;

  // Identifies the master node
  std::string master_uuid;
  if (single_primary_mode) {
    // get the current cluster session from the metadata
    auto instance_session = _metadata_storage->get_session();
    auto classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(instance_session.get());
    get_status_variable(classic->connection(), "group_replication_primary_member", master_uuid, false);
  }

  shcore::sqlstring query(
      "SELECT mysql_server_uuid, instance_name, role, MEMBER_STATE, "
      "JSON_UNQUOTE(JSON_EXTRACT(addresses, '$.mysqlClassic')) as host "
      "FROM mysql_innodb_cluster_metadata.instances "
      "LEFT JOIN performance_schema.replication_group_members "
      "ON `mysql_server_uuid`=`MEMBER_ID` WHERE replicaset_id = ?", 0);
  query << _id;
  query.done();

  auto result = _metadata_storage->execute_sql(query);

  auto raw_instances = result->call("fetchAll", shcore::Argument_list());

  auto instances = raw_instances.as_array();

  std::shared_ptr<mysqlsh::Row> master;
  int online_count = 0, total_count = 0;

  for (auto value : *instances.get()) {
    total_count++;
    auto row = value.as_object<mysqlsh::Row>();
    if (row->get_member(0).as_string() == master_uuid)
      master = row;

    auto status = row->get_member(3);
    if (status && status.as_string() == "ONLINE")
      online_count++;
  }

  /*
   * Theory:
   *
   * unreachable_instances = COUNT(member_state = 'UNREACHABLE')
   * quorum = unreachable_instances < (total_instances / 2)
   * total_ha_instances = 2 * (number_of_failures) + 1
   * number_of_failures = (total_ha_instances - 1) / 2
   */

  int number_of_failures = (online_count - 1) / 2;
  int non_active = total_count - online_count;

  std::string desc_status;
  ReplicaSetStatus::Status rs_status;

  // Get the current cluster session from the metadata
  auto session = std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(_metadata_storage->get_session());
  auto uri = session->uri();
  auto options = get_connection_data(session->uri(), false);

  std::string port = std::to_string(options->get_int("port"));
  std::string host = options->get_string("host");
  std::string active_session_address = host + ":" + port;

  if (state.quorum == ReplicationQuorum::Quorumless) {
    rs_status = ReplicaSetStatus::NO_QUORUM;
    desc_status = "Cluster has no quorum as visible from '" +
                  active_session_address + "' and cannot process write transactions.";
  } else {
    if (number_of_failures == 0) {
      rs_status = ReplicaSetStatus::OK_NO_TOLERANCE;

      desc_status = "Cluster is NOT tolerant to any failures.";
    } else {
      if (non_active > 0)
        rs_status = ReplicaSetStatus::OK_PARTIAL;
      else
        rs_status = ReplicaSetStatus::OK;

      if (number_of_failures == 1)
        desc_status = "Cluster is ONLINE and can tolerate up to ONE failure.";
      else
        desc_status = "Cluster is ONLINE and can tolerate up to " +
                      std::to_string(number_of_failures) + " failures.";
    }
  }

  if (non_active > 0) {
    if (non_active == 1)
      desc_status.append(" " + std::to_string(non_active) + " member is not active");
    else
      desc_status.append(" " + std::to_string(non_active) + " members are not active");
  }

  (*status)["name"] = shcore::Value(_name);
  (*status)["statusText"] = shcore::Value(desc_status);
  (*status)["status"] = shcore::Value(ReplicaSetStatus::describe(rs_status));

  // In single primary mode we need to add the "primary" field
  if (single_primary_mode && master)
    (*status)["primary"] = master->get_member(4);

  // Creates the topology node
  (*status)["topology"] = shcore::Value::new_map();
  auto instance_owner_node = status->get_map("topology");

  // Inserts the instances
  for (auto value : *instances.get()) {
    // Gets each row
    auto row = value.as_object<mysqlsh::Row>();

    auto instance_label = row->get_member(1).as_string();
    (*instance_owner_node)[instance_label] = shcore::Value::new_map();
    auto instance_node = instance_owner_node->get_map(instance_label);

    // check if it is the active session instance
    bool active_session_instance = false;

    if (active_session_address == row->get_member(4).as_string())
      active_session_instance = true;

    if (row == master && single_primary_mode)
      append_member_status(instance_node, row, true, active_session_instance);
    else
      append_member_status(instance_node, row, single_primary_mode ? false : true,
                           active_session_instance);

    (*instance_node)["readReplicas"] = shcore::Value::new_map();
  }

  return ret_val;
}

void ReplicaSet::remove_instances(const std::vector<std::string> &remove_instances) {
  if (!remove_instances.empty()) {
    for (auto instance : remove_instances) {
      // Validate instance address
      shcore::Argument_list args;
      args.push_back(shcore::Value(instance));
      auto instance_def = get_instance_options_map(args, mysqlsh::dba::PasswordFormat::STRING);

      // verify if the instance is on the metadata
      if (_metadata_storage->is_instance_on_replicaset(_id, instance)) {
        Value::Map_type_ref options(new shcore::Value::Map_type);

        shcore::Value::Map_type_ref connection_data = shcore::get_connection_data(instance);

        int instance_port = connection_data->get_int("port");
        std::string instance_host = connection_data->get_string("host");

        (*options)["host"] = shcore::Value(instance_host);
        (*options)["port"] = shcore::Value(instance_port);

        remove_instance_metadata(options);
      } else {
        std::string message = "The instance '" + instance + "'";
        message.append(" does not belong to the ReplicaSet: '" + get_member("name").as_string() + "'.");
        throw shcore::Exception::runtime_error(message);
      }
    }
  }
}

void ReplicaSet::rejoin_instances(const std::vector<std::string> &rejoin_instances,
                                  const shcore::Value::Map_type_ref &options) {
  std::string user, password;
  auto instance_session(_metadata_storage->get_session());

  if (!rejoin_instances.empty()) {
    // Get the user and password from the options
    // or from the instance session
    if (options) {
      shcore::Argument_map opt_map(*options);

      // Check if the password is specified on the options and if not prompt it
      if (opt_map.has_key("password"))
        password = opt_map.string_at("password");
      else if (opt_map.has_key("dbPassword"))
        password = opt_map.string_at("dbPassword");
      else
        password = instance_session->get_password();

      // check if the user is specified on the options and it not prompt it
      if (opt_map.has_key("user"))
        user = opt_map.string_at("user");
      else if (opt_map.has_key("dbUser"))
        user = opt_map.string_at("dbUser");
      else
        user = instance_session->get_user();

    } else {
      user = instance_session->get_user();
      password = instance_session->get_password();
    }

    for (auto instance : rejoin_instances) {
      // Validate instance address
      shcore::Argument_list args;
      args.push_back(shcore::Value(instance));
      auto instance_def = get_instance_options_map(args, mysqlsh::dba::PasswordFormat::STRING);

      // verify if the instance is on the metadata
      if (_metadata_storage->is_instance_on_replicaset(_id, instance)) {
        shcore::Argument_list args;
        shcore::Value::Map_type_ref options(new shcore::Value::Map_type);

        std::pair<std::string, int> instance_uri;

        shcore::Value::Map_type_ref connection_data = shcore::get_connection_data(instance);

        int instance_port = connection_data->get_int("port");
        std::string instance_host = connection_data->get_string("host");

        (*options)["user"] = shcore::Value(user);
        (*options)["password"] = shcore::Value(password);
        (*options)["host"] = shcore::Value(instance_host);
        (*options)["port"] = shcore::Value(instance_port);

        args.push_back(shcore::Value(options));

        // If rejoinInstance fails we don't want to stop the execution of the
        // function, but to log the error.
        try {
          std::string msg = "Rejoining the instance '" + instance + "' to the "
                            "cluster's default replicaset.";
          log_warning("%s", msg.c_str());
          rejoin_instance(args);
        } catch (shcore::Exception &e) {
          log_error("Failed to rejoin instance: %s", e.what());
        }
      } else {
        std::string msg = "The instance '" + instance + "' does not "
                          "belong to the cluster. Skipping rejoin to the Cluster.";
        throw shcore::Exception::runtime_error(msg);
      }
    }
  }
}
