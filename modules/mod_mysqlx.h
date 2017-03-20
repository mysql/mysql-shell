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

#ifndef _MOD_MYSQLX_H_
#define _MOD_MYSQLX_H_

#include "scripting/module_registry.h"

namespace mysqlsh {

/**
  * $(MYSQLX_BRIEF)
  *
  * $(MYSQLX_DETAIL)
  *
  * $(MYSQLX_DETAIL1)
  *
  * $(MYSQLX_DETAIL2)
  *
  * $(MYSQLX_DETAIL3)
  *
  * \if DOXYGEN_JS
  * \code
  * mysql-js> var mysqlx = require('mysqlx');
  *
  * // Then you can use the module functions and properties
  * // for example to create a session
  * mysql-js> var mySession = mysqlx.getNodeSession('admin@localhost');
  * \endcode
  * \elseif DOXYGEN_PY
  * \code
  * mysql-py> from mysqlsh import mysqlx
  *
  * // Then you can use the module functions and properties
  * // for example to create a session
  * mysql-py> mySession = mysqlx.get_node_session('admin@localhost')
  * \endcode
  * \endif
  *
  * $(MYSQLX_DETAIL4)
  */
namespace mysqlx {

#if DOXYGEN_JS
Types Types; //!< $(MYSQLX_TYPE_BRIEF)
IndexType IndexType;  //!< $(MYSQLX_INDEXTYPE_BRIEF)
NodeSession getNodeSession(ConnectionData connectionData, String password);
Expression expr(String expressionStr);
#elif DOXYGEN_PY
Types Types; //!< $(MYSQLX_TYPE_BRIEF)
IndexType IndexType;  //!< $(MYSQLX_INDEXTYPE_BRIEF)
NodeSession get_node_session(ConnectionData connectionData, str password);
Expression expr(str expressionStr);
#endif

DECLARE_MODULE(Mysqlx, mysqlx);

//DECLARE_FUNCTION(get_session);
DECLARE_FUNCTION(get_node_session);
DECLARE_FUNCTION(expr);
DECLARE_FUNCTION(date_value);

// We need to hide this from doxygen to avoif warnings
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
virtual shcore::Value get_member(const std::string &prop) const;
#endif

private:
  shcore::Object_bridge_ref _type;
  shcore::Object_bridge_ref _index_type;

  END_DECLARE_MODULE();
}
}

#endif
