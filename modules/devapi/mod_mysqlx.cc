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
REGISTER_HELP_TOPIC(X DevAPI, CATEGORY, devapi, Contents, SCRIPTING);
REGISTER_HELP(DEVAPI_BRIEF,
              "Details the <b>mysqlx</b> module as well as the capabilities of "
              "the X DevAPI which enable working with MySQL as a Document "
              "Store");
REGISTER_HELP(DEVAPI_DETAIL,
              "The X DevAPI allows working with MySQL databases through a "
              "modern and fluent API. It specially simplifies using MySQL as a "
              "Document Store.");
REGISTER_HELP(DEVAPI_DETAIL1,
              "The variety of operations available through this API includes:");
REGISTER_HELP(
    DEVAPI_DETAIL2,
    "@li Creation of a session to an X Protocol enabled MySQL Server.");
REGISTER_HELP(DEVAPI_DETAIL3, "@li Schema management.");
REGISTER_HELP(DEVAPI_DETAIL4, "@li Collection management.");
REGISTER_HELP(DEVAPI_DETAIL5, "@li CRUD operations on Collections and Tables.");
REGISTER_HELP(DEVAPI_DETAIL6,
              "The X DevAPI is a collection of functions and classes contained "
              "on the <b>mysqlx</b> module which is automatically loaded when "
              "the shell starts.");
REGISTER_HELP(DEVAPI_DETAIL7,
              "To work on a MySQL Server with the X DevAPI, start by creating "
              "a session using: <b>mysqlx.<<<createSession>>>(...)</b>.");
REGISTER_HELP(
    DEVAPI_DETAIL8,
    "For more details about the <b>mysqlx</b> module use: <b>\\? mysqlx</b>");
REGISTER_HELP(DEVAPI_DETAIL9,
              "For more details about how to create a session with the X "
              "DevAPI use: <b>\\? mysqlx</b><<<createSession>>>");
REGISTER_HELP(DEVAPI_MODULES_DESC, "IGNORE");

REGISTER_HELP_MODULE(mysqlx, devapi);
REGISTER_HELP(
    MYSQLX_GLOBAL_BRIEF,
    "Used to work with X Protocol sessions using the MySQL X DevAPI.");
REGISTER_HELP(MYSQLX_BRIEF,
              "Encloses the functions and classes available to interact with "
              "an X Protocol enabled MySQL Product.");
REGISTER_HELP(MYSQLX_DETAIL,
              "The objects contained on this module provide a full API to "
              "interact with the different MySQL Products "
              "implementing the X Protocol.");
REGISTER_HELP(MYSQLX_DETAIL1,
              "In the case of a MySQL Server the API will enable doing "
              "operations on the different database objects "
              "such as schema management operations and both table and "
              "collection management and CRUD operations. "
              "(CRUD: Create, Read, Update, Delete).");
REGISTER_HELP(MYSQLX_DETAIL2,
              "Intention of the module is to provide a full API for "
              "development through scripting languages such as "
              "JavaScript and Python, this would be normally achieved through "
              "a normal session.");
REGISTER_HELP(MYSQLX_DETAIL3,
              "To use the properties and functions available on this module "
              "you first need to import it.");
REGISTER_HELP(MYSQLX_DETAIL4,
              "When running the shell in interactive mode, this module is "
              "automatically imported.");
REGISTER_HELP(MYSQLX_OBJECTS_TITLE, "CONSTANTS");
REGISTER_HELP_OBJECT(Type, mysqlx);
REGISTER_HELP(TYPE_BRIEF, "Data type constants.");
REGISTER_HELP(TYPE_DETAIL,
              "The data type constants assigned to a Column object "
              "retrieved through RowResult.<b><<<getColumns>>></b>().");

REGISTER_HELP_PROPERTY(BIT, Type);
REGISTER_HELP(TYPE_BIT_BRIEF, "A bit-value type.");

REGISTER_HELP_PROPERTY(TINYINT, Type);
REGISTER_HELP(TYPE_TINYINT_BRIEF, "A very small integer.");

REGISTER_HELP_PROPERTY(SMALLINT, Type);
REGISTER_HELP(TYPE_SMALLINT_BRIEF, "A small integer.");

REGISTER_HELP_PROPERTY(MEDIUMINT, Type);
REGISTER_HELP(TYPE_MEDIUMINT_BRIEF, "A medium-sized integer.");

REGISTER_HELP_PROPERTY(INT, Type);
REGISTER_HELP(TYPE_INT_BRIEF, "A normal-size integer.");

REGISTER_HELP_PROPERTY(BIGINT, Type);
REGISTER_HELP(TYPE_BIGINT_BRIEF, "A large integer.");

REGISTER_HELP_PROPERTY(FLOAT, Type);
REGISTER_HELP(TYPE_FLOAT_BRIEF, "A floating-point number.");

REGISTER_HELP_PROPERTY(DECIMAL, Type);
REGISTER_HELP(TYPE_DECIMAL_BRIEF, "A packed \"exact\" fixed-point number.");

REGISTER_HELP_PROPERTY(JSON, Type);
REGISTER_HELP(TYPE_JSON_BRIEF, "A JSON-format string.");

REGISTER_HELP_PROPERTY(STRING, Type);
REGISTER_HELP(TYPE_STRING_BRIEF, "A character string.");

REGISTER_HELP_PROPERTY(BYTES, Type);
REGISTER_HELP(TYPE_BYTES_BRIEF, "A binary string.");

REGISTER_HELP_PROPERTY(TIME, Type);
REGISTER_HELP(TYPE_TIME_BRIEF, "A time.");

REGISTER_HELP_PROPERTY(DATE, Type);
REGISTER_HELP(TYPE_DATE_BRIEF, "A date.");

REGISTER_HELP_PROPERTY(DATETIME, Type);
REGISTER_HELP(TYPE_DATETIME_BRIEF, "A date and time combination.");

REGISTER_HELP_PROPERTY(SET, Type);
REGISTER_HELP(TYPE_SET_BRIEF, "A set.");

REGISTER_HELP_PROPERTY(ENUM, Type);
REGISTER_HELP(TYPE_ENUM_BRIEF, "An enumeration.");

REGISTER_HELP_PROPERTY(GEOMETRY, Type);
REGISTER_HELP(TYPE_GEOMETRY_BRIEF, "A geometry type.");

REGISTER_HELP_OBJECT(LockContention, mysqlx);
REGISTER_HELP(LOCKCONTENTION_BRIEF, "Row locking mode constants.");
REGISTER_HELP(LOCKCONTENTION_DETAIL,
              "These constants are used to indicate the locking mode to be "
              "used at the <b><<<lockShared>>></b> and "
              "<b><<<lockExclusive>>></b> functions of the TableSelect and "
              "CollectionFind objects.");

REGISTER_HELP_PROPERTY(DEFAULT, LockContention);
REGISTER_HELP(LOCKCONTENTION_DEFAULT_BRIEF, "A default locking mode.");

REGISTER_HELP_PROPERTY(NOWAIT, LockContention);
REGISTER_HELP(LOCKCONTENTION_NOWAIT_BRIEF,
              "A locking read never waits to acquire a row lock. The query "
              "executes immediately, failing with an error if a requested row "
              "is locked.");

REGISTER_HELP_PROPERTY(SKIP_LOCKED, LockContention);
REGISTER_HELP(LOCKCONTENTION_SKIP_LOCKED_BRIEF,
              "A locking read never waits to acquire a row lock. The query "
              "executes immediately, removing locked rows from the result "
              "set.");

REGISTER_MODULE(Mysqlx, mysqlx) {
  add_property("Type|Type");
  add_property("LockContention|LockContention");
  REGISTER_VARARGS_FUNCTION(Mysqlx, get_session, getSession);
  REGISTER_VARARGS_FUNCTION(Mysqlx, date_value, dateValue);
  REGISTER_FUNCTION(Mysqlx, expr, expr, "expression", shcore::String);

  _type.reset(new Type());
  _lock_contention.reset(new LockContention());
}

// We need to hide this from doxygen to avoif warnings
#if !defined DOXYGEN_JS && !defined DOXYGEN_PY
shcore::Value Mysqlx::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "Type")
    ret_val = shcore::Value(_type);
  else if (prop == "LockContention")
    ret_val = shcore::Value(_lock_contention);
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
REGISTER_HELP_FUNCTION(getSession, mysqlx);
REGISTER_HELP(MYSQLX_GETSESSION_BRIEF,
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
REGISTER_HELP(MYSQLX_GETSESSION_DETAIL1, "${TOPIC_CONNECTION_DATA}");

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
 * Detailed description of the connection data format is available at \ref
 * connection_data
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

REGISTER_HELP_FUNCTION(expr, mysqlx);
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

REGISTER_HELP_FUNCTION(dateValue, mysqlx);
REGISTER_HELP(MYSQLX_DATEVALUE_BRIEF,
              "Creates a <b>Date</b> object which represents a date time.");
REGISTER_HELP(MYSQLX_DATEVALUE_PARAM,
              "@param year The year to be used in the new Date object.");
REGISTER_HELP(MYSQLX_DATEVALUE_PARAM1,
              "@param month The month to be used in the new Date object.");
REGISTER_HELP(MYSQLX_DATEVALUE_PARAM2,
              "@param day The month to be used in the new Date object.");
REGISTER_HELP(MYSQLX_DATEVALUE_PARAM3,
              "@param hour Optional hour to be used in the new Date object.");
REGISTER_HELP(
    MYSQLX_DATEVALUE_PARAM4,
    "@param minutes optional minutes to be used in the new Date object.");
REGISTER_HELP(
    MYSQLX_DATEVALUE_PARAM5,
    "@param seconds optional seconds to be used in the new Date object.");
REGISTER_HELP(MYSQLX_DATEVALUE_PARAM6,
              "@param milliseconds optional milliseconds to be used in the new "
              "Date object.");
REGISTER_HELP(MYSQLX_DATEVALUE_SIGNATURE,
              "(year, month, day[, hour, day, minute[, milliseconds]])");
REGISTER_HELP(MYSQLX_DATEVALUE_DETAIL,
              "This function creates a Date object containing:");
REGISTER_HELP(MYSQLX_DATEVALUE_DETAIL1, "@li A date value.");
REGISTER_HELP(MYSQLX_DATEVALUE_DETAIL2, "@li A date and time value.");
REGISTER_HELP(MYSQLX_DATEVALUE_DETAIL3,
              "@li A date and time value with milliseconds.");

/**
 * \ingroup mysqlx
 * $(MYSQLX_DATEVALUE_BRIEF)
 *
 * $(MYSQLX_DATEVALUE_PARAM)
 * $(MYSQLX_DATEVALUE_PARAM1)
 * $(MYSQLX_DATEVALUE_PARAM2)
 * $(MYSQLX_DATEVALUE_PARAM3)
 * $(MYSQLX_DATEVALUE_PARAM4)
 * $(MYSQLX_DATEVALUE_PARAM5)
 * $(MYSQLX_DATEVALUE_PARAM6)
 *
 * $(MYSQLX_DATEVALUE_DETAIL)
 */
#if DOXYGEN_JS
String dateValue(Integer year, Integer month, Integer day, Integer hour,
                 Integer minutes, Integer seconds, Integer milliseconds) {}
#elif DOXYGEN_PY
String date_value(int year, int month, int day, int hour, int minutes,
                  int seconds, int milliseconds) {}
#endif
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
