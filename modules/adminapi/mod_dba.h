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
#include "modules/mod_mysql_session.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "modules/base_session.h"
#include "mod_dba_cluster.h"
#include <set>
#include <map>
#include "mod_dba_provisioning_interface.h"

namespace mysh {
namespace dba {
class MetadataStorage;

/**
* $(DBA_BRIEF)
*/
class SHCORE_PUBLIC Dba : public shcore::Cpp_object_bridge, public std::enable_shared_from_this<Dba> {
public:
  Dba(shcore::IShell_core* owner);
  virtual ~Dba();

  static std::set<std::string> _deploy_instance_opts;
  static std::set<std::string> _check_instance_config_opts;
  static std::set<std::string> _config_local_instance_opts;

  virtual std::string class_name() const { return "Dba"; };

  virtual bool operator == (const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

  std::shared_ptr<ShellDevelopmentSession> get_active_session() const;
  virtual int get_default_port() { return 33060; };
  int get_default_instance_port() { return 3306; }

  shcore::Value check_instance_config(const shcore::Argument_list &args);
  shcore::Value deploy_sandbox_instance(const shcore::Argument_list &args, const std::string &fname); // create and start
  shcore::Value stop_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value delete_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value kill_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value start_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value config_local_instance(const shcore::Argument_list &args);

  shcore::Value clone_instance(const shcore::Argument_list &args);
  shcore::Value reset_instance(const shcore::Argument_list &args);

  shcore::Value reset_session(const shcore::Argument_list &args);
  shcore::Value create_cluster(const shcore::Argument_list &args);
  shcore::Value get_cluster(const shcore::Argument_list &args) const;
  shcore::Value drop_metadata_schema(const shcore::Argument_list &args);

  shcore::IShell_core* get_owner() { return _shell_core; }

#if DOXYGEN_JS
  Boolean verbose; //!< $(DBA_VERBOSE_BRIEF)
  Cluster createCluster(String name, Dictionary options);
  Undefined deleteSandboxInstance(Integer port, Dictionary options);
  Instance deploySandboxInstance(Integer port, Dictionary options);
  Undefined dropMetadataSchema(Dictionary options);
  Cluster getCluster(String name);
  Undefined killSandboxInstance(Integer port, Dictionary options);
  Undefined resetSession(Session session);
  Undefined startSandboxInstance(Integer port, Dictionary options);
  Undefined stopSandboxInstance(Integer port, Dictionary options);
  Undefined checkInstanceConfig(Variant connectionData, String password);
  Instance configLocalInstance(Variant connectionData);
#elif DOXYGEN_PY
  bool verbose; //! $(DBA_VERBOSE)
  Cluster create_cluster(str name, dict options);
  None delete_sandbox_instance(int port, dict options);
  Instance deploy_sandbox_instance(int port, dict options);
  None drop_cluster(str name);
  None drop_metadata_schema(dict options);
  Cluster get_cluster(str name);
  None kill_sandbox_instance(int port, dict options);
  None reset_session(Session session);
  None start_sandbox_instance(int port, dict options);
  None stop_sandbox_instance(int port, dict options);
  None check_instance_config(variant connectionData, str password);
  Instance config_local_instance(variant connectionData);
#endif

  void validate_session(const std::string &source) const;

  static std::shared_ptr<mysh::mysql::ClassicSession> get_session(const shcore::Argument_list& args);

protected:
  std::shared_ptr<mysh::ShellDevelopmentSession> _custom_session;
  shcore::IShell_core *_shell_core;

  void init();

private:
  std::shared_ptr<MetadataStorage> _metadata_storage;
  uint64_t _connection_id;
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

  shcore::Value exec_instance_op(const std::string &function, const shcore::Argument_list &args);
  shcore::Value::Map_type_ref _check_instance_config(const shcore::Argument_list &args, bool allow_update);


  static std::map <std::string, std::shared_ptr<mysh::mysql::ClassicSession> > _session_cache;
};
}
}

#endif
