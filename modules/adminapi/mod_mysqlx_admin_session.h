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

#ifndef _MOD_MYSQLX_ADMIN_SESSION_H_
#define _MOD_MYSQLX_ADMIN_SESSION_H_

#include "../mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "../base_session.h"
#include "../mod_mysqlx_session_handle.h"

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
    class SHCORE_PUBLIC AdminSession : public ShellAdminSession, public std::enable_shared_from_this<AdminSession>
    {
    public:
      AdminSession();
      AdminSession(const AdminSession& s);
      virtual ~AdminSession() { reset_session(); }

      virtual std::string class_name() const { return "AdminSession"; };

      virtual shcore::Value get_member(const std::string &prop) const;
      virtual bool has_member(const std::string &prop) const;
      virtual std::vector<std::string> get_members() const;

      virtual shcore::Value connect(const shcore::Argument_list &args);
      virtual shcore::Value close(const shcore::Argument_list &args);

      virtual bool is_connected() const;
      virtual shcore::Value get_status(const shcore::Argument_list &args);
      virtual shcore::Value get_capability(const std::string& name);

      shcore::Value create_farm(const shcore::Argument_list &args);
      shcore::Value drop_farm(const shcore::Argument_list &args);
      shcore::Value get_farm(const shcore::Argument_list &args) const;
      shcore::Value drop_metadata_schema(const shcore::Argument_list &args);

      virtual void set_option(const char *option, int value);

      virtual uint64_t get_connection_id() const;
      virtual std::string db_object_exists(std::string &type, const std::string &name, const std::string& owner) const;

      static std::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
      virtual int get_default_port() { return 33060; };

      SessionHandle get_session() const;

      std::string get_user() { return _user; };
      std::string get_password() { return _password; };

#if DOXYGEN_JS
      String uri; //!< Same as getUri()
      Farm defaultFarm; //!< Same as getDefaultSchema()

      Farm createFarm(String name);
      Undefined dropFarm(String name);
      Farm getFarm(String name);
      Farm getDefaultFarm();
      Undefined dropMetadataSchema();
      String getUri();
      Undefined close();

#elif DOXYGEN_PY
      str uri; //!< Same as get_uri()
      Farm defaultFarm; //!< Same as get_default_schema()

      Farm create_farm(str name);
      None drop_farm(str name);
      Farm get_farm(str name);
      Farm get_default_farm();
      None drop_metadata_schema();
      str get_uri();
      None close();
#endif

    protected:
      SessionHandle _session;

      void init();
    private:
      std::shared_ptr<MetadataStorage> _metadata_storage;
      uint64_t _connection_id;

      void reset_session();
      std::string generate_password(int password_lenght);
    };
  }
}

#endif
