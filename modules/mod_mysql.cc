/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysql.h"
#include "mod_mysql_session.h"
#include "shellcore/utils_help.h"

using namespace std::placeholders;
namespace mysqlsh {
namespace mysql {

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


REGISTER_MODULE(Mysql, mysql) {
  REGISTER_VARARGS_FUNCTION(Mysql, get_classic_session, getClassicSession);
}

REGISTER_HELP(MYSQL_GETCLASSICSESSION_BRIEF, "Creates a ClassicSession instance using the provided connection data.");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_PARAM,  "@param connectionData The connection data for the session");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_PARAM1, "@param password Optional password for the session");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_RETURNS, "@returns A ClassicSession");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_DETAIL, "A ClassicSession object uses the traditional MySQL Protocol to allow executing operations on the "\
                                              "connected MySQL Server.");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_DETAIL1, "TOPIC_CONNECTION_DATA");

/**
 * \ingroup mysql
 * $(MYSQL_GETCLASSICSESSION)
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
  return shcore::Value(ClassicSession::create(args));
}

}
}
