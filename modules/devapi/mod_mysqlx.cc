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

#include "modules/devapi/mod_mysqlx.h"
#include <string>
#include "modules/devapi/mod_mysqlx_constants.h"
#include "modules/devapi/mod_mysqlx_expression.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mysqlxtest_utils.h"
#include "scripting/obj_date.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
namespace mysqlsh {
namespace mysqlx {

REGISTER_HELP(
    MYSQLX_INTERACTIVE_BRIEF,
    "Used to work with X Protocol sessions using the MySQL X DevAPI.");
REGISTER_HELP(
    MYSQLX_BRIEF,
    "Encloses the functions and classes available to interact with an X Protocol enabled MySQL Product.");
REGISTER_HELP(
    MYSQLX_DETAIL,
    "The objects contained on this module provide a full API to interact with the different MySQL Products "
    "implementing the X Protocol.");
REGISTER_HELP(
    MYSQLX_DETAIL1,
    "In the case of a MySQL Server the API will enable doing operations on the different database objects "
    "such as schema management operations and both table and collection management and CRUD operations. "
    "(CRUD: Create, Read, Update, Delete).");
REGISTER_HELP(
    MYSQLX_DETAIL2,
    "Intention of the module is to provide a full API for development through scripting languages such as "
    "JavaScript and Python, this would be normally achieved through a normal session.");
REGISTER_HELP(
    MYSQLX_DETAIL3,
    "To use the properties and functions available on this module you first need to import it.");
REGISTER_HELP(
    MYSQLX_DETAIL4,
    "When running the shell in interactive mode, this module is automatically imported.");

REGISTER_MODULE(Mysqlx, mysqlx) {
  add_property("Type|Type");
  add_property("IndexType|IndexType");
  REGISTER_VARARGS_FUNCTION(Mysqlx, get_session, getSession);
  REGISTER_VARARGS_FUNCTION(Mysqlx, date_value, dateValue);
  REGISTER_FUNCTION(Mysqlx, expr, expr, "expression", shcore::String, NULL);

  _type.reset(new Type());
  _index_type.reset(new IndexType());
}

// We need to hide this from doxygen to avoif warnings
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
shcore::Value Mysqlx::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "Type")
    ret_val = shcore::Value(_type);
  else if (prop == "IndexType")
    ret_val = shcore::Value(_index_type);
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}
#endif

// DEFINE_FUNCTION(Mysqlx, get_session) {
//   auto session = connect(args, mysqlsh::SessionType::X);
//   return
//   shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(session));
// }

REGISTER_HELP(
    MYSQLX_GETSESSION_BRIEF,
    "Creates a Session instance using the provided connection data.");
REGISTER_HELP(MYSQLX_GETSESSION_PARAM,
              "@param connectionData The connection data for the session");
REGISTER_HELP(MYSQLX_GETSESSION_PARAM1,
              "@param password Optional password for the session");
REGISTER_HELP(MYSQLX_GETSESSION_RETURNS, "@returns A Session");
REGISTER_HELP(
    MYSQLX_GETSESSION_DETAIL,
    "A Session object uses the X Protocol to allow executing operations on the "
    "connected MySQL Server.");
REGISTER_HELP(MYSQLX_GETSESSION_DETAIL1, "TOPIC_CONNECTION_DATA");


/**
 * \ingroup mysqlx
 * $(MYSQLX_GETSESSION)
 *
 * $(MYSQLX_GETSESSION_PARAM)
 * $(MYSQLX_GETSESSION_PARAM1)
 *
 * $(MYSQLX_GETSESSION_RETURNS)
 *
 * $(MYSQLX_GETSESSION_DETAIL)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref connection_data
 *
 */

#if DOXYGEN_JS
Session getSession(ConnectionData connectionData, String password) {}
#elif DOXYGEN_PY
Session get_session(ConnectionData connectionData, str password) {}
#endif
DEFINE_FUNCTION(Mysqlx, get_session) {
  return shcore::Value(Session::create(args));
}

REGISTER_HELP(MYSQLX_EXPR_BRIEF,
              "Creates an Expression object based on a string.");
REGISTER_HELP(
    MYSQLX_EXPR_PARAM,
    "@param expressionStr The expression to be represented by the object");
REGISTER_HELP(
    MYSQLX_EXPR_DETAIL,
    "An expression object is required in many operations on the X DevAPI.");
REGISTER_HELP(MYSQLX_EXPR_DETAIL1,
              "Some applications of the expression objects include:");
REGISTER_HELP(MYSQLX_EXPR_DETAIL2,
              "@li Creation of documents based on a JSON string");
REGISTER_HELP(
    MYSQLX_EXPR_DETAIL3,
    "@li Defining calculated fields when inserting data on the database");
REGISTER_HELP(
    MYSQLX_EXPR_DETAIL4,
    "@li Defining calculated fields when pulling data from the database");
/**
 * \ingroup mysqlx
 * $(MYSQLX_EXPR_BRIEF)
 *
 * $(MYSQLX_EXPR_PARAM)
 *
 * $(MYSQLX_EXPR_DETAIL)
 *
 * $(MYSQLX_EXPR_DETAIL1)
 * $(MYSQLX_EXPR_DETAIL2)
 * $(MYSQLX_EXPR_DETAIL3)
 * $(MYSQLX_EXPR_DETAIL4)
 */
#if DOXYGEN_JS
Expression expr(String expressionStr) {}
#elif DOXYGEN_PY
Expression expr(str expressionStr) {}
#endif
DEFINE_FUNCTION(Mysqlx, expr) {
  return shcore::Value(Expression::create(args));
}

DEFINE_FUNCTION(Mysqlx, date_value) {
  args.ensure_count(3, 7, get_function_name("dateValue").c_str());
  shcore::Value ret_val;
  try {
    ret_val = shcore::Value(shcore::Date::create(args));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("dateValue"));
  return ret_val;
}
}  // namespace mysqlx
}  // namespace mysqlsh
