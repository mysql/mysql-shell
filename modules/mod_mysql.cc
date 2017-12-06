/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysql.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_mysql_constants.h"
#include "shellcore/utils_help.h"
#include "modules/mysqlxtest_utils.h"

using namespace std::placeholders;
namespace mysqlsh {
namespace mysql {

// clang-format off
REGISTER_HELP(MYSQL_INTERACTIVE_BRIEF, "Used to work with classic MySQL sessions using SQL.");
REGISTER_HELP(MYSQL_BRIEF, "Encloses the functions and classes available to interact with a MySQL Server using the traditional "\
                           "MySQL Protocol.");

REGISTER_HELP(MYSQL_DETAIL, "Use this module to create a session using the traditional MySQL Protocol, for example for MySQL Servers where "\
                            "the X Protocol is not available.");
REGISTER_HELP(MYSQL_DETAIL1,"Note that the API interface on this module is very limited, even you can load schemas, tables and views as "\
                            "objects there are no operations available on them.");
REGISTER_HELP(MYSQL_DETAIL2,"The purpose of this module is to allow SQL Execution on MySQL Servers where the X Protocol is not enabled.");
REGISTER_HELP(MYSQL_DETAIL3,"To use the properties and functions available on this module you first need to import it.");
REGISTER_HELP(MYSQL_DETAIL4,"When running the shell in interactive mode, this module is automatically imported.");
// clang-format on

REGISTER_MODULE(Mysql, mysql) {
  REGISTER_VARARGS_FUNCTION(Mysql, get_classic_session, getClassicSession);
  REGISTER_VARARGS_FUNCTION(Mysql, get_session, getSession);

  _type.reset(new Type());
}

// We need to hide this from doxygen to avoif warnings
#if !defined(DOXYGEN_JS) && !defined(DOXYGEN_PY)
shcore::Value Mysql::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "Type")
    ret_val = shcore::Value(_type);
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}
#endif

// clang-format off
REGISTER_HELP(MYSQL_GETCLASSICSESSION_BRIEF, "Opens a classic MySQL protocol session to a MySQL server.");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_PARAM,  "@param connectionData The connection data for the session");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_PARAM1, "@param password Optional password for the session");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_RETURNS, "@returns A ClassicSession");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_DETAIL, "A ClassicSession object uses the traditional MySQL Protocol to allow executing operations on the "\
                                              "connected MySQL Server.");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_DETAIL1, "TOPIC_CONNECTION_DATA");
// clang-format on

/**
 * \ingroup mysql
 * $(MYSQL_GETCLASSICSESSION_BRIEF)
 *
 * $(MYSQL_GETCLASSICSESSION_PARAM)
 * $(MYSQL_GETCLASSICSESSION_PARAM1)
 *
 * $(MYSQL_GETCLASSICSESSION_RETURNS)
 *
 * $(MYSQL_GETCLASSICSESSION_DETAIL)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref connection_data
 *
 */

#if DOXYGEN_JS
ClassicSession getClassicSession(ConnectionData connectionData, String password){}
#elif DOXYGEN_PY
ClassicSession get_classic_session(ConnectionData connectionData, str password){}
#endif

DEFINE_FUNCTION(Mysql, get_classic_session) {
  args.ensure_count(1, 2, get_function_name("getClassicSession").c_str());

  shcore::Value ret_val;
  try {
  ret_val = shcore::Value(ClassicSession::create(args));
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getClassicSession"));

  return ret_val;
}

REGISTER_HELP(MYSQL_GETSESSION_BRIEF, "Opens a classic MySQL protocol session to a MySQL server.");
REGISTER_HELP(MYSQL_GETSESSION_PARAM,  "@param connectionData The connection data for the session");
REGISTER_HELP(MYSQL_GETSESSION_PARAM1, "@param password Optional password for the session");
REGISTER_HELP(MYSQL_GETSESSION_RETURNS, "@returns A ClassicSession");
REGISTER_HELP(MYSQL_GETSESSION_DETAIL, "A ClassicSession object uses the traditional MySQL Protocol to allow executing operations on the "\
                                              "connected MySQL Server.");
REGISTER_HELP(MYSQL_GETSESSION_DETAIL1, "TOPIC_CONNECTION_DATA");
// clang-format on

/**
 * \ingroup mysql
 * $(MYSQL_GETSESSION_BRIEF)
 *
 * $(MYSQL_GETSESSION_PARAM)
 * $(MYSQL_GETSESSION_PARAM1)
 *
 * $(MYSQL_GETSESSION_RETURNS)
 *
 * $(MYSQL_GETSESSION_DETAIL)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref connection_data
 *
 */
#if DOXYGEN_JS
ClassicSession getSession(ConnectionData connectionData, String password) {}
#elif DOXYGEN_PY
ClassicSession get_session(ConnectionData connectionData, str password) {}
#endif

DEFINE_FUNCTION(Mysql, get_session) {
  args.ensure_count(1, 2, get_function_name("getSession").c_str());

  shcore::Value ret_val;
  try {
  ret_val = shcore::Value(ClassicSession::create(args));
  } CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("getSession"));

  return ret_val;
}

}
}
