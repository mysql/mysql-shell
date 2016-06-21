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
    /**
    * Base functionality for Session classes through the X Protocol.
    *
    * This class represents a connection to a Metadata Store and enables
    *
    * - Accessing available Clusters.
    * - Cluster management operations.
    */
    class SHCORE_PUBLIC AdminSession : public ShellAdminSession
    {
    public:
      AdminSession();
      AdminSession(const AdminSession& s);
      virtual ~AdminSession() { reset_session(); }

      virtual shcore::Value get_member(const std::string &prop) const;

      virtual shcore::Value connect(const shcore::Argument_list &args);
      virtual shcore::Value close(const shcore::Argument_list &args);

      virtual bool is_connected() const;
      virtual shcore::Value get_status(const shcore::Argument_list &args);
      virtual shcore::Value get_capability(const std::string& name);

      shcore::Value create_cluster(const shcore::Argument_list &args);
      shcore::Value get_cluster(const shcore::Argument_list &args) const;
      shcore::Value get_clusters(const shcore::Argument_list &args) const;

      virtual void set_option(const char *option, int value);

      virtual uint64_t get_connection_id() const;

      boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

#ifdef DOXYGEN
      String uri; //!< Same as getUri()
      Schema defaultCluster; //!< Same as getDefaultSchema()

      Cluster createCluster(String name);
      Cluster getCluster(String name);
      Cluster getDefaultCluster();
      List getClusters();
      String getUri();
      Undefined close();
#endif
    protected:
      virtual int get_default_port() { return 33060; };

      SessionHandle _session;

      void init();
    private:
      uint64_t _connection_id;
      void reset_session();
    };
  }
}

#endif
