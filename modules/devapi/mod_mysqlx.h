/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_DEVAPI_MOD_MYSQLX_H_
#define MODULES_DEVAPI_MOD_MYSQLX_H_

#include <string>
#include "scripting/module_registry.h"

namespace mysqlshdk {
namespace db {

class Connection_options;

}  // namespace db
}  // namespace mysqlshdk

namespace mysqlsh {

/**
 *\defgroup mysqlx mysqlx
 *\ingroup XDevAPI
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
 * mysql-js> var mySession = mysqlx.getSession('admin@localhost');
 * \endcode
 * \elseif DOXYGEN_PY
 * \code
 * mysql-py> from mysqlsh import mysqlx
 *
 * // Then you can use the module functions and properties
 * // for example to create a session
 * mysql-py> mySession = mysqlx.get_session('admin@localhost')
 * \endcode
 * \endif
 *
 * $(MYSQLX_DETAIL4)
 */
namespace mysqlx {

#if DOXYGEN_JS
Types Types;  //!< $(MYSQLX_TYPE_BRIEF)
Session getSession(ConnectionData connectionData, String password);
Expression expr(String expressionStr);
Date dateValue(Integer year, Integer month, Integer day, Integer hour,
               Integer minutes, Integer seconds, Integer milliseconds);
#elif DOXYGEN_PY
Types Types;  //!< $(MYSQLX_TYPE_BRIEF)
Session get_session(ConnectionData connectionData, str password);
Expression expr(str expressionStr);
Date date_value(int year, int month, int day, int hour, int minutes,
                int seconds, int milliseconds);
#endif

DECLARE_MODULE(Mysqlx, mysqlx);

// We need to hide these from doxygen to avoid warnings
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
std::shared_ptr<shcore::Object_bridge> get_session(
    const mysqlshdk::db::Connection_options &co,
    const char *password = nullptr);

DECLARE_FUNCTION(expr);

std::shared_ptr<shcore::Object_bridge> date_value(int64_t year, int64_t month,
                                                  int64_t day);
std::shared_ptr<shcore::Object_bridge> date_value(int64_t year, int64_t month,
                                                  int64_t day, int64_t hour,
                                                  int64_t minutes,
                                                  int64_t seconds,
                                                  int64_t milliseconds = 0);

virtual shcore::Value get_member(const std::string &prop) const;
#endif

private:
shcore::Object_bridge_ref _type;
shcore::Object_bridge_ref _lock_contention;

END_DECLARE_MODULE();
}  // namespace mysqlx
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_MOD_MYSQLX_H_
