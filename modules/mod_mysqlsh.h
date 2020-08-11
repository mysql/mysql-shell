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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_MOD_MYSQLSH_H_
#define MODULES_MOD_MYSQLSH_H_

#include <memory>
#include <string>
#include "mysqlshdk/include/scripting/module_registry.h"

#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/include/scripting/types.h"

namespace mysqlsh {

#if DOXYGEN_JS
Dba connectDba(ConnectionData connectionData);
#elif DOXYGEN_PY
Dba connect_dba(ConnectionData connectionData);
#endif

DECLARE_MODULE(Mysqlsh, mysqlsh);

// We need to hide this from doxygen to avoid warnings
#if !defined(DOXYGEN_JS) && !defined(DOXYGEN_PY)
shcore::Value get_member(const std::string &prop) const;

std::shared_ptr<dba::Dba> connect_dba(
    const mysqlshdk::db::Connection_options &connection_options);
#endif

END_DECLARE_MODULE();

}  // namespace mysqlsh

#endif  // MODULES_MOD_MYSQLSH_H_
