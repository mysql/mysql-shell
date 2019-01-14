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

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_options.h"
#include "modules/adminapi/cluster/cluster_set_option.h"
#include "modules/adminapi/cluster/cluster_status.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/replicaset/remove_instance.h"
#include "modules/adminapi/replicaset/replicaset.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "shellcore/utils_help.h"
#include "utils/debug.h"
#include "utils/utils_general.h"

using std::placeholders::_1;

DEBUG_OBJ_ENABLE(Cluster);

namespace mysqlsh {
namespace dba {

// Documentation of the Cluster Class
REGISTER_HELP_CLASS(Cluster, adminapi);
REGISTER_HELP(CLUSTER_BRIEF, "Represents an InnoDB cluster.");
REGISTER_HELP(CLUSTER_DETAIL,
              "The cluster object is the entry point to manage and monitor "
              "a MySQL InnoDB cluster.");
REGISTER_HELP(
    CLUSTER_DETAIL1,
    "A cluster is a set of MySQLd Instances which holds the user's data.");
REGISTER_HELP(
    CLUSTER_DETAIL2,
    "It provides high-availability and scalability for the user's data.");

REGISTER_HELP(
    CLUSTER_CLOSING,
    "For more help on a specific function use: cluster.help('<functionName>')");
REGISTER_HELP(CLUSTER_CLOSING1, "e.g. cluster.help('addInstance')");

Cluster::Cluster(const std::string &name,
                 std::shared_ptr<mysqlshdk::db::ISession> group_session,
                 std::shared_ptr<MetadataStorage> metadata_storage)
    : _name(name),
      m_invalidated(false),
      _group_session(group_session),
      _metadata_storage(metadata_storage) {
  DEBUG_OBJ_ALLOC2(Cluster, [](void *ptr) {
    return "refs:" + std::to_string(reinterpret_cast<Cluster *>(ptr)
                                        ->shared_from_this()
                                        .use_count());
  });
  init();
}

Cluster::~Cluster() { DEBUG_OBJ_DEALLOC(Cluster); }

std::string &Cluster::append_descr(std::string &s_out, int UNUSED(indent),
                                   int UNUSED(quote_strings)) const {
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

void Cluster::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();
  dumper.append_string("class", class_name());
  dumper.append_string("name", _name);
  dumper.end_object();
}

bool Cluster::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Cluster::init() {
  add_property("name", "getName");
  add_method("addInstance", std::bind(&Cluster::add_instance, this, _1),
             "data");
  add_method("rejoinInstance", std::bind(&Cluster::rejoin_instance, this, _1),
             "data");
  add_method("removeInstance", std::bind(&Cluster::remove_instance, this, _1),
             "data");
  add_method("describe", std::bind(&Cluster::describe, this, _1));
  add_method("status", std::bind(&Cluster::status, this, _1));
  add_varargs_method("dissolve", std::bind(&Cluster::dissolve, this, _1));

  expose<shcore::Value, const std::string &, Cluster>(
      "checkInstanceState", &Cluster::check_instance_state, "instanceDef");
  expose<shcore::Value, const shcore::Dictionary_t &, Cluster>(
      "checkInstanceState", &Cluster::check_instance_state, "instanceDef");

  expose("rescan", &Cluster::rescan, "?options");
  add_varargs_method(
      "forceQuorumUsingPartitionOf",
      std::bind(&Cluster::force_quorum_using_partition_of, this, _1));
  add_method("disconnect", std::bind(&Cluster::disconnect, this, _1));

  expose<void, const std::string &, Cluster>(
      "switchToSinglePrimaryMode", &Cluster::switch_to_single_primary_mode,
      "instanceDef");
  expose<void, const shcore::Dictionary_t &, Cluster>(
      "switchToSinglePrimaryMode", &Cluster::switch_to_single_primary_mode,
      "instanceDef");
  expose("switchToSinglePrimaryMode", &Cluster::switch_to_single_primary_mode);
  expose("switchToMultiPrimaryMode", &Cluster::switch_to_multi_primary_mode);
  expose<void, const std::string &, Cluster>(
      "setPrimaryInstance", &Cluster::set_primary_instance, "instanceDef");
  expose<void, const shcore::Dictionary_t &, Cluster>(
      "setPrimaryInstance", &Cluster::set_primary_instance, "instanceDef");

  expose("options", &Cluster::options, "?options");

  expose("setOption", &Cluster::set_option, "option", "value");

  expose<void, const shcore::Dictionary_t &, const std::string &,
         const shcore::Value &, Cluster>("setInstanceOption",
                                         &Cluster::set_instance_option,
                                         "instanceDef", "option", "value");
  expose<void, const std::string &, const std::string &, const shcore::Value &,
         Cluster>("setInstanceOption", &Cluster::set_instance_option,
                  "instanceDef", "option", "value");
}

// Documentation of the getName function
REGISTER_HELP_FUNCTION(getName, Cluster);
REGISTER_HELP_PROPERTY(name, Cluster);
REGISTER_HELP(CLUSTER_GETNAME_BRIEF, "Retrieves the name of the cluster.");
REGISTER_HELP(CLUSTER_NAME_BRIEF, "${CLUSTER_GETNAME_BRIEF}");
REGISTER_HELP(CLUSTER_GETNAME_RETURNS, "@returns The name of the cluster.");

/**
 * $(CLUSTER_GETNAME_BRIEF)
 *
 * $(CLUSTER_GETNAME_RETURNS)
 *
 */
#if DOXYGEN_JS
String Cluster::getName() {}
#elif DOXYGEN_PY
str Cluster::get_name() {}
#endif

shcore::Value Cluster::call(const std::string &name,
                            const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid(name);
  return Cpp_object_bridge::call(name, args);
}

shcore::Value Cluster::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  // Throw an error if the cluster has already been dissolved
  assert_valid(prop);

  if (prop == "name")
    ret_val = shcore::Value(_name);
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);
  return ret_val;
}

std::shared_ptr<mysqlshdk::innodbcluster::Metadata_mysql> Cluster::metadata()
    const {
  return _metadata_storage->get_new_metadata();
}

void Cluster::assert_valid(const std::string &option_name) const {
  std::string name;

  if (option_name == "disconnect") return;

  if (has_member(option_name) && m_invalidated) {
    if (has_method(option_name)) {
      name = get_function_name(option_name, false);
      throw shcore::Exception::runtime_error(class_name() + "." + name + ": " +
                                             "Can't call function '" + name +
                                             "' on a dissolved cluster");
    } else {
      name = get_member_name(option_name, naming_style);
      throw shcore::Exception::runtime_error(class_name() + "." + name + ": " +
                                             "Can't access object member '" +
                                             name + "' on a dissolved cluster");
    }
  }
  if (!_group_session) {
    throw shcore::Exception::runtime_error(
        "The cluster object is disconnected. Please use <Dba>." +
        get_function_name("getCluster", false) +
        " to obtain a fresh cluster handle.");
  }
}

#if 0
#if DOXYGEN_CPP
/**
 * Use this function to add a Seed Instance to the Cluster object
 * \param args : A list of values to be used to add a Seed Instance to the Cluster.
 *
 * This function creates the Default ReplicaSet implicitly and adds the Instance to it
 * This function returns an empty Value.
 */
#else
/**
* Adds a Seed Instance to the Cluster
* \param conn The Connection String or URI of the Instance to be added
*/
#if DOXYGEN_JS
Undefined addSeedInstance(
    String conn, String root_password, String topology_type) {}
#elif DOXYGEN_PY
None add_seed_instance(str conn, str root_password, str topology_type) {}
#endif
/**
* Adds a Seed Instance to the Cluster
* \param doc The Document representing the Instance to be added
*/
#if DOXYGEN_JS
Undefined addSeedInstance(Document doc) {}
#elif DOXYGEN_PY
None add_seed_instance(Document doc) {}
#endif
#endif
#endif
shcore::Value Cluster::add_seed_instance(
    const mysqlshdk::db::Connection_options &connection_options,
    const shcore::Argument_list &args, bool multi_primary, bool is_adopted,
    const std::string &replication_user, const std::string &replication_pwd,
    const std::string &group_name) {
  shcore::Value ret_val;

  MetadataStorage::Transaction tx(_metadata_storage);
  std::shared_ptr<ReplicaSet> default_rs = get_default_replicaset();

  // Check if we have a Default ReplicaSet, if so it means we already added the
  // Seed Instance
  if (default_rs != NULL) {
    uint64_t rs_id = default_rs->get_id();
    if (!_metadata_storage->is_replicaset_empty(rs_id))
      throw shcore::Exception::logic_error(
          "Default ReplicaSet already initialized. Please use: addInstance() "
          "to add more Instances to the ReplicaSet.");
  } else {
    // Create the Default ReplicaSet and assign it to the Cluster's
    // default_replica_set var
    create_default_replicaset("default", multi_primary, "", is_adopted);
  }
  if (!is_adopted) {
    // Add the Instance to the Default ReplicaSet passing already created
    // replication user and the group_name (if provided)
    ret_val = _default_replica_set->add_instance(
        connection_options, args, replication_user, replication_pwd, true,
        group_name, true);
  }
  std::string group_replication_group_name =
      get_gr_replicaset_group_name(_group_session);
  _default_replica_set->set_group_name(group_replication_group_name);

  tx.commit();

  return ret_val;
}

REGISTER_HELP_FUNCTION(addInstance, Cluster);
REGISTER_HELP(CLUSTER_ADDINSTANCE_BRIEF, "Adds an Instance to the cluster.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_PARAM,
              "@param instance An instance "
              "definition.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_PARAM1,
              "@param options Optional dictionary "
              "with options for the operation.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS,
              "MetadataError in the following scenarios:");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS1,
              "@li If the "
              "Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS2,
              "@li If the "
              "Metadata update operation failed.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS3,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS4,
              "@li If the "
              "instance parameter is empty.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS5,
              "@li If the "
              "instance definition is invalid.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS6,
              "@li If the "
              "instance definition is a "
              "connection dictionary but empty.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS7,
              "@li If the "
              "value for the memberSslMode "
              "option is not one of the "
              "allowed: \"AUTO\", \"DISABLED\", "
              "\"REQUIRED\".");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS8,
              "@li If the value for the ipWhitelist, "
              "localAddress, groupSeeds, or exitStateAction options is empty.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS9,
              "@li If the instance definition cannot be used "
              "for Group Replication.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS10,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS11,
              "@li If the "
              "instance accounts are invalid.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS12,
              "@li If the "
              "instance is not in bootstrapped "
              "state.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS13,
              "@li If the SSL "
              "mode specified is not compatible "
              "with the one used in the cluster.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_THROWS14,
              "@li If the value for the localAddress, "
              "groupSeeds, exitStateAction, or memberWeight options is not "
              "valid for Group "
              "Replication.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_RETURNS, "@returns nothing");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL,
              "This function adds an Instance to "
              "the default replica set of the "
              "cluster.");

REGISTER_HELP(
    CLUSTER_ADDINSTANCE_DETAIL1,
    "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL2,
              "${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL3,
              "The options dictionary may contain "
              "the following attributes:");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL4,
              "@li label: an identifier for the "
              "instance being added");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL5,
              "@li password: the instance "
              "connection password");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL6,
              "@li memberSslMode: SSL mode used "
              "on the instance");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL7,
              "@li ipWhitelist: The list of "
              "hosts allowed to connect to the "
              "instance for group replication");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL8,
              "@li localAddress: string value with the Group Replication "
              "local address to be used instead of the automatically "
              "generated one.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL9,
              "@li groupSeeds: string value with a comma-separated list of "
              "the Group Replication peer addresses to be used instead of the "
              "automatically generated one.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL10,
              "@li exitStateAction: string value indicating the group "
              "replication exit state action.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL11,
              "@li memberWeight: integer value with a percentage weight for "
              "automatic primary election on failover.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL12,
              "The password may be contained on "
              "the instance definition, "
              "however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL13,
              "@attention The memberSslMode option will be removed in a "
              "future release.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL14,
              "The memberSslMode option supports "
              "the following values:");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL15,
              "@li REQUIRED: if used, SSL "
              "(encryption) will be enabled for "
              "the instance to communicate with "
              "other members of the cluster");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL16,
              "@li DISABLED: if used, SSL "
              "(encryption) will be disabled");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL17,
              "@li AUTO: if used, SSL (encryption)"
              " will be automatically "
              "enabled or disabled based on the "
              "cluster configuration");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL18,
              "If memberSslMode is not specified "
              "AUTO will be used by default.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL19,
              "The exitStateAction option supports the following values:");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL20,
              "@li ABORT_SERVER: if used, the instance shuts itself down if "
              "it leaves the cluster unintentionally.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL21,
              "@li READ_ONLY: if used, the instance switches itself to "
              "super-read-only mode if it leaves the cluster "
              "unintentionally.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL22,
              "If exitStateAction is not specified READ_ONLY will be used "
              "by default.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL23,
              "The ipWhitelist format is a comma "
              "separated list of IP "
              "addresses or subnet CIDR "
              "notation, for example: "
              "192.168.1.0/24,10.0.0.1. "
              "By default the "
              "value is set to AUTOMATIC, "
              "allowing addresses from the "
              "instance private network to be "
              "automatically set for "
              "the whitelist.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL24,
              "The localAddress and groupSeeds are advanced options and "
              "their usage is discouraged since incorrect values can lead to "
              "Group Replication errors.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL25,
              "The value for localAddress is used to set the Group "
              "Replication system variable 'group_replication_local_address'. "
              "The localAddress option accepts values in the format: "
              "'host:port' or 'host:' or ':port'. If the specified "
              "value does not include a colon (:) and it is numeric, then it "
              "is assumed to be the port, otherwise it is considered to be the "
              "host. When the host is not specified, the default value is the "
              "value of the system variable 'report_host' if defined (i.e., "
              "not 'NULL'), otherwise it is the hostname value. When "
              "the port is not specified, the default value is the port of "
              "the target instance * 10 + 1. In case the automatically "
              "determined default port value is invalid (> 65535) then a "
              "random value in the range [10000, 65535] is used.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL26,
              "The value for groupSeeds is used to set the Group Replication "
              "system variable 'group_replication_group_seeds'. The "
              "groupSeeds option accepts a comma-separated list of addresses "
              "in the format: 'host1:port1,...,hostN:portN'.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL27,
              "The value for exitStateAction is used to configure how Group "
              "Replication behaves when a server instance leaves the group "
              "unintentionally, for example after encountering an applier "
              "error. When set to ABORT_SERVER, the instance shuts itself "
              "down, and when set to READ_ONLY the server switches itself to "
              "super-read-only mode. The exitStateAction option accepts "
              "case-insensitive string values, being the accepted values: "
              "ABORT_SERVER (or 1) and READ_ONLY (or 0). The default value is "
              "READ_ONLY.");

REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL28,
              "The value for memberWeight is used to set the Group Replication "
              "system variable 'group_replication_member_weight'. The "
              "memberWeight option accepts integer values. Group Replication "
              "limits the value range from 0 to 100, automatically adjusting "
              "it if a lower/bigger value is provided. Group Replication uses "
              "a default value of 50 if no value is provided.");

/**
 * $(CLUSTER_ADDINSTANCE_BRIEF)
 *
 * $(CLUSTER_ADDINSTANCE_PARAM)
 * $(CLUSTER_ADDINSTANCE_PARAM1)
 *
 * $(CLUSTER_ADDINSTANCE_THROWS)
 * $(CLUSTER_ADDINSTANCE_THROWS1)
 * $(CLUSTER_ADDINSTANCE_THROWS2)
 * $(CLUSTER_ADDINSTANCE_THROWS3)
 * $(CLUSTER_ADDINSTANCE_THROWS4)
 * $(CLUSTER_ADDINSTANCE_THROWS5)
 * $(CLUSTER_ADDINSTANCE_THROWS6)
 * $(CLUSTER_ADDINSTANCE_THROWS7)
 * $(CLUSTER_ADDINSTANCE_THROWS8)
 * $(CLUSTER_ADDINSTANCE_THROWS9)
 * $(CLUSTER_ADDINSTANCE_THROWS10)
 * $(CLUSTER_ADDINSTANCE_THROWS11)
 * $(CLUSTER_ADDINSTANCE_THROWS12)
 * $(CLUSTER_ADDINSTANCE_THROWS13)
 * $(CLUSTER_ADDINSTANCE_THROWS14)
 *
 * $(CLUSTER_ADDINSTANCE_RETURNS)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL1)
 *
 * $(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY1)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL3)
 * $(CLUSTER_ADDINSTANCE_DETAIL4)
 * $(CLUSTER_ADDINSTANCE_DETAIL5)
 * $(CLUSTER_ADDINSTANCE_DETAIL6)
 * $(CLUSTER_ADDINSTANCE_DETAIL7)
 * $(CLUSTER_ADDINSTANCE_DETAIL8)
 * $(CLUSTER_ADDINSTANCE_DETAIL9)
 * $(CLUSTER_ADDINSTANCE_DETAIL10)
 * $(CLUSTER_ADDINSTANCE_DETAIL11)
 * $(CLUSTER_ADDINSTANCE_DETAIL12)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL13)
 * $(CLUSTER_ADDINSTANCE_DETAIL14)
 * $(CLUSTER_ADDINSTANCE_DETAIL15)
 * $(CLUSTER_ADDINSTANCE_DETAIL16)
 * $(CLUSTER_ADDINSTANCE_DETAIL17)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL18)
 * $(CLUSTER_ADDINSTANCE_DETAIL19)
 * $(CLUSTER_ADDINSTANCE_DETAIL20)
 * $(CLUSTER_ADDINSTANCE_DETAIL21)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL22)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL23)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL24)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL25)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL26)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL27)
 *
 * $(CLUSTER_ADDINSTANCE_DETAIL28)
 */
#if DOXYGEN_JS
Undefined Cluster::addInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::add_instance(InstanceDef instance, dict options) {}
#endif
shcore::Value Cluster::add_instance(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("addInstance");

  args.ensure_count(1, 2, get_function_name("addInstance").c_str());

  // Add the Instance to the Default ReplicaSet
  shcore::Value ret_val;
  try {
    check_preconditions("addInstance");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    auto connection_options =
        mysqlsh::get_connection_options(args, PasswordFormat::OPTIONS);

    validate_connection_options(connection_options);

    // Validate if the connection host is resolved to a supported IP address.
    validate_host_ip(connection_options.get_host());

    shcore::Argument_list rest;
    if (args.size() == 2) rest.push_back(args.at(1));

    ret_val = _default_replica_set->add_instance(connection_options, rest);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(rejoinInstance, Cluster);
REGISTER_HELP(CLUSTER_REJOININSTANCE_BRIEF,
              "Rejoins an Instance to the cluster.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_PARAM,
              "@param instance An instance "
              "definition.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_PARAM1,
              "@param options Optional dictionary "
              "with options for the operation.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS,
              "MetadataError in the following scenarios:");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS1,
              "@li If the "
              "Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS2,
              "@li If the "
              "Metadata update operation failed.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS3,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS4,
              "@li If the "
              "value for the memberSslMode "
              "option is not one of the allowed: "
              "\"AUTO\", \"DISABLED\", \"REQUIRED\".");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS5,
              "@li If the instance definition cannot be used "
              "for Group Replication.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS6,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS7,
              "@li If the "
              "instance does not exist.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS8,
              "@li If the "
              "instance accounts are invalid.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS9,
              "@li If the "
              "instance is not in bootstrapped "
              "state.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS10,
              "@li If the SSL "
              "mode specified is not compatible "
              "with the one used in the cluster.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_THROWS11,
              "@li If the "
              "instance is an active member "
              "of the ReplicaSet.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL,
              "This function rejoins an Instance "
              "to the cluster.");

REGISTER_HELP(
    CLUSTER_REJOININSTANCE_DETAIL1,
    "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL2,
              "${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL3,
              "The options dictionary may "
              "contain the following attributes:");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL4,
              "@li label: an identifier for "
              "the instance being added");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL5,
              "@li password: the instance "
              "connection password");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL6,
              "@li memberSslMode: SSL mode used "
              "on the instance");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL7,
              "@li ipWhitelist: The list of "
              "hosts allowed to connect to the "
              "instance for group replication");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL8,
              "The password may be contained "
              "on the instance definition, "
              "however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL9,
              "@attention The memberSslMode option will be removed in a "
              "future release.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL10,
              "The memberSslMode option supports "
              "these values:");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL11,
              "@li REQUIRED: if used, SSL "
              "(encryption) will be enabled "
              "for the instance to communicate "
              "with other members of the cluster");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL12,
              "@li DISABLED: if used, SSL "
              "(encryption) will be disabled");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL13,
              "@li AUTO: if used, SSL "
              "(encryption) will be automatically "
              "enabled or disabled based on the cluster "
              "configuration");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL14,
              "If memberSslMode is not specified "
              "AUTO will be used by default.");

REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL15,
              "The ipWhitelist format is a "
              "comma separated list of IP "
              "addresses or subnet CIDR notation, "
              "for example: 192.168.1.0/24,10.0.0.1. "
              "By default the value is set to "
              "AUTOMATIC, allowing addresses "
              "from the instance private network "
              "to be automatically set for the whitelist.");

/**
 * $(CLUSTER_REJOININSTANCE_BRIEF)
 *
 * $(CLUSTER_REJOININSTANCE_PARAM)
 * $(CLUSTER_REJOININSTANCE_PARAM1)
 *
 * $(CLUSTER_REJOININSTANCE_THROWS)
 * $(CLUSTER_REJOININSTANCE_THROWS1)
 * $(CLUSTER_REJOININSTANCE_THROWS2)
 * $(CLUSTER_REJOININSTANCE_THROWS3)
 * $(CLUSTER_REJOININSTANCE_THROWS4)
 * $(CLUSTER_REJOININSTANCE_THROWS5)
 * $(CLUSTER_REJOININSTANCE_THROWS6)
 * $(CLUSTER_REJOININSTANCE_THROWS7)
 * $(CLUSTER_REJOININSTANCE_THROWS8)
 * $(CLUSTER_REJOININSTANCE_THROWS9)
 * $(CLUSTER_REJOININSTANCE_THROWS10)
 * $(CLUSTER_REJOININSTANCE_THROWS11)
 *
 * $(CLUSTER_REJOININSTANCE_RETURNS)
 *
 * $(CLUSTER_REJOININSTANCE_DETAIL)
 *
 * $(CLUSTER_REJOININSTANCE_DETAIL1)
 *
 * $(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY1)
 *
 * $(CLUSTER_REJOININSTANCE_DETAIL3)
 * $(CLUSTER_REJOININSTANCE_DETAIL4)
 * $(CLUSTER_REJOININSTANCE_DETAIL5)
 * $(CLUSTER_REJOININSTANCE_DETAIL6)
 * $(CLUSTER_REJOININSTANCE_DETAIL7)
 *
 * $(CLUSTER_REJOININSTANCE_DETAIL8)
 *
 * $(CLUSTER_REJOININSTANCE_DETAIL9)
 *
 * $(CLUSTER_REJOININSTANCE_DETAIL10)
 * $(CLUSTER_REJOININSTANCE_DETAIL11)
 * $(CLUSTER_REJOININSTANCE_DETAIL12)
 * $(CLUSTER_REJOININSTANCE_DETAIL13)
 * $(CLUSTER_REJOININSTANCE_DETAIL14)
 *
 * $(CLUSTER_REJOININSTANCE_DETAIL15)
 */
#if DOXYGEN_JS
Undefined Cluster::rejoinInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::rejoin_instance(InstanceDef instance, dict options) {}
#endif

shcore::Value Cluster::rejoin_instance(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("rejoinInstance");

  args.ensure_count(1, 2, get_function_name("rejoinInstance").c_str());

  // rejoin the Instance to the Default ReplicaSet
  shcore::Value ret_val;
  try {
    check_preconditions("rejoinInstance");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    auto instance_def =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

    validate_connection_options(instance_def);

    shcore::Value::Map_type_ref options;

    if (args.size() == 2) options = args.map_at(1);

    // if not, call mysqlprovision to join the instance to its own group
    ret_val = _default_replica_set->rejoin_instance(&instance_def, options);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(removeInstance, Cluster);
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_BRIEF,
              "Removes an Instance from the cluster.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_PARAM,
              "@param instance An instance definition.");
REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_PARAM1,
    "@param options Optional dictionary with options for the operation.");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS,
              "MetadataError in the following scenarios:");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS1,
              "@li If the "
              "Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS2,
              "@li If the "
              "Metadata update operation failed.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS3,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS4,
              "@li If the instance parameter is empty.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS5,
              "@li If the instance definition is invalid.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS6,
              "@li If the instance definition is a "
              "connection dictionary but empty.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS7,
              "@li If the instance definition cannot be used "
              "for Group Replication.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS8,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS9,
              "@li If the instance accounts are invalid.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_THROWS10,
              "@li If an error occurs when trying to remove "
              "the instance "
              "(e.g., instance is not reachable).");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL,
              "This function removes an "
              "Instance from the default "
              "replicaSet of the cluster.");

REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_DETAIL1,
    "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL2,
              "${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}");

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL3,
              "The options dictionary may contain the following attributes:");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL4,
              "@li password: the instance connection password");
REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_DETAIL5,
    "@li force: boolean, indicating if the instance must be removed (even if "
    "only from metadata) in case it cannot be reached. By default, set to "
    "false.");
REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_DETAIL6,
    "@li interactive: boolean value used to disable/enable the wizards in the "
    "command execution, i.e. prompts and confirmations will be provided or "
    "not according to the value set. The default value is equal to MySQL Shell "
    "wizard mode.");
REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_DETAIL7,
    "The password may be contained in the instance definition, however, it can "
    "be overwritten if it is specified on the options.");
REGISTER_HELP(
    CLUSTER_REMOVEINSTANCE_DETAIL8,
    "The force option (set to true) must only be used to remove instances that "
    "are permanently not available (no longer reachable) or never to be reused "
    "again in a cluster. This allows to remove from the metadata an instance "
    "than can no longer be recovered. Otherwise, the instance must be brought "
    "back ONLINE and removed without the force option to avoid errors trying "
    "to add it back to a cluster.");

/**
 * $(CLUSTER_REMOVEINSTANCE_BRIEF)
 *
 * $(CLUSTER_REMOVEINSTANCE_PARAM)
 * $(CLUSTER_REMOVEINSTANCE_PARAM1)
 *
 * $(CLUSTER_REMOVEINSTANCE_THROWS)
 * $(CLUSTER_REMOVEINSTANCE_THROWS1)
 * $(CLUSTER_REMOVEINSTANCE_THROWS2)
 * $(CLUSTER_REMOVEINSTANCE_THROWS3)
 * $(CLUSTER_REMOVEINSTANCE_THROWS4)
 * $(CLUSTER_REMOVEINSTANCE_THROWS5)
 * $(CLUSTER_REMOVEINSTANCE_THROWS6)
 * $(CLUSTER_REMOVEINSTANCE_THROWS7)
 * $(CLUSTER_REMOVEINSTANCE_THROWS8)
 * $(CLUSTER_REMOVEINSTANCE_THROWS9)
 * $(CLUSTER_REMOVEINSTANCE_THROWS10)
 *
 * $(CLUSTER_REMOVEINSTANCE_RETURNS)
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL)
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL1)
 *
 * $(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY1)
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL3)
 * $(CLUSTER_REMOVEINSTANCE_DETAIL4)
 * $(CLUSTER_REMOVEINSTANCE_DETAIL5)
 * $(CLUSTER_REMOVEINSTANCE_DETAIL6)
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL7)
 *
 * $(CLUSTER_REMOVEINSTANCE_DETAIL8)
 */
#if DOXYGEN_JS
Undefined Cluster::removeInstance(InstanceDef instance, Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::remove_instance(InstanceDef instance, dict options) {}
#endif

shcore::Value Cluster::remove_instance(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  // Check arguments count.
  // NOTE: check for arguments need to be performed here for the correct
  // context "Cluster.removeInstance" to be used in the error message
  // (not ReplicaSet.removeInstance).
  args.ensure_count(1, 2, get_function_name("removeInstance").c_str());

  // Throw an error if the cluster has already been dissolved
  assert_valid("removeInstance");

  // Remove the Instance from the Default ReplicaSet
  try {
    check_preconditions("removeInstance");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    ret_val = _default_replica_set->remove_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeInstance"));

  return ret_val;
}

#if 0  // Hidden for now
/**
* Returns the ReplicaSet of the given name.
* \sa ReplicaSet
* \param name the name of the ReplicaSet to look for.
* \return the ReplicaSet object matching the name.
*
* Verifies if the requested Collection exist on the metadata schema, if exists, returns the corresponding ReplicaSet object.
*/
#if DOXYGEN_JS
ReplicaSet Cluster::getReplicaSet(String name) {}
#elif DOXYGEN_PY
ReplicaSet Cluster::get_replica_set(str name) {}
#endif
#endif
shcore::Value Cluster::get_replicaset(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("getReplicaSet");

  shcore::Value ret_val;

  if (args.size() == 0) {
    ret_val = shcore::Value(
        std::dynamic_pointer_cast<shcore::Object_bridge>(_default_replica_set));
  } else {
    args.ensure_count(1, get_function_name("getReplicaSet").c_str());
    std::string name = args.string_at(0);

    if (name == "default") {
      ret_val = shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(
          _default_replica_set));
    } else {
      /*
       * Retrieve the ReplicaSet from the ReplicaSets array
       */
      // if (found)

      // else
      //  throw shcore::Exception::runtime_error("The ReplicaSet " + _name + "."
      //  + name + " does not exist");
    }
  }

  return ret_val;
}

void Cluster::set_default_replicaset(const std::string &name,
                                     const std::string &topology_type,
                                     const std::string &group_name) {
  _default_replica_set = std::make_shared<ReplicaSet>(
      name, topology_type, group_name, _metadata_storage);

  _default_replica_set->set_cluster(shared_from_this());
}

std::shared_ptr<ReplicaSet> Cluster::create_default_replicaset(
    const std::string &name, bool multi_primary, const std::string &group_name,
    bool is_adopted) {
  std::string topology_type = ReplicaSet::kTopologySinglePrimary;
  if (multi_primary) {
    topology_type = ReplicaSet::kTopologyMultiPrimary;
  }
  _default_replica_set = std::make_shared<ReplicaSet>(
      name, topology_type, group_name, _metadata_storage);

  _default_replica_set->set_cluster(shared_from_this());

  // Update the Cluster table with the Default ReplicaSet on the Metadata
  _metadata_storage->insert_replica_set(_default_replica_set, true, is_adopted);

  return _default_replica_set;
}

REGISTER_HELP_FUNCTION(describe, Cluster);
REGISTER_HELP(CLUSTER_DESCRIBE_BRIEF, "Describe the structure of the cluster.");
REGISTER_HELP(CLUSTER_DESCRIBE_THROWS,
              "MetadataError in the following scenarios:");
REGISTER_HELP(CLUSTER_DESCRIBE_THROWS1, "@li If the Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_DESCRIBE_THROWS2,
              "@li If the Metadata update operation failed.");
REGISTER_HELP(
    CLUSTER_DESCRIBE_RETURNS,
    "@returns A JSON object describing the structure of the cluster.");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL,
              "This function describes the structure of the cluster including "
              "all its information, ReplicaSets and Instances.");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL1,
              "The returned JSON object contains the following attributes:");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL2, "@li clusterName: the cluster name");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL3,
              "@li defaultReplicaSet: the default ReplicaSet object");
REGISTER_HELP(
    CLUSTER_DESCRIBE_DETAIL4,
    "The defaultReplicaSet JSON object contains the following attributes:");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL5, "@li name: the ReplicaSet name");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL6,
              "@li topology: a list of dictionaries describing each instance "
              "belonging to the ReplicaSet.");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL7,
              "Each instance dictionary contains the following attributes:");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL8,
              "@li address: the instance address in the form of host:port");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL9,
              "@li label: the instance name identifier");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL10, "@li role: the instance role");

/**
 * $(CLUSTER_DESCRIBE_BRIEF)
 *
 * $(CLUSTER_DESCRIBE_THROWS)
 * $(CLUSTER_DESCRIBE_THROWS1)
 * $(CLUSTER_DESCRIBE_THROWS2)
 *
 * $(CLUSTER_DESCRIBE_RETURNS)
 *
 * $(CLUSTER_DESCRIBE_DETAIL)
 *
 * $(CLUSTER_DESCRIBE_DETAIL1)
 * $(CLUSTER_DESCRIBE_DETAIL2)
 * $(CLUSTER_DESCRIBE_DETAIL3)
 *
 * $(CLUSTER_DESCRIBE_DETAIL4)
 * $(CLUSTER_DESCRIBE_DETAIL5)
 * $(CLUSTER_DESCRIBE_DETAIL6)
 *
 * $(CLUSTER_DESCRIBE_DETAIL7)
 * $(CLUSTER_DESCRIBE_DETAIL8)
 * $(CLUSTER_DESCRIBE_DETAIL9)
 * $(CLUSTER_DESCRIBE_DETAIL10)
 */
#if DOXYGEN_JS
String Cluster::describe() {}
#elif DOXYGEN_PY
str Cluster::describe() {}
#endif

shcore::Value Cluster::describe(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("describe");

  args.ensure_count(0, get_function_name("describe").c_str());

  shcore::Value ret_val;
  try {
    auto state = check_preconditions("describe");

    bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                    state.source_state != ManagedInstance::OnlineRO);

    if (!_metadata_storage->cluster_exists(_name))
      throw shcore::Exception::argument_error("The cluster '" + _name +
                                              "' no longer exists.");

    ret_val = shcore::Value::new_map();

    auto description = ret_val.as_map();

    (*description)["clusterName"] = shcore::Value(_name);

    if (!_default_replica_set)
      (*description)["defaultReplicaSet"] = shcore::Value::Null();
    else
      (*description)["defaultReplicaSet"] =
          _default_replica_set->get_description();

    if (warning) {
      std::string warning =
          "The instance description may be outdated since was generated from "
          "an instance in ";
      warning.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*description)["warning"] = shcore::Value(warning);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("describe"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(status, Cluster);
REGISTER_HELP(CLUSTER_STATUS_BRIEF, "Describe the status of the cluster.");
REGISTER_HELP(CLUSTER_STATUS_PARAM,
              "@param options Optional dictionary with options.");

REGISTER_HELP(CLUSTER_STATUS_THROWS,
              "MetadataError in the following scenarios:");
REGISTER_HELP(CLUSTER_STATUS_THROWS1, "@li If the Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_STATUS_THROWS2,
              "@li If the Metadata update operation failed.");

REGISTER_HELP(CLUSTER_STATUS_RETURNS,
              "@returns A JSON object describing the status of the cluster.");

REGISTER_HELP(CLUSTER_STATUS_DETAIL,
              "This function describes the status of the cluster including its "
              "ReplicaSets and Instances. The following options may be given "
              "to control the amount of information gathered and returned.");
REGISTER_HELP(
    CLUSTER_STATUS_DETAIL1,
    "@li extended: if true, includes information about transactions processed "
    "by connection and applier, as well as groupName and memberId values.");
REGISTER_HELP(
    CLUSTER_STATUS_DETAIL2,
    "@li queryMembers: if true, connect to each Instance of the ReplicaSets to "
    "query for more detailed stats about the replication machinery.");

/**
 * $(CLUSTER_STATUS_BRIEF)
 *
 * $(CLUSTER_STATUS_PARAM)
 *
 * $(CLUSTER_STATUS_THROWS)
 * $(CLUSTER_STATUS_THROWS1)
 * $(CLUSTER_STATUS_THROWS2)
 *
 * $(CLUSTER_STATUS_RETURNS)
 *
 * $(CLUSTER_STATUS_DETAIL)
 *
 * $(CLUSTER_STATUS_DETAIL1)
 * $(CLUSTER_STATUS_DETAIL2)
 */
#if DOXYGEN_JS
String Cluster::status(Dictionary options) {}
#elif DOXYGEN_PY
str Cluster::status(dict options) {}
#endif

shcore::Value Cluster::status(const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("status");

  args.ensure_count(0, 1, get_function_name("status").c_str());

  bool query_members = false;
  bool extended = false;
  if (args.size() > 0) {
    shcore::Value::Map_type_ref options = args.map_at(0);

    Unpack_options(options)
        .optional("extended", &extended)
        .optional("queryMembers", &query_members)
        .end();
  }

  shcore::Value ret_val;
  try {
    auto state = check_preconditions("status");

    bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                    state.source_state != ManagedInstance::OnlineRO);

    // Create the Cluster_status command and execute it.
    Cluster_status op_status(*this, extended, query_members);
    // Always execute finish when leaving "try catch".
    auto finally =
        shcore::on_leave_scope([&op_status]() { op_status.finish(); });
    // Prepare the Cluster_status command execution (validations).
    op_status.prepare();
    // Execute Cluster_status operations.
    ret_val = op_status.execute();

    if (warning) {
      std::string warning =
          "The instance status may be inaccurate as it was generated from an "
          "instance in ";
      warning.append(ManagedInstance::describe(
          static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*ret_val.as_map())["warning"] = shcore::Value(warning);
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("status"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(options, Cluster);
REGISTER_HELP(CLUSTER_OPTIONS_BRIEF,
              "Lists the cluster configuration options.");
REGISTER_HELP(CLUSTER_OPTIONS_PARAM,
              "@param options Optional Dictionary with options.");

REGISTER_HELP(CLUSTER_OPTIONS_THROWS,
              "MetadataError in the following scenarios:");
REGISTER_HELP(CLUSTER_OPTIONS_THROWS1, "@li If the Metadata is inaccessible.");

REGISTER_HELP(CLUSTER_OPTIONS_RETURNS,
              "@returns A JSON object describing the configuration options of "
              "the cluster.");

REGISTER_HELP(
    CLUSTER_OPTIONS_DETAIL,
    "This function lists the cluster configuration options for its"
    "ReplicaSets and Instances. The following options may be given to control"
    "the amount of information gathered and returned.");
REGISTER_HELP(CLUSTER_OPTIONS_DETAIL1,
              "@li all: if true, includes information about all "
              "group_replication system variables.");

/**
 * $(CLUSTER_OPTIONS_BRIEF)
 *
 * $(CLUSTER_OPTIONS_PARAM)
 *
 * $(CLUSTER_OPTIONS_THROWS)
 * $(CLUSTER_OPTIONS_THROWS1)
 * $(CLUSTER_OPTIONS_THROWS2)
 *
 * $(CLUSTER_OPTIONS_RETURNS)
 *
 * $(CLUSTER_OPTIONS_DETAIL)
 *
 * $(CLUSTER_OPTIONS_DETAIL1)
 */
#if DOXYGEN_JS
String Cluster::options(Dictionary options) {}
#elif DOXYGEN_PY
str Cluster::options(dict options) {}
#endif

shcore::Value Cluster::options(const shcore::Dictionary_t &options) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("options");
  auto state = check_preconditions("options");

  bool all = false;
  // Retrieves optional options
  Unpack_options(options).optional("all", &all).end();

  // Create the Cluster_options command and execute it.
  Cluster_options op_option(*this, all);
  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope([&op_option]() { op_option.finish(); });
  // Prepare the Cluster_options command execution (validations).
  op_option.prepare();
  // Execute Cluster_options operations.
  return op_option.execute();
}

REGISTER_HELP_FUNCTION(dissolve, Cluster);
REGISTER_HELP(CLUSTER_DISSOLVE_BRIEF, "Dissolves the cluster.");

REGISTER_HELP(CLUSTER_DISSOLVE_THROWS,
              "MetadataError in the following scenarios:");
REGISTER_HELP(CLUSTER_DISSOLVE_THROWS1, "@li If the Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_DISSOLVE_THROWS2,
              "@li If the Metadata update operation failed.");

REGISTER_HELP(CLUSTER_DISSOLVE_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_DISSOLVE_PARAM,
              "@param options Optional parameter to "
              "specify if it should deactivate "
              "replication and unregister the "
              "ReplicaSets from the cluster.");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL,
              "This function disables replication on "
              "the ReplicaSets, unregisters them and "
              "the the cluster from the metadata.");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL1, "It keeps all the user's data intact.");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL2,
              "The options dictionary may contain the following attributes:");
REGISTER_HELP(
    CLUSTER_DISSOLVE_DETAIL3,
    "@li force: boolean value used to confirm that the dissolve operation must "
    "be executed, even if some members of the cluster cannot be reached or the "
    "timeout was reached when waiting for members to catch up with replication "
    "changes. By default, set to false.");
REGISTER_HELP(
    CLUSTER_DISSOLVE_DETAIL4,
    "@li interactive: boolean value used to disable/enable the wizards in the "
    "command execution, i.e. prompts and confirmations will be provided or "
    "not according to the value set. The default value is equal to MySQL Shell "
    "wizard mode.");
REGISTER_HELP(
    CLUSTER_DISSOLVE_DETAIL5,
    "The force option (set to true) must only be used to dissolve a cluster "
    "with instances that are permanently not available (no longer reachable) "
    "or never to be reused again in a cluster. This allows to dissolve a "
    "cluster and remove it from the metadata, including instances than can no "
    "longer be recovered. Otherwise, the instances must be brought back ONLINE "
    "and the cluster dissolved without the force option to avoid errors trying "
    "to reuse the instances and add them back to a cluster.");

/**
 * $(CLUSTER_DISSOLVE_BRIEF)
 *
 * $(CLUSTER_DISSOLVE_THROWS)
 * $(CLUSTER_DISSOLVE_THROWS1)
 * $(CLUSTER_DISSOLVE_THROWS2)
 *
 * $(CLUSTER_DISSOLVE_RETURNS)
 *
 * $(CLUSTER_DISSOLVE_PARAM)
 *
 * $(CLUSTER_DISSOLVE_DETAIL)
 * $(CLUSTER_DISSOLVE_DETAIL1)
 * $(CLUSTER_DISSOLVE_DETAIL2)
 * $(CLUSTER_DISSOLVE_DETAIL3)
 * $(CLUSTER_DISSOLVE_DETAIL4)
 *
 * $(CLUSTER_DISSOLVE_DETAIL5)
 */
#if DOXYGEN_JS
Undefined Cluster::dissolve(Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::dissolve(dict options) {}
#endif

shcore::Value Cluster::dissolve(const shcore::Argument_list &args) {
  shcore::Value ret_val;

  // Check arguments count.
  // NOTE: check for arguments need to be performed here for the correct
  // context "Cluster.removeInstance" to be used in the error message
  // (not ReplicaSet.removeInstance).
  args.ensure_count(0, 1, get_function_name("dissolve").c_str());

  // Throw an error if the cluster has already been dissolved
  assert_valid("dissolve");

  // Dissolve the default replicaset.
  try {
    // We need to check if the group has quorum and if not we must abort the
    // operation otherwise GR blocks the writes to preserve the consistency
    // of the group and we end up with a hang.
    // This check is done at check_preconditions()
    check_preconditions("dissolve");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    ret_val = _default_replica_set->dissolve(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dissolve"))

  return ret_val;
}

REGISTER_HELP_FUNCTION(rescan, Cluster);
REGISTER_HELP(CLUSTER_RESCAN_BRIEF, "Rescans the cluster.");

REGISTER_HELP(CLUSTER_RESCAN_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(
    CLUSTER_RESCAN_THROWS1,
    "@li If the value for `addInstances` or `removeInstance` is empty.");
REGISTER_HELP(
    CLUSTER_RESCAN_THROWS2,
    "@li If the value for `addInstances` or `removeInstance` is invalid.");
REGISTER_HELP(CLUSTER_RESCAN_THROWS3,
              "MetadataError in the following scenarios:");
REGISTER_HELP(CLUSTER_RESCAN_THROWS4, "@li If the Metadata is inaccessible.");
REGISTER_HELP(CLUSTER_RESCAN_THROWS5,
              "@li If the Metadata update operation failed.");
REGISTER_HELP(CLUSTER_RESCAN_THROWS6, "LogicError in the following scenarios:");
REGISTER_HELP(CLUSTER_RESCAN_THROWS7, "@li If the cluster does not exist.");
REGISTER_HELP(CLUSTER_RESCAN_THROWS8,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_RESCAN_THROWS9,
              "@li If all the "
              "ReplicaSet instances of any ReplicaSet "
              "are offline.");
REGISTER_HELP(CLUSTER_RESCAN_THROWS10,
              "@li If an instance specified for `addInstances` is not an "
              "active member of the replication group.");
REGISTER_HELP(CLUSTER_RESCAN_THROWS11,
              "@li If an instance specified for `removeInstances` is an active "
              "member of the replication group.");

REGISTER_HELP(
    CLUSTER_RESCAN_PARAM,
    "@param options Optional Dictionary with options for the operation.");

REGISTER_HELP(CLUSTER_RESCAN_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_RESCAN_DETAIL,
              "This function rescans the cluster for new and obsolete Group "
              "Replication members/instances, as well as changes in the used "
              "topology mode (i.e., single-primary and multi-primary).");
REGISTER_HELP(CLUSTER_RESCAN_DETAIL1,
              "The options dictionary may contain the following attributes:");
REGISTER_HELP(CLUSTER_RESCAN_DETAIL2,
              "@li addInstances: List with the connection data of the new "
              "active instances to add to the metadata, or \"auto\" to "
              "automatically add missing instances to the metadata.");
REGISTER_HELP(CLUSTER_RESCAN_DETAIL3,
              "@li interactive: boolean value used to disable/enable the "
              "wizards in the command execution, i.e. prompts and "
              "confirmations will be provided or not according to the value "
              "set. The default value is equal to MySQL Shell wizard mode.");
REGISTER_HELP(CLUSTER_RESCAN_DETAIL4,
              "@li removeInstances: List with the connection data of the "
              "obsolete instances to remove from the metadata, or \"auto\" to "
              "automatically remove obsolete instances from the metadata.");
REGISTER_HELP(CLUSTER_RESCAN_DETAIL5,
              "@li updateTopologyMode: boolean value used to indicate if the "
              "topology mode (single-primary or multi-primary) in the metadata "
              "should be updated (true) or not (false) to match the one being "
              "used by the cluster. By default, the metadata is not updated "
              "(false).");
REGISTER_HELP(CLUSTER_RESCAN_DETAIL6,
              "The value for addInstances and removeInstances is used to "
              "specify which instances to add or remove from the metadata, "
              "respectively. Both options accept list connection data. In "
              "addition, the \"auto\" value can be used for both options in "
              "order to automatically add or remove the instances in the "
              "metadata, without having to explicitly specify them.");

/**
 * $(CLUSTER_RESCAN_BRIEF)
 *
 * $(CLUSTER_RESCAN_THROWS)
 * $(CLUSTER_RESCAN_THROWS1)
 * $(CLUSTER_RESCAN_THROWS2)
 * $(CLUSTER_RESCAN_THROWS3)
 * $(CLUSTER_RESCAN_THROWS4)
 * $(CLUSTER_RESCAN_THROWS5)
 * $(CLUSTER_RESCAN_THROWS6)
 * $(CLUSTER_RESCAN_THROWS7)
 * $(CLUSTER_RESCAN_THROWS8)
 * $(CLUSTER_RESCAN_THROWS9)
 * $(CLUSTER_RESCAN_THROWS10)
 * $(CLUSTER_RESCAN_THROWS11)
 *
 * $(CLUSTER_RESCAN_PARAM)
 *
 * $(CLUSTER_RESCAN_RETURNS)
 *
 * $(CLUSTER_RESCAN_DETAIL)
 *
 * $(CLUSTER_RESCAN_DETAIL1)
 *
 * $(CLUSTER_RESCAN_DETAIL2)
 * $(CLUSTER_RESCAN_DETAIL3)
 * $(CLUSTER_RESCAN_DETAIL4)
 * $(CLUSTER_RESCAN_DETAIL5)
 *
 * $(CLUSTER_RESCAN_DETAIL6)
 */
#if DOXYGEN_JS
Undefined Cluster::rescan(Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::rescan(dict options) {}
#endif

void Cluster::rescan(const shcore::Dictionary_t &options) {
  assert_valid("rescan");

  check_preconditions("rescan");

  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  _default_replica_set->rescan(options);
}

REGISTER_HELP_FUNCTION(disconnect, Cluster);
REGISTER_HELP(CLUSTER_DISCONNECT_BRIEF,
              "Disconnects all internal sessions used by the cluster object.");

REGISTER_HELP(CLUSTER_DISCONNECT_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_DISCONNECT_DETAIL,
              "Disconnects the internal MySQL sessions used by the cluster "
              "to query for metadata and replication information.");

/**
 * $(CLUSTER_DISCONNECT_BRIEF)
 *
 * $(CLUSTER_DISCONNECT_RETURNS)
 *
 * $(CLUSTER_DISCONNECT_DETAIL)
 */
#if DOXYGEN_JS
Undefined Cluster::disconnect() {}
#elif DOXYGEN_PY
None Cluster::disconnect() {}
#endif

shcore::Value Cluster::disconnect(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("disconnect").c_str());

  try {
    if (_group_session) {
      // no preconditions check needed for just disconnecting everything
      _group_session->close();
      _group_session.reset();
    }
    if (_metadata_storage->get_session()) {
      _metadata_storage->get_session()->close();
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("disconnect"));

  return shcore::Value();
}

REGISTER_HELP_FUNCTION(forceQuorumUsingPartitionOf, Cluster);
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_BRIEF,
              "Restores the cluster from quorum loss.");
REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_PARAM,
    "@param instance An instance definition to derive the forced group from.");
REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_PARAM1,
    "@param password Optional string with the password for the connection.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS1,
              "@li If the instance parameter is empty.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS2,
              "@li If the instance definition cannot be used "
              "for Group Replication.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS3,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS4,
              "@li If the instance does not exist on the Metadata.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS5,
              "@li If the instance is not on the ONLINE state.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS6,
              "@li If the instance does is not an active "
              "member of a replication group.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS7,
              "@li If there are no ONLINE instances visible "
              "from the given one.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS8,
              "LogicError in the following scenarios:");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS9,
              "@li If the cluster does not exist.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL,
              "This function restores the cluster's default replicaset back "
              "into operational status from a loss of quorum scenario. "
              "Such a scenario can occur if a group is partitioned or more "
              "crashes than tolerable occur.");

REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL1,
    "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL2,
              "${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL3,
              "The options dictionary may contain the following options:");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL4,
              "@li mycnfPath: The path of the MySQL configuration file for the "
              "instance.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL5,
              "@li password: The password to get connected to the instance.");
REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL6,
    "@li clusterAdmin: The name of the InnoDB cluster administrator user.");
REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL7,
              "@li clusterAdminPassword: The password for the InnoDB cluster "
              "administrator account.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL8,
              "The password may be contained on the instance definition, "
              "however, it can be overwritten "
              "if it is specified on the options.");

REGISTER_HELP(
    CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL9,
    "Note that this operation is DANGEROUS as it can create a "
    "split-brain if incorrectly used and should be considered a last "
    "resort. Make absolutely sure that there are no partitions of this group "
    "that are still operating somewhere in the network, but not "
    "accessible from your location.");

REGISTER_HELP(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL10,
              "When this function is used, all the members that are ONLINE "
              "from the point of view "
              "of the given instance definition will be added to the group.");

/**
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_BRIEF)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_PARAM)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_PARAM1)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS1)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS2)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS3)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS4)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS5)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS6)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS7)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS8)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_THROWS9)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_RETURNS)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL1)
 *
 * $(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY1)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL3)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL4)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL5)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL6)
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL7)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL8)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL9)
 *
 * $(CLUSTER_FORCEQUORUMUSINGPARTITIONOF_DETAIL10)
 */
#if DOXYGEN_JS
Undefined Cluster::forceQuorumUsingPartitionOf(InstanceDef instance,
                                               String password) {}
#elif DOXYGEN_PY
None Cluster::force_quorum_using_partition_of(InstanceDef instance,
                                              str password) {}
#endif

shcore::Value Cluster::force_quorum_using_partition_of(
    const shcore::Argument_list &args) {
  // Throw an error if the cluster has already been dissolved
  assert_valid("forceQuorumUsingPartitionOf");

  args.ensure_count(1, 2,
                    get_function_name("forceQuorumUsingPartitionOf").c_str());

  shcore::Value ret_val;
  try {
    check_preconditions("forceQuorumUsingPartitionOf");

    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    ret_val = _default_replica_set->force_quorum_using_partition_of(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("forceQuorumUsingPartitionOf"));

  return ret_val;
}

void Cluster::set_attribute(const std::string &attribute,
                            const shcore::Value &value) {
  if (!_attributes) _attributes.reset(new shcore::Value::Map_type());

  (*_attributes)[attribute] = value;
}

REGISTER_HELP_FUNCTION(checkInstanceState, Cluster);
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_BRIEF,
              "Verifies the instance gtid state in relation to the cluster.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_PARAM,
              "@param instance An instance definition.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS1,
              "@li If the 'instance' parameter is empty.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS2,
              "@li If the 'instance' parameter is invalid.");
REGISTER_HELP(
    CLUSTER_CHECKINSTANCESTATE_THROWS3,
    "@li If the 'instance' definition is a connection dictionary but empty.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS4,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS5,
              "@li If the 'instance' is unreachable/offline.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS6,
              "@li If the 'instance' is a cluster member.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS7,
              "@li If the 'instance' belongs to a Group Replication group that "
              "is not managed as an InnoDB cluster.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS8,
              "@li If the 'instance' is a standalone instance but is part of a "
              "different InnoDB Cluster.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_THROWS9,
              "@li If the 'instance' has an unknown state.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_RETURNS,
              "@returns resultset A JSON object with the status.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL,
              "Analyzes the instance executed GTIDs with the executed/purged "
              "GTIDs on the cluster "
              "to determine if the instance is valid for the cluster.");

REGISTER_HELP(
    CLUSTER_CHECKINSTANCESTATE_DETAIL1,
    "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL2,
              "${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL3,
              "The returned JSON object contains the following attributes:");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL4,
              "@li state: the state of the instance");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL5,
              "@li reason: the reason for the state reported");

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL6,
              "The state of the instance can be one of the following:");
REGISTER_HELP(
    CLUSTER_CHECKINSTANCESTATE_DETAIL7,
    "@li ok: if the instance transaction state is valid for the cluster");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL8,
              "@li error: if the instance "
              "transaction state is not "
              "valid for the cluster");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL9,
              "The reason for the state reported can be one of the following:");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL10,
              "@li new: if the instance doesn’t have any transactions");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL11,
              "@li recoverable:  if the instance executed GTIDs are not "
              "conflicting with the executed GTIDs of the cluster instances");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL12,
              "@li diverged: if the instance executed GTIDs diverged with the "
              "executed GTIDs of the cluster instances");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL13,
              "@li lost_transactions: if the instance has more executed GTIDs "
              "than the executed GTIDs of the cluster instances");

/**
 * $(CLUSTER_CHECKINSTANCESTATE_BRIEF)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_PARAM)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS)
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS1)
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS2)
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS3)
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS4)
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS5)
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS6)
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS7)
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS8)
 * $(CLUSTER_CHECKINSTANCESTATE_THROWS9)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_RETURNS)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL1)
 *
 * $(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY1)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL3)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL4)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL5)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL6)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL7)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL8)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL9)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL10)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL11)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL12)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL13)
 */
#if DOXYGEN_JS
Undefined Cluster::checkInstanceState(InstanceDef instance) {}
#elif DOXYGEN_PY
None Cluster::check_instance_state(InstanceDef instance) {}
#endif

shcore::Value Cluster::check_instance_state(
    const Connection_options &instance_def) {
  assert_valid("checkInstanceState");
  check_preconditions("checkInstanceState");

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  return _default_replica_set->check_instance_state(instance_def);
}

shcore::Value Cluster::check_instance_state(const std::string &instance_def) {
  return check_instance_state(get_connection_options(instance_def));
}

shcore::Value Cluster::check_instance_state(
    const shcore::Dictionary_t &instance_def) {
  return check_instance_state(get_connection_options(instance_def));
}

REGISTER_HELP_FUNCTION(switchToSinglePrimaryMode, Cluster);
REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_BRIEF,
              "Switches the cluster to single-primary mode.");

REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS1,
              "@li If the instance parameter is empty.");
REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS2,
              "@li If the instance definition is invalid.");

REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS3,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS4,
              "@li If 'instance' does not refer to a cluster member.");
REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS5,
              "@li If any of the cluster members has a version < 8.0.13.");
REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS6,
              "@li If the cluster has no visible quorum.");
REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS7,
              "@li If any of the cluster members is not ONLINE.");

REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_PARAM,
              "@param instance Optional An instance definition.");

REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL,
              "This function changes a cluster running in multi-primary mode "
              "to single-primary mode.");

REGISTER_HELP(
    CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL1,
    "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL2,
              "${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}");

REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL3,
              "The instance definition is optional and is the identifier of "
              "the cluster member that shall become the new primary.");

REGISTER_HELP(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL4,
              "If the instance definition is not provided, the new primary "
              "will be the instance with the highest member weight (and the "
              "lowest UUID in case of a tie on member weight).");

/**
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_BRIEF)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS1)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS2)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS3)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS4)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS5)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS6)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS7)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_RETURNS)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_PARAM)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL1)
 *
 * $(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY1)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL3)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL4)
 */
#if DOXYGEN_JS
Undefined Cluster::switchToSinglePrimaryMode(InstanceDef instance) {}
#elif DOXYGEN_PY
None Cluster::switch_to_single_primary_mode(InstanceDef instance) {}
#endif

void Cluster::switch_to_single_primary_mode(
    const Connection_options &instance_def) {
  assert_valid("switchToSinglePrimaryMode");
  check_preconditions("switchToSinglePrimaryMode");

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  // Switch to single-primary mode
  _default_replica_set->switch_to_single_primary_mode(instance_def);
}

REGISTER_HELP_FUNCTION(switchToMultiPrimaryMode, Cluster);
REGISTER_HELP(CLUSTER_SWITCHTOMULTIPRIMARYMODE_BRIEF,
              "Switches the cluster to multi-primary mode.");

REGISTER_HELP(CLUSTER_SWITCHTOMULTIPRIMARYMODE_THROWS,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_SWITCHTOMULTIPRIMARYMODE_THROWS1,
              "@li If any of the cluster members has a version < 8.0.13.");
REGISTER_HELP(CLUSTER_SWITCHTOMULTIPRIMARYMODE_THROWS2,
              "@li If the cluster has no visible quorum.");
REGISTER_HELP(CLUSTER_SWITCHTOMULTIPRIMARYMODE_THROWS3,
              "@li If any of the cluster members is not ONLINE.");

REGISTER_HELP(CLUSTER_SWITCHTOMULTIPRIMARYMODE_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_SWITCHTOMULTIPRIMARYMODE_DETAIL,
              "This function changes a cluster running in single-primary mode "
              "to multi-primary mode.");

/**
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE_BRIEF)
 *
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE_THROWS)
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE_THROWS1)
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE_THROWS2)
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE_THROWS3)
 *
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE_RETURNS)
 *
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE_PARAM)
 *
 * $(CLUSTER_SWITCHTOMULTIPRIMARYMODE_DETAIL)
 */
#if DOXYGEN_JS
Undefined Cluster::switchToMultiPrimaryMode() {}
#elif DOXYGEN_PY
None Cluster::switch_to_multi_primary_mode() {}
#endif

void Cluster::switch_to_multi_primary_mode(void) {
  assert_valid("switchToMultiPrimaryMode");
  check_preconditions("switchToMultiPrimaryMode");

  // Switch to single-primary mode

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  _default_replica_set->switch_to_multi_primary_mode();
}

REGISTER_HELP_FUNCTION(setPrimaryInstance, Cluster);
REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_BRIEF,
              "Elects a specific cluster member as the new primary.");

REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_THROWS1,
              "@li If the instance parameter is empty.");
REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_THROWS2,
              "@li If the instance definition is invalid.");

REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_THROWS3,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_THROWS4,
              "@li If the cluster is in multi-primary mode.");
REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_THROWS5,
              "@li If 'instance' does not refer to a cluster member.");
REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_THROWS6,
              "@li If any of the cluster members has a version < 8.0.13.");
REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_THROWS7,
              "@li If the cluster has no visible quorum.");
REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_THROWS8,
              "@li If any of the cluster members is not ONLINE.");

REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_PARAM,
              "@param instance An instance definition.");

REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_DETAIL,
              "This function forces the election of a new primary, overriding "
              "any election process.");

REGISTER_HELP(
    CLUSTER_SETPRIMARYINSTANCE_DETAIL1,
    "The instance definition is the connection data for the instance.");

REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_DETAIL2,
              "${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}");

REGISTER_HELP(CLUSTER_SETPRIMARYINSTANCE_DETAIL3,
              "The instance definition is mandatory and is the identifier of "
              "the cluster member that shall become the new primary.");

/**
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_BRIEF)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS1)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS2)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS3)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS4)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS5)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS6)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS7)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_THROWS8)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_RETURNS)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_PARAM)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL)
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL1)
 *
 * $(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY1)
 *
 * $(CLUSTER_SWITCHTOSINGLEPRIMARYMODE_DETAIL3)
 */
#if DOXYGEN_JS
Undefined Cluster::setPrimaryInstance(InstanceDef instance) {}
#elif DOXYGEN_PY
None Cluster::set_primary_instance(InstanceDef instance) {}
#endif

void Cluster::set_primary_instance(const Connection_options &instance_def) {
  assert_valid("setPrimaryInstance");
  check_preconditions("setPrimaryInstance");

  // Set primary instance

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  _default_replica_set->set_primary_instance(instance_def);
}

void Cluster::set_primary_instance(const std::string &instance_def) {
  mysqlshdk::db::Connection_options target_instance;

  target_instance = get_connection_options(instance_def);

  set_primary_instance(target_instance);
}

void Cluster::set_primary_instance(const shcore::Dictionary_t &instance_def) {
  mysqlshdk::db::Connection_options target_instance;

  target_instance = get_connection_options(instance_def);

  set_primary_instance(target_instance);
}

REGISTER_HELP_FUNCTION(setOption, Cluster);
REGISTER_HELP(
    CLUSTER_SETOPTION_BRIEF,
    "Changes the value of a configuration option for the whole cluster.");

REGISTER_HELP(CLUSTER_SETOPTION_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(CLUSTER_SETOPTION_THROWS1,
              "@li If the 'option' parameter is empty.");
REGISTER_HELP(CLUSTER_SETOPTION_THROWS2,
              "@li If the 'value' parameter is empty.");
REGISTER_HELP(CLUSTER_SETOPTION_THROWS3,
              "@li If the 'option' parameter is invalid.");
REGISTER_HELP(CLUSTER_SETOPTION_THROWS4,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_SETOPTION_THROWS5,
              "@li If any of the cluster members do not support the "
              "configuration option passed in 'option'.");
REGISTER_HELP(
    CLUSTER_SETOPTION_THROWS6,
    "@li If the value passed in 'option' is not valid for Group Replication.");
REGISTER_HELP(CLUSTER_SETOPTION_THROWS7,
              "@li If the cluster has no visible quorum.");
REGISTER_HELP(CLUSTER_SETOPTION_THROWS8,
              "@li If any of the cluster members is not ONLINE.");

REGISTER_HELP(CLUSTER_SETOPTION_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_SETOPTION_PARAM,
              "@param option The configuration option to be changed.");
REGISTER_HELP(
    CLUSTER_SETOPTION_PARAM1,
    "@param value The value that the configuration option shall get.");

REGISTER_HELP(CLUSTER_SETOPTION_DETAIL,
              "This function changes an InnoDB Cluster configuration option in "
              "all members of the cluster.");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL1,
              "The 'option' parameter is the name of the configuration option "
              "to be changed.");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL2,
              "The value parameter is the value that the configuration option "
              "shall get.");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL3,
              "The accepted values for the configuration option are:");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL4,
              "@li clusterName: string value to define the cluster name.");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL5, "${CLUSTER_OPT_EXIT_STATE_ACTION}");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL6, "${CLUSTER_OPT_MEMBER_WEIGHT}");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL7, "${CLUSTER_OPT_FAILOVER_CONSISTENCY}");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL8, "${CLUSTER_OPT_EXPEL_TIMEOUT}");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL9,
              "The value for the configuration option is used to set the Group "
              "Replication system variable that corresponds to it.");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL10,
              "${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL11,
              "${CLUSTER_OPT_FAILOVER_CONSISTENCY_DETAIL}");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL12,
              "${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL13,
              "${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL14,
              "${CLUSTER_OPT_FAILOVER_CONSISTENCY_EXTRA}");
REGISTER_HELP(CLUSTER_SETOPTION_DETAIL15, "${CLUSTER_OPT_EXPELTIMEOUT_EXTRA}");

/**
 * $(CLUSTER_SETOPTION_BRIEF)
 *
 * $(CLUSTER_SETOPTION_THROWS)
 * $(CLUSTER_SETOPTION_THROWS1)
 * $(CLUSTER_SETOPTION_THROWS2)
 * $(CLUSTER_SETOPTION_THROWS3)
 * $(CLUSTER_SETOPTION_THROWS4)
 * $(CLUSTER_SETOPTION_THROWS5)
 * $(CLUSTER_SETOPTION_THROWS6)
 * $(CLUSTER_SETOPTION_THROWS7)
 * $(CLUSTER_SETOPTION_THROWS8)
 *
 * $(CLUSTER_SETOPTION_RETURNS)
 *
 * $(CLUSTER_SETOPTION_PARAM)
 * $(CLUSTER_SETOPTION_PARAM1)
 *
 * $(CLUSTER_SETOPTION_DETAIL)
 * $(CLUSTER_SETOPTION_DETAIL1)
 * $(CLUSTER_SETOPTION_DETAIL2)
 * $(CLUSTER_SETOPTION_DETAIL3)
 * $(CLUSTER_SETOPTION_DETAIL4)
 * $(CLUSTER_OPT_EXIT_STATE_ACTION)
 * $(CLUSTER_OPT_MEMBER_WEIGHT)
 * $(CLUSTER_SETOPTION_DETAIL6)
 * $(CLUSTER_SETOPTION_DETAIL7)
 * $(CLUSTER_SETOPTION_DETAIL8)
 * $(CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL)
 * $(CLUSTER_OPT_FAILOVER_CONSISTENCY_DETAIL)
 * $(CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA)
 * $(CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA)
 * $(CLUSTER_OPT_FAILOVER_CONSISTENCY_EXTRA)
 * $(CLUSTER_OPT_EXPELTIMEOUT_EXTRA)
 */
#if DOXYGEN_JS
Undefined Cluster::setOption(String option, String value) {}
#elif DOXYGEN_PY
None Cluster::set_option(str option, str value) {}
#endif

void Cluster::set_option(const std::string &option,
                         const shcore::Value &value) {
  assert_valid("setOption");
  check_preconditions("setOption");

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  // Set Cluster configuration option

  // Create the Cluster_set_option object and execute it.
  std::unique_ptr<Cluster_set_option> op_cluster_set_option;

  // Validation types due to a limitation on the expose() framework.
  // Currently, it's not possible to do overloading of functions that overload
  // an argument of type string/int since the type int is convertible to
  // string, thus overloading becomes ambiguous. As soon as that limitation is
  // gone, this type checking shall go away too.
  if (value.type == shcore::String) {
    std::string value_str = value.as_string();
    op_cluster_set_option =
        shcore::make_unique<Cluster_set_option>(this, option, value_str);
  } else if (value.type == shcore::Integer || value.type == shcore::UInteger) {
    int64_t value_int = value.as_int();
    op_cluster_set_option =
        shcore::make_unique<Cluster_set_option>(this, option, value_int);
  } else {
    throw shcore::Exception::argument_error(
        "Argument #2 is expected to be a string or an Integer.");
  }

  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_cluster_set_option]() { op_cluster_set_option->finish(); });

  // Prepare the Set_option command execution (validations).
  op_cluster_set_option->prepare();

  // Execute Set_instance_option operations.
  op_cluster_set_option->execute();
}

REGISTER_HELP_FUNCTION(setInstanceOption, Cluster);
REGISTER_HELP(
    CLUSTER_SETINSTANCEOPTION_BRIEF,
    "Changes the value of a configuration option in a Cluster member.");

REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS,
              "ArgumentError in the following scenarios:");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS1,
              "@li If the 'instance' parameter is empty.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS2,
              "@li If the 'instance' parameter is invalid.");
REGISTER_HELP(
    CLUSTER_SETINSTANCEOPTION_THROWS3,
    "@li If the 'instance' definition is a connection dictionary but empty.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS4,
              "@li If the 'option' parameter is empty.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS5,
              "@li If the 'value' parameter is empty.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS6,
              "@li If the 'option' parameter is invalid.");

REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS7,
              "RuntimeError in the following scenarios:");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS8,
              "@li If 'instance' does not refer to a cluster member.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS9,
              "@li If the cluster has no visible quorum.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS10,
              "@li If 'instance' is not ONLINE.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_THROWS11,
              "@li If 'instance' does not support the configuration option "
              "passed in 'option'.");
REGISTER_HELP(
    CLUSTER_SETINSTANCEOPTION_THROWS12,
    "@li If the value passed in 'option' is not valid for Group Replication.");

REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_RETURNS, "@returns Nothing.");

REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_PARAM,
              "@param instance An instance definition.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_PARAM1,
              "@param option The configuration option to be changed.");
REGISTER_HELP(
    CLUSTER_SETINSTANCEOPTION_PARAM2,
    "@param value The value that the configuration option shall get.");

REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL,
              "This function changes an InnoDB Cluster configuration option in "
              "a member of the cluster.");
REGISTER_HELP(
    CLUSTER_SETINSTANCEOPTION_DETAIL1,
    "The instance definition is the connection data for the instance.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL2,
              "${TOPIC_CONNECTION_MORE_INFO_TCP_ONLY}");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL3,
              "The option parameter is the name of the configuration option to "
              "be changed");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL4,
              "The value parameter is the value that the configuration option "
              "shall get.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL5,
              "The accepted values for the configuration option are:");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL6,
              "${CLUSTER_OPT_EXIT_STATE_ACTION}");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL7,
              "${CLUSTER_OPT_MEMBER_WEIGHT}");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL8,
              "@li label a string identifier of the instance.");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL9,
              "${CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL}");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL10,
              "${CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA}");
REGISTER_HELP(CLUSTER_SETINSTANCEOPTION_DETAIL11,
              "${CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA}");

/**
 * $(CLUSTER_SETINSTANCEOPTION_BRIEF)
 *
 * $(CLUSTER_SETINSTANCEOPTION_THROWS)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS1)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS2)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS3)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS4)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS5)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS6)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS7)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS8)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS9)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS10)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS11)
 * $(CLUSTER_SETINSTANCEOPTION_THROWS12)
 *
 * $(CLUSTER_SETINSTANCEOPTION_RETURNS)
 *
 * $(CLUSTER_SETINSTANCEOPTION_PARAM)
 * $(CLUSTER_SETINSTANCEOPTION_PARAM1)
 * $(CLUSTER_SETINSTANCEOPTION_PARAM2)
 *
 * $(CLUSTER_SETINSTANCEOPTION_DETAIL)
 * $(CLUSTER_SETINSTANCEOPTION_DETAIL1)
 *
 * $(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY1)
 *
 * $(CLUSTER_SETINSTANCEOPTION_DETAIL3)
 * $(CLUSTER_SETINSTANCEOPTION_DETAIL4)
 * $(CLUSTER_SETINSTANCEOPTION_DETAIL5)
 * $(CLUSTER_OPT_EXIT_STATE_ACTION)
 * $(CLUSTER_OPT_MEMBER_WEIGHT)
 * $(CLUSTER_SETINSTANCEOPTION_DETAIL8)
 * $(CLUSTER_OPT_EXIT_STATE_ACTION_DETAIL)
 * $(CLUSTER_OPT_MEMBER_WEIGHT_DETAIL_EXTRA)
 * $(CLUSTER_OPT_EXIT_STATE_ACTION_EXTRA)
 */
#if DOXYGEN_JS
Undefined Cluster::setInstanceOption(InstanceDef instance, String option,
                                     String value) {}
#elif DOXYGEN_PY
None Cluster::set_instance_option(InstanceDef instance, str option, str value);
#endif
void Cluster::set_instance_option(const Connection_options &instance_def,
                                  const std::string &option,
                                  const shcore::Value &value) {
  assert_valid("setInstanceOption");
  check_preconditions("setInstanceOption");

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  // Set the option in the Default ReplicaSet
  _default_replica_set->set_instance_option(instance_def, option, value);
}

void Cluster::set_instance_option(const std::string &instance_def,
                                  const std::string &option,
                                  const shcore::Value &value) {
  mysqlshdk::db::Connection_options target_instance;

  target_instance = get_connection_options(instance_def);

  set_instance_option(target_instance, option, value);
}

void Cluster::set_instance_option(const shcore::Dictionary_t &instance_def,
                                  const std::string &option,
                                  const shcore::Value &value) {
  mysqlshdk::db::Connection_options target_instance;

  target_instance = get_connection_options(instance_def);

  set_instance_option(target_instance, option, value);
}

Cluster_check_info Cluster::check_preconditions(
    const std::string &function_name) const {
  return check_function_preconditions("Cluster." + function_name,
                                      _group_session);
}

void Cluster::sync_transactions(
    const mysqlshdk::mysql::IInstance &target_instance) const {
  // Must get the value of the 'gtid_executed' variable with GLOBAL scope to get
  // the GTID of ALL transactions, otherwise only a set of transactions written
  // to the cache in the current session might be returned.
  std::string gtid_set = _group_session->query("SELECT @@GLOBAL.GTID_EXECUTED")
                             ->fetch_one()
                             ->get_string(0);

  bool sync_res = mysqlshdk::mysql::wait_for_gtid_set(
      target_instance, gtid_set,
      current_shell_options()->get().dba_gtid_wait_timeout);
  if (!sync_res) {
    std::string instance_address =
        target_instance.get_connection_options().as_uri(
            mysqlshdk::db::uri::formats::only_transport());
    throw shcore::Exception::runtime_error(
        "Timeout reached waiting for cluster transactions to be applied on "
        "instance '" +
        instance_address + "'");
  }
}

}  // namespace dba
}  // namespace mysqlsh
