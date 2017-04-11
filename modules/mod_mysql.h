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

#ifndef _MOD_MYSQL_H_
#define _MOD_MYSQL_H_

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
ClassicSession getClassicSession(ConnectionData connectionData, String password);
#elif DOXYGEN_PY
ClassicSession get_classic_session(ConnectionData connectionData, str password);
#endif

DECLARE_MODULE(Mysql, mysql);

DECLARE_FUNCTION(get_classic_session);

END_DECLARE_MODULE();
}
}

#endif
