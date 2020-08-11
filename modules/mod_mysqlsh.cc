/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "modules/mod_mysqlsh.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/utils_help.h"

namespace mysqlsh {

REGISTER_MODULE(Mysqlsh, mysqlsh) {
  expose("connectDba", &Mysqlsh::connect_dba, "connectionData");
}

// We need to hide this from doxygen to avoid warnings
#if !defined(DOXYGEN_JS) && !defined(DOXYGEN_PY)
shcore::Value Mysqlsh::get_member(const std::string &) const { return {}; }
#endif

REGISTER_HELP_FUNCTION(connectDba, Mysqlsh);
REGISTER_HELP_FUNCTION_TEXT(MYSQLSH_CONNECTDBA, R"*(
Creates a new Dba object for the given target server.

@param connectionData Connection data for the target MySQL server.

@returns A new instance of the Dba object.
)*");

/**
 * $(MYSQLSH_CONNECTDBA_BRIEF)
 *
 * $(MYSQLSH_CONNECTDBA)
 */
#if DOXYGEN_JS
Dba Mysqlsh::connectDba(ConnectionData connectionData) {}
#elif DOXYGEN_PY
Dba Mysqlsh::connect_dba(ConnectionData connectionData) {}
#else
std::shared_ptr<dba::Dba> Mysqlsh::connect_dba(
    const mysqlshdk::db::Connection_options &connection_options) {
  auto session = establish_session(connection_options, false);

  return std::make_shared<dba::Dba>(ShellBaseSession::wrap_session(session));
}
#endif

}  // namespace mysqlsh
