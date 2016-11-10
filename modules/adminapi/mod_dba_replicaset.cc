/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlx_connection.h"
#include "shellcore/shell_core_options.h"
#include "common/process_launcher/process_launcher.h"
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
#ifdef _WIN32
#define strerror_r(errno,buf,len) strerror_s(buf,len,errno)
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#endif

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::dba;
using namespace shcore;

#define PASSWORD_LENGTH 16

std::set<std::string> ReplicaSet::_add_instance_opts = {"name", "host", "port", "user", "dbUser", "password", "dbPassword", "socket", "ssl_ca", "ssl_cert", "ssl_key", "ssl_key"};
std::set<std::string> ReplicaSet::_remove_instance_opts = {"name", "host", "port", "socket", "ssl_ca", "ssl_cert", "ssl_key", "ssl_key"};

char const *ReplicaSet::kTopologyPrimaryMaster = "pm";
char const *ReplicaSet::kTopologyMultiMaster = "mm";

static const std::string kSandboxDatadir = "sandboxdata";

static std::string generate_password(int password_lenght);

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
                                 bool read_write) {
  //dumper.append_string("name", member_row->get_member(1).as_string());
  auto status = member_row->get_member(3);
  (*node)["address"] = member_row->get_member(4);
  (*node)["status"] = status ? status : shcore::Value("OFFLINE");
  (*node)["role"] = member_row->get_member(2);
  (*node)["mode"] = shcore::Value(read_write ? "R/W" : "R/O");
}

shcore::Value ReplicaSet::get_status() const {
  shcore::Value ret_val = shcore::Value::new_map();
  auto status = ret_val.as_map();

  bool single_primary_mode = _topology_type == kTopologyPrimaryMaster;

  // Identifies the master node
  std::string master_uuid;
  if (single_primary_mode) {
    auto uuid_result = _metadata_storage->execute_sql("show status like 'group_replication_primary_member'");
    auto uuid_row = uuid_result->call("fetchOne", shcore::Argument_list());
    if (uuid_row)
      master_uuid = uuid_row.as_object<Row>()->get_member(1).as_string();
  }

  shcore::sqlstring query("SELECT mysql_server_uuid, instance_name, role, MEMBER_STATE, JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) as host " \
                          " FROM mysql_innodb_cluster_metadata.instances left join performance_schema.replication_group_members on `mysql_server_uuid`=`MEMBER_ID` " \
                          " WHERE replicaset_id = ?", 0);
  query << _id;
  query.done();

  auto result = _metadata_storage->execute_sql(query);

  auto raw_instances = result->call("fetchAll", shcore::Argument_list());

  // First we identify the master instance
  auto instances = raw_instances.as_array();

  std::shared_ptr<mysqlsh::Row> master;
  int online_count = 0;
  for (auto value : *instances.get()) {
    auto row = value.as_object<mysqlsh::Row>();
    if (row->get_member(0).as_string() == master_uuid)
      master = row;

    auto status = row->get_member(3);
    if (status && status.as_string() == "ONLINE")
      online_count++;
  }

  std::string rset_status;
  switch (online_count) {
    case 0:
    case 1:
    case 2:
      rset_status = "Cluster is NOT tolerant to any failures.";
      break;
    case 3:
      rset_status = "Cluster tolerant to up to ONE failure.";
      break;
      // Add logic on the default for the even/uneven count
    default:
      rset_status = "Cluster tolerant to up to " + std::to_string(online_count - 2) + " failures.";
      break;
  }

  (*status)["name"] = shcore::Value(_name);
  (*status)["status"] = shcore::Value(rset_status);

  // Creates the topology node
  (*status)["topology"] = shcore::Value::new_map();
  auto instance_owner_node = status->get_map("topology");

  // In single primary mode there should be a master
  if (single_primary_mode && master) {

    auto master_name = master->get_member(1).as_string();
    (*instance_owner_node)[master_name] = shcore::Value::new_map();
    auto master_node = instance_owner_node->get_map(master_name);

    append_member_status(master_node, master, true);

    (*master_node)["leaves"] = shcore::Value::new_map();
    instance_owner_node = master_node->get_map("leaves");
  }

  //Inserts the read only instances
  for (auto value : *instances.get()) {

    // Gets each row
    auto row = value.as_object<mysqlsh::Row>();

    // Inserts the ones that are not the master
    if (row != master) {
      auto instance_name = row->get_member(1).as_string();
      (*instance_owner_node)[instance_name] = shcore::Value::new_map();
      auto instance_node = instance_owner_node->get_map(instance_name);
      append_member_status(instance_node, row, single_primary_mode ? false : true);
      (*instance_node)["leaves"] = shcore::Value::new_map();
    }
  }

  return ret_val;
}

shcore::Value ReplicaSet::get_description() const {
  shcore::Value ret_val = shcore::Value::new_map();
  auto description = ret_val.as_map();


  shcore::sqlstring query("SELECT mysql_server_uuid, instance_name, role, " \
                          "JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) AS host "
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

    (*instance_obj)["name"] = row->get_member(1);
    (*instance_obj)["host"] = row->get_member(3);
    (*instance_obj)["role"] = row->get_member(2);

    instance_list->push_back(instance);
  }

  return ret_val;
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
  add_method("addInstance", std::bind(&ReplicaSet::add_instance_, this, _1), "data");
  add_method("rejoinInstance", std::bind(&ReplicaSet::rejoin_instance, this, _1), "data");
  add_method("removeInstance", std::bind(&ReplicaSet::remove_instance_, this, _1), "data");
  add_varargs_method("disable", std::bind(&ReplicaSet::disable, this, _1));
  add_varargs_method("dissolve", std::bind(&ReplicaSet::dissolve, this, _1));
  add_varargs_method("checkInstanceState", std::bind(&ReplicaSet::check_instance_state, this, _1));
}


void ReplicaSet::adopt_from_gr() {
  shcore::Value ret_val;

  auto newly_discovered_instances_list(get_newly_discovered_instances());

  // Add all instances to the cluster metadata
  for (ReplicaSet::NewInstanceInfo &instance : newly_discovered_instances_list) {
    Value::Map_type_ref newly_discovered_instance(new shcore::Value::Map_type);
    (*newly_discovered_instance)["host"] = shcore::Value(instance.host);
    (*newly_discovered_instance)["port"] = shcore::Value(instance.port);

    log_info("Adopting member %s:%d from existing group",
              instance.host.c_str(),
              instance.port);

    // TODO: what if the password is different on each server?
    // And what if is different from the current session?
    auto session = _metadata_storage->get_dba()->get_active_session();
    (*newly_discovered_instance)["user"] = shcore::Value(session->get_user());
    (*newly_discovered_instance)["password"] = shcore::Value(session->get_password());

    add_instance_metadata(newly_discovered_instance);
  }
}

#if DOXYGEN_CPP
/**
 * Use this function to add a Instance to the ReplicaSet object
 * \param args : A list of values to be used to add a Instance to the ReplicaSet.
 *
 * This function returns an empty Value.
 */
#else
/**
* Adds a Instance to the ReplicaSet
* \param conn The Connection String or URI of the Instance to be added
*/
#if DOXYGEN_JS
Undefined ReplicaSet::addInstance(String conn) {}
#elif DOXYGEN_PY
None ReplicaSet::add_instance(str conn) {}
#endif
/**
* Adds a Instance to the ReplicaSet
* \param doc The Document representing the Instance to be added
*/
#if DOXYGEN_JS
Undefined ReplicaSet::addInstance(Document doc) {}
#elif DOXYGEN_PY
None ReplicaSet::add_instance(Document doc) {}
#endif
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
  if (is_local_host(hostname)) {
    return true;
  } else {
    struct hostent *he;
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
    if (datadir[datadir.size()-1] == '/' || datadir[datadir.size()-1] == '\\')
      datadir.pop_back();
    if (datadir.compare(datadir.length() - kSandboxDatadir.length(), kSandboxDatadir.length(),
                        kSandboxDatadir) != 0) {
      log_info("'%s' is a local address but not in a sandbox (datadir %s)",
               hostname.c_str(), datadir.c_str());
      throw shcore::Exception::runtime_error(
         "To add an instance to the cluster, please use a valid, non-local hostname or IP. "
         +hostname+" can only be used with sandbox MySQL instances.");
    } else {
      log_info("'%s' (%s) detected as local sandbox", hostname.c_str(), datadir.c_str());
    }
  } else {
    auto row = session->execute_sql("select @@report_host, @@hostname")->fetch_one();
    // host is not set explicitly by the user, so GR will pick hostname by default
    // now we check if this is a loopback address
    if (!row->get_value(0)) {
      if (check_if_local_host(row->get_value_as_string(1))) {
        std::string msg = "MySQL server reports hostname as being '"+row->get_value_as_string(1)+"', which may cause the cluster to be inaccessible externally. Please set report_host in MySQL to fix this.";
        log_warning("%s", msg.c_str());
      }
    }
  }
}

static void run_queries(mysqlsh::mysql::ClassicSession *session, const std::vector<std::string> &queries) {
  for (auto & q : queries) {
    log_info("DBA: run_sql(%s)", q.c_str());
    session->execute_sql(q);
  }
}

shcore::Value ReplicaSet::add_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  bool seed_instance = false;

  // NOTE: This function is called from either the add_instance_ on this class
  //       or the add_instance in Cluster class, hence this just throws exceptions
  //       and the proper handling is done on the caller functions (to append the called function name)

  // Check if we're on a addSeedInstance or not
  if (_metadata_storage->is_replicaset_empty(_id)) seed_instance = true;

  auto options = get_instance_options_map(args);

  shcore::Argument_map opt_map(*options);
  opt_map.ensure_keys({"host"}, _add_instance_opts, "instance definition");

  if (!options->has_key("port"))
    (*options)["port"] = shcore::Value(get_default_port());

  // Sets a default user if not specified
  resolve_instance_credentials(options, nullptr);
  std::string user = options->get_string(options->has_key("user") ? "user" : "dbUser");
  std::string super_user_password = options->get_string(options->has_key("password") ? "password" : "dbPassword");
  std::string joiner_host = options->get_string("host");

  // Check if the instance was already added
  std::string instance_address = joiner_host + ":" + std::to_string(options->get_int("port"));

  bool is_instance_on_md = _metadata_storage->is_instance_on_replicaset(get_id(), instance_address);
  log_debug("RS %lu: Adding instance '%s' to replicaset%s",
      static_cast<unsigned long>(_id), instance_address.c_str(),
      is_instance_on_md ? " (already in MD)" : "");

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(options));
  auto session = Dba::get_session(new_args);

  // Check whether the address being used is not in a known not-good case
  validate_instance_address(session, joiner_host, options->get_int("port"));

  GRInstanceType type = get_gr_instance_type(session->connection());

  // If type is GRInstanceType::InnoDBCluster it means that is on GR and MD
  switch (type) {
    case GRInstanceType::InnoDBCluster:
      log_debug("Instance '%s' already managed by InnoDB cluster", instance_address.c_str());
      throw shcore::Exception::runtime_error("The instance '" + instance_address + "' already belongs to the ReplicaSet: '" + get_member("name").as_string() + "'.");

    // If the instance is not on GR, we must add it
    case GRInstanceType::Standalone:
    {
      // generate a replication user account + password for this instance
      // This account will be replicated to all instances in the replicaset, so that
      // the newly joining instance can connect to any of them for recovery.
      std::string replication_user;
      std::string replication_user_password = generate_password(PASSWORD_LENGTH);

      log_debug("Instance '%s' is not yet in the cluster", instance_address.c_str());

      MySQL_timer timer;
      std::string tstamp = std::to_string(timer.get_time());
      std::string base_user = "mysql_innodb_cluster_rplusr";
      replication_user = base_user.substr(0, 32 - tstamp.size()) + tstamp;
      // TODO: Replication accounts should be created with grants for the joining instance only
      // However, we don't have a reliable way of getting the external IP and/or fully qualified domain name
      replication_user.append("@'%'");

      log_debug("Creating replication user '%s'", replication_user.c_str());
      create_repl_account(dynamic_cast<mysqlsh::mysql::ClassicSession*>(_metadata_storage->get_dba()->get_active_session().get()),
                          replication_user, replication_user_password);

      // Call the gadget to bootstrap the group with this instance
      if (seed_instance) {
        log_info("Joining '%s' to group using account %s@%s",
            instance_address.c_str(),
            user.c_str(), instance_address.c_str());
        // Call mysqlprovision to bootstrap the group using "start"
        do_join_replicaset(user + "@" + instance_address,
                           "",
                           super_user_password,
                           replication_user, replication_user_password);
      } else {
        // We need to retrieve a peer instance, so let's use the Seed one
        std::string peer_instance = get_peer_instance();
        log_info("Joining '%s' to group using account %s@%s to peer '%s'",
            instance_address.c_str(),
            user.c_str(), instance_address.c_str(), peer_instance.c_str());
        // Call mysqlprovision to do the work
        do_join_replicaset(user + "@" + instance_address,
                           user + "@" + peer_instance,
                           super_user_password,
                           replication_user, replication_user_password);
      }
    }
    break;
  case GRInstanceType::GroupReplication:
    log_debug("Instance '%s' is already part of GR, but not managed", instance_address.c_str());
    break;
  }

  // If the instance is not on the Metadata, we must add it
  if (!is_instance_on_md) {
    add_instance_metadata(options);
  }
  log_debug("Instance add finished");

  return ret_val;
}

bool ReplicaSet::do_join_replicaset(const std::string &instance_url,
                                    const std::string &peer_instance_url,
                                    const std::string &super_user_password,
                                    const std::string &repl_user, const std::string &repl_user_password) {
  shcore::Value ret_val;
  int exit_code = -1;

  bool is_seed_instance = peer_instance_url.empty() ? true : false;

  std::string command;
  shcore::Value::Array_type_ref errors;

  if (is_seed_instance) {
    exit_code = _cluster->get_provisioning_interface()->start_replicaset(instance_url,
                repl_user, super_user_password,
                repl_user_password,
                _topology_type == kTopologyMultiMaster,
                errors);
  } else {
    exit_code = _cluster->get_provisioning_interface()->join_replicaset(instance_url,
                repl_user, peer_instance_url,
                super_user_password, repl_user_password,
                _topology_type == kTopologyMultiMaster,
                errors);
  }

  if (exit_code == 0) {
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
#endif // DOXYGEN_CPP
shcore::Value ReplicaSet::rejoin_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, 2, get_function_name("rejoinInstance").c_str());
  // Check if the ReplicaSet is empty
  if (_metadata_storage->is_replicaset_empty(get_id()))
    throw shcore::Exception::runtime_error(
      "ReplicaSet not initialized. Please add the Seed Instance using: addSeedInstance().");

  // Add the Instance to the Default ReplicaSet
  try {
    std::string super_user_password;
    std::string host;
    int port = 0;

    auto options = get_instance_options_map(args);
    shcore::Argument_map opt_map(*options);
    opt_map.ensure_keys({"host"}, _add_instance_opts, "instance definition");

    if (!options->has_key("port"))
      (*options)["port"] = shcore::Value(get_default_port());

    port = options->get_int("port");

    std::string peer_instance = _metadata_storage->get_seed_instance(get_id());
    if (peer_instance.empty()) {
      throw shcore::Exception::runtime_error("Cannot rejoin instance. There are no remaining available instances in the replicaset.");
    }

    resolve_instance_credentials(options, nullptr);
    std::string user = options->get_string(options->has_key("user") ? "user" : "dbUser");
    std::string password = options->get_string(options->has_key("password") ? "password" : "dbPassword");

    host = options->get_string("host");

    do_join_replicaset(user + "@" + host + ":" + std::to_string(port),
                       user + "@" + peer_instance,
                       super_user_password,
                       "", "");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

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
  args.ensure_count(1, get_function_name("removeInstance").c_str());

  std::string uri, name, port;

  auto options = get_instance_options_map(args);

  // No required options set before refactoring
  shcore::Argument_map opt_map(*options);
  opt_map.ensure_keys({}, _remove_instance_opts, "instance definition");

  if (!options->has_key("port"))
    (*options)["port"] = shcore::Value(get_default_port());

  port = std::to_string(options->get_int("port"));

  // get instance admin and user information from the current active session of the shell
  // Note: when separate metadata session vs active session is supported, this should
  // be changed to use the active shell session
  auto instance_session(_metadata_storage->get_dba()->get_active_session());
  std::string instance_admin_user = instance_session->get_user();
  std::string instance_admin_user_password = instance_session->get_password();

  std::string host = options->get_string("host");

  // Check if the instance was already added
  std::string instance_address = host + ":" + port;

  bool is_instance_on_md = _metadata_storage->is_instance_on_replicaset(get_id(), instance_address);

  auto session = _metadata_storage->get_dba()->get_active_session();
  mysqlsh::mysql::ClassicSession *classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());

  GRInstanceType type = get_gr_instance_type(classic->connection());

  // Check if the instance exists on the ReplicaSet
  //std::string instance_address = options->get_string("host") + ":" + std::to_string(options->get_int("port"));
  if (!is_instance_on_md) {
    std::string message = "The instance '" + instance_address + "'";

    message.append(" does not belong to the ReplicaSet: '" + get_member("name").as_string() + "'.");

    throw shcore::Exception::runtime_error(message);
  }

  // If the instance is not on GR, we can remove it from the MD

  // TODO: do we remove the host? we check if is the last instance of that host and them remove?
  // auto result = _metadata_storage->remove_host(args);

  // TODO: the instance_name can be actually a name, check TODO above
  if (type == GRInstanceType::InnoDBCluster || type == GRInstanceType::GroupReplication) {
    // call provisioning to remove the instance from the replicaset
    int exit_code = -1;
    std::string instance_url;
    shcore::Value::Array_type_ref errors;

    instance_url = instance_admin_user + "@";

    instance_url.append(host);

    instance_url.append(":" + port);

    exit_code = _cluster->get_provisioning_interface()->leave_replicaset(instance_url, instance_admin_user_password, errors);

    if (exit_code != 0)
      throw shcore::Exception::runtime_error(get_mysqlprovision_error_string(errors));

    // Drop replication user
    shcore::Argument_list temp_args, new_args;

    (*options)["dbUser"] = shcore::Value(instance_admin_user);
    (*options)["dbPassword"] = shcore::Value(instance_admin_user_password);
    new_args.push_back(shcore::Value(options));

    auto session = mysqlsh::connect_session(new_args, mysqlsh::SessionType::Classic);
    mysqlsh::mysql::ClassicSession *classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());

    temp_args.clear();
    temp_args.push_back(shcore::Value("SET sql_log_bin = 0"));
    classic->run_sql(temp_args);

    // TODO: This is NO longer deleting anything since the user was created using a timestamp
    // instead of the port
    std::string replication_user = "mysql_innodb_cluster_rplusr" + port;
    run_queries(classic, {
      "DROP USER IF EXISTS '" + replication_user + "'@'" + host + "'"
    });

    temp_args.clear();
    temp_args.push_back(shcore::Value("SET sql_log_bin = 1"));
    classic->run_sql(temp_args);
  }

  // Remove it from the MD
  if (is_instance_on_md)
    remove_instance_metadata(args);

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

    if (force)
      disable(shcore::Argument_list());
    else if (_metadata_storage->is_replicaset_active(get_id()))
      throw shcore::Exception::runtime_error("Cannot dissolve the ReplicaSet: the ReplicaSet is active.");

    MetadataStorage::Transaction tx(_metadata_storage);

    // remove all the instances from the ReplicaSet
    auto instances = _metadata_storage->get_replicaset_instances(get_id());

    for (auto value : *instances.get()) {
      auto row = value.as_object<mysqlsh::Row>();

      std::string instance_name = row->get_member(1).as_string();
      shcore::Argument_list args_instance;
      args_instance.push_back(shcore::Value(instance_name));

      remove_instance(args_instance);
    }

    // Remove the replicaSet
    _metadata_storage->drop_replicaset(get_id());

    tx.commit();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dissolve"));

  return ret_val;
}

shcore::Value ReplicaSet::disable(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  args.ensure_count(0, get_function_name("disable").c_str());

  try {
    MetadataStorage::Transaction tx(_metadata_storage);
    auto instance_session(_metadata_storage->get_dba()->get_active_session());
    std::string instance_admin_user = instance_session->get_user();
    std::string instance_admin_user_password = instance_session->get_password();

    // Get all instances of the replicaset
    auto instances = _metadata_storage->get_replicaset_instances(get_id());

    for (auto value : *instances.get()) {
      int exit_code;
      auto row = value.as_object<mysqlsh::Row>();

      std::string instance_name = row->get_member(1).as_string();

      shcore::Value::Map_type_ref data = shcore::get_connection_data(instance_name, false);
      if (data->has_key("host")) {
        auto host = data->get_string("host");
      }

      std::string instance_url = instance_admin_user + "@" + instance_name;
      shcore::Value::Array_type_ref errors;

      // Leave the replicaset
      exit_code = _cluster->get_provisioning_interface()->leave_replicaset(instance_url,
                  instance_admin_user_password, errors);
      if (exit_code != 0)
        throw shcore::Exception::runtime_error(get_mysqlprovision_error_string(errors));
    }

    // Update the metadata to turn 'active' off
    _metadata_storage->disable_replicaset(get_id());
    tx.commit();
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

  std::vector<NewInstanceInfo> newly_discovered_instances_list = get_newly_discovered_instances();

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

  std::vector<MissingInstanceInfo> unavailable_instances_list = get_unavailable_instances();

  // Creates the unavailableInstances array
  shcore::Value::Array_type_ref unavailable_instances(new shcore::Value::Array_type());

  for (auto &instance : unavailable_instances_list) {
    shcore::Value::Map_type_ref unavailable_instance(new shcore::Value::Map_type());
    (*unavailable_instance)["member_id"] = shcore::Value(instance.id);
    (*unavailable_instance)["name"] = shcore::Value(instance.name);
    (*unavailable_instance)["host"] = shcore::Value(instance.host);

    unavailable_instances->push_back(shcore::Value(unavailable_instance));
  }
  // Add the missing_instances list to the result Map
  (*ret_val)["unavailableInstances"] = shcore::Value(unavailable_instances);

  return ret_val;
}

std::string ReplicaSet::get_peer_instance() {
  std::vector<std::string> result;
  auto session = dynamic_cast<mysqlsh::mysql::ClassicSession*>(_metadata_storage->get_dba()->get_active_session().get());

  // We need to retrieve a peer instance, so let's use the Seed one
  auto instances = _metadata_storage->get_replicaset_instances(get_id());
  if (instances) {
    for (auto value : *instances) {
      auto row = value.as_object<mysqlsh::Row>();
      std::string peer_instance = row->get_member("instance_name").as_string();
      result.push_back(peer_instance);
    }
  }
  return result.front();
}

/**
 * Create an account in the replicaset.
 */
void ReplicaSet::create_repl_account(mysqlsh::mysql::ClassicSession *session,
                                     const std::string &username,
                                     const std::string &password) {
  try {
    run_queries(session, {
      "START TRANSACTION",
      "DROP USER IF EXISTS " + username,
      "CREATE USER IF NOT EXISTS " + username + " IDENTIFIED BY '" + password + "'",
      "GRANT REPLICATION SLAVE ON *.* to " + username,
      "COMMIT"
    });
  } catch (...) {
    session->execute_sql("ROLLBACK");
    throw;
  }
}

static std::string generate_password(int password_lenght) {
  std::random_device rd;
  std::string pwd;
  static const char *alphabet = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~@#%$^&*()-_=+]}[{|;:.>,</?";
  std::uniform_int_distribution<int> dist(0, strlen(alphabet) - 1);

  for (int i = 0; i < password_lenght; i++)
    pwd += alphabet[dist(rd)];

  return pwd;
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

  auto options = get_instance_options_map(args);

  shcore::Argument_map opt_map(*options);
  opt_map.ensure_keys({"host"}, _add_instance_opts, "instance definition");


  if (!options->has_key("port"))
    (*options)["port"] = shcore::Value(get_default_port());

  // Sets a default user if not specified
  resolve_instance_credentials(options, nullptr);

  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(options));
  auto instance_session = Dba::get_session(new_args);

  // We will work with the current global session
  // Assuming it is the R/W instance
  auto master_dev_session = _metadata_storage->get_dba()->get_active_session();
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
  switch(state) {
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

void ReplicaSet::add_instance_metadata(const shcore::Value::Map_type_ref &instance_definition) {
  log_debug("Adding instance to metadata");

  MetadataStorage::Transaction tx(_metadata_storage);

  int xport = instance_definition->get_int("port") * 10;

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
  (*instance_definition)["mysql_server_uuid"] = shcore::Value(mysql_server_uuid);

  if (!instance_definition->has_key("name"))
    (*instance_definition)["instance_name"] = shcore::Value(instance_address);

  // update the metadata with the host
  uint32_t host_id = _metadata_storage->insert_host(instance_definition);

  // And the instance
  _metadata_storage->insert_instance(instance_definition, host_id, get_id());

  tx.commit();
}

void ReplicaSet::remove_instance_metadata(const shcore::Argument_list &args) {
  MetadataStorage::Transaction tx(_metadata_storage);

  auto options = get_instance_options_map(args, true);

  std::string port = std::to_string(options->get_int("port"));

  std::string host = options->get_string("host");

  // Check if the instance was already added
  std::string instance_address = host + ":" + port;

  _metadata_storage->remove_instance(instance_address);

  tx.commit();
}

std::vector<std::string> ReplicaSet::get_instances_gr() {
  // Get the list of instances belonging to the GR group
  std::string query = "SELECT member_id FROM performance_schema.replication_group_members";

  auto result = _metadata_storage->execute_sql(query);
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

std::vector<std::string> ReplicaSet::get_instances_md() {
  // Get the list of instances registered on the Metadata
  shcore::sqlstring query("SELECT mysql_server_uuid FROM mysql_innodb_cluster_metadata.instances " \
                          "WHERE replicaset_id = ?", 0);
  query << _id;
  query.done();

  auto result = _metadata_storage->execute_sql(query);
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

std::vector<ReplicaSet::NewInstanceInfo> ReplicaSet::get_newly_discovered_instances() {
  std::vector<std::string> instances_gr_array, instances_md_array;
  std::string query;

  instances_gr_array = get_instances_gr();
  instances_md_array = get_instances_md();

  // Check the differences between the two lists
  std::vector<std::string> new_members;

  // Check if the instances_gr list has more members than the instances_md lists
  // Meaning that an instance was added to the GR group outside of the AdminAPI
  std::set_difference(instances_gr_array.begin(), instances_gr_array.end(), instances_md_array.begin(),
                      instances_md_array.end(), std::inserter(new_members, new_members.begin()));

  std::vector<NewInstanceInfo> ret;
  for (auto i : new_members) {
    shcore::sqlstring query("SELECT MEMBER_ID, MEMBER_HOST, MEMBER_PORT " \
                            "FROM performance_schema.replication_group_members " \
                            "WHERE MEMBER_ID = ?", 0);
    query << i;
    query.done();

    auto result = _metadata_storage->execute_sql(query);
    auto row = result->fetch_one();

    NewInstanceInfo info;
    info.member_id = row->get_value(0).as_string();
    info.host = row->get_value(1).as_string();
    info.port = row->get_value(2).as_int();
    ret.push_back(info);
  }

  return ret;
}

std::vector<ReplicaSet::MissingInstanceInfo> ReplicaSet::get_unavailable_instances() {
  std::vector<std::string> instances_gr_array, instances_md_array;

  instances_gr_array = get_instances_gr();
  instances_md_array = get_instances_md();

  // Check the differences between the two lists
  std::vector<std::string> removed_members;

  // Check if the instances_md list has more members than the instances_gr lists
  // Meaning that an instance was removed from the GR group outside of the AdminAPI
  std::set_difference(instances_md_array.begin(), instances_md_array.end(), instances_gr_array.begin(),
                      instances_gr_array.end(), std::inserter(removed_members, removed_members.begin()));

  std::vector<MissingInstanceInfo> ret;
  for (auto i : removed_members) {
    shcore::sqlstring query("SELECT mysql_server_uuid, instance_name, " \
                            "JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) AS host " \
                            "FROM mysql_innodb_cluster_metadata.instances " \
                            "WHERE mysql_server_uuid = ?", 0);
    query << i;
    query.done();

    auto result = _metadata_storage->execute_sql(query);
    auto row = result->fetch_one();
    MissingInstanceInfo info;
    info.id = row->get_value(0).as_string();
    info.name = row->get_value(1).as_string();
    info.host = row->get_value(2).as_string();
    ret.push_back(info);
  }

  return ret;
}
