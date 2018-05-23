/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_MYSQL_H_
#define _MOD_MYSQL_H_

#include <string>
#include "scripting/module_registry.h"

namespace mysqlsh {
/**
 * \defgroup mysql mysql
 * \ingroup ShellAPI
 * $(MYSQL_BRIEF)
 *
 * $(MYSQL_DETAIL)
 *
 * $(MYSQL_DETAIL1)
 *
 * $(MYSQL_DETAIL2)
 *
 * $(MYSQL_DETAIL3)
 *
 * \if DOXYGEN_JS
 * \code
 * mysql-js> var mysql = require('mysql');
 *
 * // Then you can use the module functions and properties
 * // for example to create a session
 * mysql-js> var mySession = mysql.getClassicSession('admin@localhost');
 * \endcode
 * \elseif DOXYGEN_PY
 * \code
 * mysql-py> from mysqlsh import mysql
 *
 * // Then you can use the module functions and properties
 * // for example to create a session
 * mysql-py> mySession = mysql.get_classic_session('admin@localhost')
 * \endcode
 * \endif
 *
 * $(MYSQL_DETAIL4)
 */
namespace mysql {

#if DOXYGEN_JS
ClassicSession getClassicSession(ConnectionData connectionData,
                                 String password);
ClassicSession getSession(ConnectionData connectionData, String password);
#elif DOXYGEN_PY
ClassicSession get_classic_session(ConnectionData connectionData, str password);
ClassicSession get_session(ConnectionData connectionData, str password);
#endif

DECLARE_MODULE(Mysql, mysql);

DECLARE_FUNCTION(get_classic_session);
DECLARE_FUNCTION(get_session);

// We need to hide this from doxygen to avoif warnings
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
virtual shcore::Value get_member(const std::string &prop) const;
#endif

private:
shcore::Object_bridge_ref _type;

END_DECLARE_MODULE();
}  // namespace mysql
}  // namespace mysqlsh

#endif
