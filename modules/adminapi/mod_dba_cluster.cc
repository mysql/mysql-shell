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

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_common.h"
#include "common/uuid/include/uuid_gen.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "my_aes.h"

#include "mod_dba_replicaset.h"
#include "mod_dba_metadata_storage.h"
#include "../mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include "utils/utils_help.h"
#include "mod_dba_sql.h"

using namespace std::placeholders;
using namespace mysqlsh;
using namespace mysqlsh::dba;
using namespace shcore;

// Documentation of the Cluster Class
REGISTER_HELP(CLUSTER_BRIEF, "Represents an instance of MySQL InnoDB Cluster.");
REGISTER_HELP(CLUSTER_DETAIL, "The cluster object is the entrance point to manage the MySQL InnoDB Cluster system.");
REGISTER_HELP(CLUSTER_DETAIL1, "A cluster is a set of MySQLd Instances which holds the user's data.");
REGISTER_HELP(CLUSTER_DETAIL2, "It provides high-availability and scalability for the user's data.");

REGISTER_HELP(CLUSTER_CLOSING, "For more help on a specific function use: cluster.help('<functionName>')");
REGISTER_HELP(CLUSTER_CLOSING1, "e.g. cluster.help('addInstance')");

REGISTER_HELP(CLUSTER_NAME_BRIEF, "Cluster name.");
REGISTER_HELP(CLUSTER_ADMINTYPE_BRIEF, "Cluster Administration type.");

Cluster::Cluster(const std::string &name, std::shared_ptr<MetadataStorage> metadata_storage) :
_name(name), _metadata_storage(metadata_storage) {
  init();
}

Cluster::~Cluster() {}

std::string &Cluster::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const {
  s_out.append("<" + class_name() + ":" + _name + ">");
  return s_out;
}

bool Cluster::operator == (const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Cluster::init() {
  add_property("name", "getName");
  add_property("adminType", "getAdminType");
  add_method("addInstance", std::bind(&Cluster::add_instance, this, _1), "data");
  add_method("rejoinInstance", std::bind(&Cluster::rejoin_instance, this, _1), "data");
  add_method("removeInstance", std::bind(&Cluster::remove_instance, this, _1), "data");
  add_method("describe", std::bind(&Cluster::describe, this, _1), NULL);
  add_method("status", std::bind(&Cluster::status, this, _1), NULL);
  add_varargs_method("dissolve", std::bind(&Cluster::dissolve, this, _1));
  add_varargs_method("checkInstanceState", std::bind(&Cluster::check_instance_state, this, _1));
  add_varargs_method("rescan", std::bind(&Cluster::rescan, this, _1));
}

// Documentation of the getName function
REGISTER_HELP(CLUSTER_GETNAME_BRIEF, "Retrieves the name of the cluster.");
REGISTER_HELP(CLUSTER_GETNAME_RETURN, "@return The name of the cluster.");

/**
* $(CLUSTER_GETNAME_BRIEF)
*
* $(CLUSTER_GETNAME_RETURN)
*
*/
#if DOXYGEN_JS
String Cluster::getName() {}
#elif DOXYGEN_PY
str Cluster::get_name() {}
#endif

shcore::Value Cluster::get_member(const std::string &prop) const {
  shcore::Value ret_val;
  if (prop == "name")
    ret_val = shcore::Value(_name);
  else if (prop == "adminType")
    ret_val = (*_options)[OPT_ADMIN_TYPE];
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);

  return ret_val;
}

// Documentation of the getAdminType function
REGISTER_HELP(CLUSTER_GETADMINTYPE_BRIEF, "Retrieves the Administration type of the cluster.");
REGISTER_HELP(CLUSTER_GETADMINTYPE_RETURN, "@return The Administration type of the cluster.");

/**
* $(CLUSTER_GETADMINTYPE_BRIEF)
*
* $(CLUSTER_GETADMINTYPE_RETURN)
*
*/
#if DOXYGEN_JS
String Cluster::getAdminType() {}
#elif DOXYGEN_PY
str Cluster::get_admin_type() {}
#endif

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
Undefined addSeedInstance(String conn, String root_password, String topology_type) {}
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
shcore::Value Cluster::add_seed_instance(const shcore::Argument_list &args,
    bool multi_master, bool is_adopted) {
  shcore::Value ret_val;

  MetadataStorage::Transaction tx(_metadata_storage);
  std::string default_replication_user = "rpl_user"; // Default for V1.0 is rpl_user
  std::shared_ptr<ReplicaSet> default_rs = get_default_replicaset();

  // Check if we have a Default ReplicaSet, if so it means we already added the Seed Instance
  if (default_rs != NULL) {
    uint64_t rs_id = default_rs->get_id();
    if (!_metadata_storage->is_replicaset_empty(rs_id))
      throw shcore::Exception::logic_error("Default ReplicaSet already initialized. Please use: addInstance() to add more Instances to the ReplicaSet.");
  } else {
    std::string topology_type = ReplicaSet::kTopologyPrimaryMaster;
    if (multi_master) {
      topology_type = ReplicaSet::kTopologyMultiMaster;
    }
    // Create the Default ReplicaSet and assign it to the Cluster's default_replica_set var
    _default_replica_set.reset(new ReplicaSet("default", topology_type,
                                              _metadata_storage));
    _default_replica_set->set_cluster(shared_from_this());

    // If we reached here without errors we can update the Metadata

    // Update the Cluster table with the Default ReplicaSet on the Metadata
    _metadata_storage->insert_replica_set(_default_replica_set, true, is_adopted);
  }
  // Add the Instance to the Default ReplicaSet
  ret_val = _default_replica_set->add_instance(args);

  std::string group_replication_group_name = _metadata_storage->get_replicaset_group_name();
  _metadata_storage->set_replicaset_group_name(_default_replica_set, group_replication_group_name);

  tx.commit();

  return ret_val;
}

REGISTER_HELP(CLUSTER_ADDINSTANCE_BRIEF, "Adds an Instance to the cluster.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_PARAM, "@param instance An instance definition.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_PARAM1, "@param password Optional string with the password for the connection.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL, "This function adds an Instance to the cluster. ");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL1, "The Instance is added to the Default ReplicaSet of the cluster.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL2, "The instance definition can be any of:");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL3, "@li URI string.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL4, "@li Connection data dictionary.");
//REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL5, "@li An Instance object.");
REGISTER_HELP(CLUSTER_ADDINSTANCE_DETAIL6, "The password may be contained on the instance parameter or can be "\
"specified on the password parameter. When both are specified the password parameter "\
"is used instead of the one in the instance data.");

/**
* $(CLUSTER_ADDINSTANCE_BRIEF)
*
* $(CLUSTER_ADDINSTANCE_PARAM)
* $(CLUSTER_ADDINSTANCE_PARAM1)
*
* $(CLUSTER_ADDINSTANCE_DETAIL)
*
* $(CLUSTER_ADDINSTANCE_DETAIL1)
*
* $(CLUSTER_ADDINSTANCE_DETAIL2)
* $(CLUSTER_ADDINSTANCE_DETAIL3)
* $(CLUSTER_ADDINSTANCE_DETAIL4)
*
* $(CLUSTER_ADDINSTANCE_DETAIL6)
*/
#if DOXYGEN_JS
Undefined Cluster::addInstance(InstanceDef instance, String password) {}
#elif DOXYGEN_PY
None Cluster::add_instance(InstanceDef instance, str password) {}
#endif
shcore::Value Cluster::add_instance(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("addInstance").c_str());

  check_preconditions("addInstance");

  // Add the Instance to the Default ReplicaSet
  shcore::Value ret_val;
  try {
    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    ret_val = _default_replica_set->add_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("addInstance"));

  return ret_val;
}

REGISTER_HELP(CLUSTER_REJOININSTANCE_BRIEF, "Rejoins an Instance to the cluster.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_PARAM, "@param instance An instance definition.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_PARAM1, "@param password Optional string with the password for the connection.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL, "This function rejoins an Instance to the cluster. ");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL2, "The instance definition can be any of:");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL3, "@li URI string.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL4, "@li Connection data dictionary.");
//REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL4, "@li An Instance object.");
REGISTER_HELP(CLUSTER_REJOININSTANCE_DETAIL5, "The password may be contained on the connectionData parameter or can be "\
"specified on the password parameter. When both are specified the password parameter "\
"is used instead of the one in the instance data.");

/**
* $(CLUSTER_REJOININSTANCE_BRIEF)
*
* $(CLUSTER_REJOININSTANCE_PARAM)
* $(CLUSTER_REJOININSTANCE_PARAM2)
*
* $(CLUSTER_REJOININSTANCE_DETAIL)
*
* $(CLUSTER_REJOININSTANCE_DETAIL1)
*
* $(CLUSTER_REJOININSTANCE_DETAIL2)
* $(CLUSTER_REJOININSTANCE_DETAIL3)
* $(CLUSTER_REJOININSTANCE_DETAIL4)
*
* $(CLUSTER_REJOININSTANCE_DETAIL5)
*/
#if DOXYGEN_JS
Undefined Cluster::rejoinInstance(InstanceDef instance) {}
#elif DOXYGEN_PY
None Cluster::rejoin_instance(InstanceDef instance) {}
#endif

shcore::Value Cluster::rejoin_instance(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("rejoinInstance").c_str());

  check_preconditions("rejoinInstance");

  // rejoin the Instance to the Default ReplicaSet
  shcore::Value ret_val;
  try {
    // Check if we have a Default ReplicaSet
    if (!_default_replica_set)
      throw shcore::Exception::logic_error("ReplicaSet not initialized.");

    // if not, call mysqlprovision to join the instance to its own group
    ret_val = _default_replica_set->rejoin_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rejoinInstance"));

  return ret_val;
}

REGISTER_HELP(CLUSTER_REMOVEINSTANCE_BRIEF, "Removes an Instance from the cluster.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_PARAM, "@param instance An instance definition.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL, "This function removes an Instance from the cluster.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL1, "The Instance is removed from the Default ReplicaSet of the cluster.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL2, "The instance parameter can be any of:");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL3, "@li The name of the Instance to be removed.");
REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL4, "@li Connection data Dictionary of the Instance to be removed.");
//REGISTER_HELP(CLUSTER_REMOVEINSTANCE_DETAIL5, "@li An Instance object.");

/**
* $(CLUSTER_REMOVEINSTANCE_BRIEF)
*
* $(CLUSTER_REMOVEINSTANCE_PARAM)
* $(CLUSTER_REMOVEINSTANCE_PARAMALT)
*
* $(CLUSTER_REMOVEINSTANCE_DETAIL)
*
* $(CLUSTER_REMOVEINSTANCE_DETAIL1)
*
* $(CLUSTER_REMOVEINSTANCE_DETAIL2)
* $(CLUSTER_REMOVEINSTANCE_DETAIL3)
* $(CLUSTER_REMOVEINSTANCE_DETAIL4)
*/
#if DOXYGEN_JS
Undefined Cluster::removeInstance(InstanceDef instance) {}
#elif DOXYGEN_PY
None Cluster::remove_instance(InstanceDef instance) {}
#endif

shcore::Value Cluster::remove_instance(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("removeInstance").c_str());

  check_preconditions("removeInstance");

  // Remove the Instance from the Default ReplicaSet
  try {
    _default_replica_set->remove_instance(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("removeInstance"));

  return Value();
}

#if 0 // Hidden for now
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
  shcore::Value ret_val;

  if (args.size() == 0)
    ret_val = shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(_default_replica_set));

  else {
    args.ensure_count(1, get_function_name("getReplicaSet").c_str());
    std::string name = args.string_at(0);

    if (name == "default")
      ret_val = shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(_default_replica_set));

    else {
      /*
       * Retrieve the ReplicaSet from the ReplicaSets array
       */
      //if (found)

      //else
      //  throw shcore::Exception::runtime_error("The ReplicaSet " + _name + "." + name + " does not exist");
    }
  }

  return ret_val;
}

void Cluster::set_default_replicaset(std::shared_ptr<ReplicaSet> default_rs) {
  _default_replica_set = default_rs;

  if (_default_replica_set)
    _default_replica_set->set_cluster(shared_from_this());
};

REGISTER_HELP(CLUSTER_DESCRIBE_BRIEF, "Describe the structure of the cluster.");
REGISTER_HELP(CLUSTER_DESCRIBE_RETURN, "@return A JSON object describing the structure of the cluster.");
REGISTER_HELP(CLUSTER_DESCRIBE_DETAIL, "This function describes the structure of the cluster including all its information, ReplicaSets and Instances.");

/**
* $(CLUSTER_DESCRIBE_BRIEF)
*
* $(CLUSTER_DESCRIBE_RETURN)
*
* $(CLUSTER_DESCRIBE_DETAIL)
*/
#if DOXYGEN_JS
String Cluster::describe() {}
#elif DOXYGEN_PY
str Cluster::describe() {}
#endif

shcore::Value Cluster::describe(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("describe").c_str());

  auto state = check_preconditions("describe");

  bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                  state.source_state != ManagedInstance::OnlineRO);

  shcore::Value ret_val;
  try {
    if (!_metadata_storage->cluster_exists(_name))
      throw Exception::argument_error("The cluster '" + _name + "' no longer exists.");

    ret_val = shcore::Value::new_map();

    auto description = ret_val.as_map();

    (*description)["clusterName"] = shcore::Value(_name);

    if (!_default_replica_set)
      (*description)["defaultReplicaSet"] = shcore::Value::Null();
    else
      (*description)["defaultReplicaSet"] = _default_replica_set->get_description();

    if (warning) {
      std::string warning = "The instance description may be outdated since was generated from an instance in ";
      warning.append(ManagedInstance::describe(static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*description)["warning"] = shcore::Value(warning);
    }
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("describe"));

  return ret_val;
}

REGISTER_HELP(CLUSTER_STATUS_BRIEF, "Describe the status of the cluster.");
REGISTER_HELP(CLUSTER_STATUS_RETURN, "@return A JSON object describing the status of the cluster.");
REGISTER_HELP(CLUSTER_STATUS_DETAIL, "This function describes the status of the cluster including its ReplicaSets and Instances.");

/**
* $(CLUSTER_STATUS_BRIEF)
*
* $(CLUSTER_STATUS_RETURN)
*
* $(CLUSTER_STATUS_DETAIL)
*/
#if DOXYGEN_JS
String Cluster::status() {}
#elif DOXYGEN_PY
str Cluster::status() {}
#endif

shcore::Value Cluster::status(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("status").c_str());

  auto state = check_preconditions("status");

  bool warning = (state.source_state != ManagedInstance::OnlineRW &&
                  state.source_state != ManagedInstance::OnlineRO);

  shcore::Value ret_val;
  try {
    ret_val = shcore::Value::new_map();

    auto status = ret_val.as_map();

    (*status)["clusterName"] = shcore::Value(_name);

    if (!_default_replica_set)
      (*status)["defaultReplicaSet"] = shcore::Value::Null();
    else
      (*status)["defaultReplicaSet"] = _default_replica_set->get_status(state);

    if (warning) {
      std::string warning = "The instance status may be inaccurate as it was generated from an instance in ";
      warning.append(ManagedInstance::describe(static_cast<ManagedInstance::State>(state.source_state)));
      warning.append(" state");
      (*status)["warning"] = shcore::Value(warning);
    }
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("status"));

  return ret_val;
}

REGISTER_HELP(CLUSTER_DISSOLVE_BRIEF, "Dissolves the cluster.");
REGISTER_HELP(CLUSTER_DISSOLVE_PARAM, "@param options Optional parameter to specify if it should deactivate replication and unregister the ReplicaSets from the cluster.");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL, "This function disables replication on the ReplicaSets, unregisters them and the the cluster from the metadata.");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL1, "It keeps all the user's data intact.");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL2, "The following is the only option supported:");
REGISTER_HELP(CLUSTER_DISSOLVE_DETAIL3, "@li force: boolean, confirms that the dissolve operation must be executed.");

/**
* $(CLUSTER_DISSOLVE_BRIEF)
*
* $(CLUSTER_DISSOLVE_PARAM)
*
* $(CLUSTER_DISSOLVE_DETAIL)
* $(CLUSTER_DISSOLVE_DETAIL1)
*/
#if DOXYGEN_JS
Undefined Cluster::dissolve(Dictionary options) {}
#elif DOXYGEN_PY
None Cluster::dissolve(Dictionary options) {}
#endif

shcore::Value Cluster::dissolve(const shcore::Argument_list &args) {
  args.ensure_count(0, 1, get_function_name("dissolve").c_str());

  check_preconditions("dissolve");

  try {
    shcore::Value::Map_type_ref options;

    bool force = false;
    if (args.size() == 1)
      options = args.map_at(0);

    if (options) {
      // Verification of invalid attributes on the instance creation options
      shcore::Argument_map opt_map(*options);

      opt_map.ensure_keys({}, {"force"}, "dissolve options");

      if (opt_map.has_key("force"))
        force = opt_map.bool_at("force");
    }

    MetadataStorage::Transaction tx(_metadata_storage);
    std::string cluster_name = get_name();

    // We need to check if the group has quorum and if not we must abort the operation
    // otherwise we GR blocks the writes to preserve the consistency of the group and we end up
    // with a hang.
    auto session = _metadata_storage->get_dba()->get_active_session();
    mysqlsh::mysql::ClassicSession *classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());

    // check if the Cluster is empty
    if (_metadata_storage->is_cluster_empty(get_id())) {
      _metadata_storage->drop_cluster(cluster_name);
      tx.commit();
    } else {
      if (force) {
        // Gets the instances on the only available replica set
        auto instances = _metadata_storage->get_replicaset_instances(_default_replica_set->get_id());

        _metadata_storage->drop_replicaset(_default_replica_set->get_id());

        // TODO: we only have the Default ReplicaSet, but will have more in the future
        _metadata_storage->drop_cluster(cluster_name);

        tx.commit();

        // once the data changes are done, we proceed doing the remove from GR
        _default_replica_set->remove_instances_from_gr(instances);
      } else {
        throw Exception::logic_error("Cannot drop cluster: The cluster is not empty.");
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dissolve"))

  return Value();
}

REGISTER_HELP(CLUSTER_RESCAN_BRIEF, "Rescans the cluster.");
REGISTER_HELP(CLUSTER_RESCAN_DETAIL, "This function rescans the cluster for new Group Replication members/instances.");

/**
* $(CLUSTER_RESCAN_BRIEF)
*
* $(CLUSTER_RESCAN_DETAIL)
*/
#if DOXYGEN_JS
Undefined Cluster::rescan() {}
#elif DOXYGEN_PY
None Cluster::rescan() {}
#endif

shcore::Value Cluster::rescan(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("rescan").c_str());

  check_preconditions("rescan");

  shcore::Value ret_val;
  try {
    ret_val = shcore::Value(_rescan(args));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("rescan"));

  return ret_val;
}

shcore::Value::Map_type_ref Cluster::_rescan(const shcore::Argument_list &args) {
  shcore::Value::Map_type_ref ret_val(new shcore::Value::Map_type());

  // Check if we have a Default ReplicaSet
  if (!_default_replica_set)
    throw shcore::Exception::logic_error("ReplicaSet not initialized.");

  // Rescan the Default ReplicaSet
  (*ret_val)["defaultReplicaSet"] = _default_replica_set->rescan(args);

  return ret_val;
}

void Cluster::set_option(const std::string& option, const shcore::Value& value) {
  if (!_options)
    _options.reset(new shcore::Value::Map_type());

  (*_options)[option] = value;
}

void Cluster::set_attribute(const std::string& attribute, const shcore::Value& value) {
  if (!_attributes)
    _attributes.reset(new shcore::Value::Map_type());

  (*_attributes)[attribute] = value;
}

REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_BRIEF, "Verifies the instance gtid state in relation with the cluster.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_PARAM, "@param instance An instance definition.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_PARAM1, "@param password Optional string with the password for the connection.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL, "Analyzes the instance executed GTIDs with the executed/purged GTIDs on the cluster "\
                                                 "to determine if the instance is valid for the cluster.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL1, "The instance definition can be any of:");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL2, "@li URI string.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL3, "@li Connection data dictionary.");
//REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL5, "@li An Instance object.");
REGISTER_HELP(CLUSTER_CHECKINSTANCESTATE_DETAIL4, "The password may be contained on the instance parameter or can be "\
"specified on the password parameter. When both are specified the password parameter "\
"is used instead of the one in the instance data.");

/**
 * $(CLUSTER_CHECKINSTANCESTATE_BRIEF)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_PARAM)
 * $(CLUSTER_CHECKINSTANCESTATE_PARAM1)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL1)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL2)
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL3)
 *
 * $(CLUSTER_CHECKINSTANCESTATE_DETAIL4)
 */
#if DOXYGEN_JS
Undefined Cluster::checkInstanceState(InstanceDef instance, String password) {}
#elif DOXYGEN_PY
None Cluster::check_instance_state(InstanceDef instance, str password) {}
#endif
shcore::Value Cluster::check_instance_state(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("checkInstanceState").c_str());

  check_preconditions("checkInstanceState");

  shcore::Value ret_val;
  // Verifies the transaction state of the instance ins relation to the cluster
  try {
    ret_val = get_default_replicaset()->retrieve_instance_state(args);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("checkInstanceState"));

  return ret_val;
}

ReplicationGroupState Cluster::check_preconditions(const std::string& function_name) const {
  return check_function_preconditions(class_name(), function_name, get_function_name(function_name), _metadata_storage);
}
