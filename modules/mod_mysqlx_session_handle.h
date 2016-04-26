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

#ifndef _MOD_MYSQLX_SESSION_HANDLE_H_
#define _MOD_MYSQLX_SESSION_HANDLE_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "mysqlxtest/mysqlx.h"

namespace mysh
{
  namespace mysqlx
  {
    // This is a helper class that wraps a connection created using mysqlxtest
    // and the different operations available through it.
    class SHCORE_PUBLIC SessionHandle
    {
    public:
      bool is_connected() const { return _session ? true : false; }
      boost::shared_ptr< ::mysqlx::Session> get() const { return _session; }
      void open(const std::string &host, int port, const std::string &schema,
                const std::string &user, const std::string &pass,
                const std::string &ssl_ca, const std::string &ssl_cert,
                const std::string &ssl_key, const std::size_t timeout,
                const std::string &auth_method = "", const bool get_caps = false);

      boost::shared_ptr< ::mysqlx::Result> execute_sql(const std::string &sql) const;
      void enable_protocol_trace(bool value);
      void reset();
      boost::shared_ptr< ::mysqlx::Result> execute_statement(const std::string &domain, const std::string& command, const shcore::Argument_list &args) const;

      shcore::Value get_capability(const std::string& name);
      uint64_t get_client_id();

    private:
      mutable boost::shared_ptr< ::mysqlx::Result> _last_result;
      boost::shared_ptr< ::mysqlx::Session> _session;

      ::mysqlx::ArgumentValue get_argument_value(shcore::Value source) const;
    };
  }
}

#endif
