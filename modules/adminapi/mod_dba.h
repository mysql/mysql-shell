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
#include "mod_dba_farm.h"
#include <set>

namespace mysh
{
  namespace mysqlx
  {
    class MetadataStorage;

    /**
    * This class represents a connection to a Metadata Store and enables
    *
    * - Accessing available Farms.
    * - Farm management operations.
    */
    class SHCORE_PUBLIC Dba : public shcore::Cpp_object_bridge, public std::enable_shared_from_this<Dba>
    {
    public:
      Dba(shcore::IShell_core* owner);
      virtual ~Dba() { /*reset_session();*/ }

      static std::set<std::string> _deploy_instance_opts;

      virtual std::string class_name() const { return "Dba"; };

      virtual shcore::Value get_member(const std::string &prop) const;

      virtual bool operator == (const Object_bridge &other) const;

      std::shared_ptr<ShellDevelopmentSession> get_active_session();
      virtual int get_default_port() { return 33060; };
      int get_default_instance_port() { return 3306; }

      shcore::Value validate_instance(const shcore::Argument_list &args);
      shcore::Value deploy_local_instance(const shcore::Argument_list &args);
      shcore::Value clone_instance(const shcore::Argument_list &args);
      shcore::Value configure_instance(const shcore::Argument_list &args);
      shcore::Value reset_instance(const shcore::Argument_list &args);

      shcore::Value reset_session(const shcore::Argument_list &args);
      shcore::Value create_farm(const shcore::Argument_list &args);
      shcore::Value drop_farm(const shcore::Argument_list &args);
      shcore::Value get_farm(const shcore::Argument_list &args) const;
      shcore::Value drop_metadata_schema(const shcore::Argument_list &args);

      Farm get_default_farm();

#if DOXYGEN_JS
      Farm defaultFarm; //!< Same as getDefaultSchema()

      Farm createFarm(String name);
      Undefined dropFarm(String name);
      Farm getFarm(String name);
      Farm getDefaultFarm();
      Undefined dropMetadataSchema();

#elif DOXYGEN_PY
      Farm defaultFarm; //!< Same as get_default_schema()

      Farm create_farm(str name);
      None drop_farm(str name);
      Farm get_farm(str name);
      Farm get_default_farm();
      None drop_metadata_schema();
#endif

    protected:
      std::shared_ptr<mysh::ShellDevelopmentSession> _custom_session;
      shcore::IShell_core *_shell_core;

      mutable std::shared_ptr<shcore::Value::Map_type> _farms;
      mutable std::string _default_farm_name;
      mutable std::shared_ptr<mysh::mysqlx::Farm> _default_farm;

      void init();
    private:

      std::shared_ptr<MetadataStorage> _metadata_storage;
      uint64_t _connection_id;

      std::string generate_password(int password_lenght);
    };
  }
}

#endif
