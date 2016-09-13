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

// Interactive session access module for MySQL X sessions
// Exposed as "session" in the shell

#ifndef _MOD_DBA_H_
#define _MOD_DBA_H_

#include "modules/mod_common.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "modules/base_session.h"
#include "mod_dba_cluster.h"
#include <set>
#include "mod_dba_provisioning_interface.h"

namespace mysh {
namespace dba {
class MetadataStorage;

/**
* This class represents a connection to a Metadata Store and enables
*
* - Accessing available Clusters.
* - Cluster management operations.
*/
class SHCORE_PUBLIC Dba : public shcore::Cpp_object_bridge, public std::enable_shared_from_this<Dba> {
public:
  Dba(shcore::IShell_core* owner);
  virtual ~Dba() { /*reset_session();*/ }

  static std::set<std::string> _deploy_instance_opts;
  static std::set<std::string> _validate_instance_opts;

  virtual std::string class_name() const { return "Dba"; };

  virtual bool operator == (const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

  std::shared_ptr<ShellDevelopmentSession> get_active_session() const;
  virtual int get_default_port() { return 33060; };
  int get_default_instance_port() { return 3306; }

  shcore::Value validate_instance(const shcore::Argument_list &args);
  shcore::Value deploy_local_instance(const shcore::Argument_list &args, const std::string &fname); // create and start
  shcore::Value stop_local_instance(const shcore::Argument_list &args);
  shcore::Value delete_local_instance(const shcore::Argument_list &args);
  shcore::Value kill_local_instance(const shcore::Argument_list &args);

  shcore::Value clone_instance(const shcore::Argument_list &args);
  shcore::Value configure_instance(const shcore::Argument_list &args);
  shcore::Value reset_instance(const shcore::Argument_list &args);

  shcore::Value reset_session(const shcore::Argument_list &args);
  shcore::Value create_cluster(const shcore::Argument_list &args);
  shcore::Value get_cluster(const shcore::Argument_list &args) const;
  shcore::Value drop_metadata_schema(const shcore::Argument_list &args);

  shcore::IShell_core* get_owner() { return _shell_core; }

#if DOXYGEN_JS
  Undefined resetSession(Session session);
  Cluster createCluster(String name, String masterKey, Dictionary options);
  Undefined dropCluster(String name);
  Cluster getCluster(String name, Dictionary options);
  Undefined dropMetadataSchema();

#elif DOXYGEN_PY
  None reset_session(Session session);
  Cluster create_cluster(str name, str masterKey, Dictionary options);
  None drop_cluster(str name);
  Cluster get_cluster(str name, Dictionary options);
  None drop_metadata_schema();
#endif

  void validate_session(const std::string &source) const;

protected:
  std::shared_ptr<mysh::ShellDevelopmentSession> _custom_session;
  shcore::IShell_core *_shell_core;

  void init();

private:
  virtual std::string get_help_text(const std::string& topic, bool full);

  std::shared_ptr<MetadataStorage> _metadata_storage;
  uint64_t _connection_id;
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

  std::string generate_password(int password_lenght);

  shcore::Value exec_instance_op(const std::string &function, const shcore::Argument_list &args);
};
}
}

#endif
