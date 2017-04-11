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

#include "mod_mysqlx.h"
#include "mod_mysqlx_session.h"
#include "mod_mysqlx_expression.h"
#include "mod_mysqlx_constants.h"
#include "scripting/obj_date.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
namespace mysqlsh {
namespace mysqlx {

REGISTER_HELP(MYSQLX_INTERACTIVE_BRIEF, "Used to work with X Protocol sessions using the MySQL X DevAPI.");
REGISTER_HELP(MYSQLX_BRIEF,  "Encloses the functions and classes available to interact with an X Protocol enabled MySQL Product.");
REGISTER_HELP(MYSQLX_DETAIL, "The objects contained on this module provide a full API to interact with the different MySQL Products "\
                             "implementing the X Protocol.");
REGISTER_HELP(MYSQLX_DETAIL1,"In the case of a MySQL Server the API will enable doing operations on the different database objects "\
                             "such as schema management operations and both table and collection management and CRUD operations. "\
                             "(CRUD: Create, Read, Update, Delete).");
REGISTER_HELP(MYSQLX_DETAIL2,"Intention of the module is to provide a full API for development through scripting languages such as "\
                             "JavaScript and Python, this would be normally achieved through a normal session.");
REGISTER_HELP(MYSQLX_DETAIL3,"To use the properties and functions available on this module you first need to import it.");
REGISTER_HELP(MYSQLX_DETAIL4,"When running the shell in interactive mode, this module is automatically imported.");


REGISTER_MODULE(Mysqlx, mysqlx) {
  add_property("Type|Type");
  add_property("IndexType|IndexType");
  //REGISTER_VARARGS_FUNCTION(Mysqlx, get_session, getSession);
  REGISTER_VARARGS_FUNCTION(Mysqlx, get_node_session, getNodeSession);
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
//   auto session = connect_session(args, mysqlsh::SessionType::X);
//   return shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(session));
// }

REGISTER_HELP(MYSQLX_GETNODESESSION_BRIEF, "Creates a NodeSession instance using the provided connection data.");
REGISTER_HELP(MYSQLX_GETNODESESSION_PARAM,  "@param connectionData The connection data for the session");
REGISTER_HELP(MYSQLX_GETNODESESSION_PARAM1, "@param password Optional password for the session");
REGISTER_HELP(MYSQLX_GETNODESESSION_RETURNS, "@returns A NodeSession");
REGISTER_HELP(MYSQLX_GETNODESESSION_DETAIL, "A NodeSession object uses the traditional MySQL Protocol to allow executing operations on the "\
"connected MySQL Server.");
REGISTER_HELP(MYSQLX_GETNODESESSION_DETAIL1,"The connection data can be any of:");
REGISTER_HELP(MYSQLX_GETNODESESSION_DETAIL2,"@li A URI string");
REGISTER_HELP(MYSQLX_GETNODESESSION_DETAIL3,"@li A Dictionary with the connection options");

/**
 * $(MYSQLX_GETNODESESSION)
 *
 * $(MYSQLX_GETNODESESSION_PARAM)
 * $(MYSQLX_GETNODESESSION_PARAM1)
 *
 * $(MYSQLX_GETNODESESSION_RETURNS)
 *
 * $(MYSQLX_GETNODESESSION_DETAIL)
 *
 * $(MYSQLX_GETNODESESSION_DETAIL1)
 * $(MYSQLX_GETNODESESSION_DETAIL2)
 * $(MYSQLX_GETNODESESSION_DETAIL3)
 */

#if DOXYGEN_JS
NodeSession getNodeSession(ConnectionData connectionData, String password){}
#elif DOXYGEN_PY
NodeSession get_node_session(ConnectionData connectionData, str password){}
#endif
DEFINE_FUNCTION(Mysqlx, get_node_session) {
  return shcore::Value(NodeSession::create(args));
}


REGISTER_HELP(MYSQLX_EXPR_BRIEF, "Creates an Expression object based on a string.");
REGISTER_HELP(MYSQLX_EXPR_PARAM,  "@param expressionStr The expression to be represented by the object");
REGISTER_HELP(MYSQLX_EXPR_DETAIL, "An expression object is required in many operations on the X DevAPI.");
REGISTER_HELP(MYSQLX_EXPR_DETAIL1,"Some applications of the expression objects include:");
REGISTER_HELP(MYSQLX_EXPR_DETAIL2,"@li Creation of documents based on a JSON string");
REGISTER_HELP(MYSQLX_EXPR_DETAIL3,"@li Defining calculated fields when inserting data on the database");
REGISTER_HELP(MYSQLX_EXPR_DETAIL4,"@li Defining calculated fields when pulling data from the database");
/**
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
Expression expr(String expressionStr){}
#elif DOXYGEN_PY
Expression expr(str expressionStr){}
#endif
DEFINE_FUNCTION(Mysqlx, expr) {
  return shcore::Value(Expression::create(args));
}

DEFINE_FUNCTION(Mysqlx, date_value) {
  return shcore::Value(shcore::Date::create(args));
}

}
}