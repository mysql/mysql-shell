/*
 * Copyright (c) 2016, 2022, Oracle and/or its affiliates.
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

#include "modules/mod_mysql.h"

#include <memory>
#include <utility>
#include <vector>
#include "modules/mod_mysql_constants.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/utils/utils.h"
#include "mysqlshdk/libs/parser/mysql_parser_utils.h"
#include "mysqlshdk/libs/utils/utils_mysql_parsing.h"

#include "errmsg.h"  // NOLINT
namespace mysqlsh {
namespace mysql {
struct Mysqld_ername {
  const char *name;
  int code;
  const char *text;
  const char *sqlstate;
  const char *dummy1;
  int dummy2;
};

#if !defined(DOXYGEN_JS) && !defined(DOXYGEN_PY)
static const Mysqld_ername k_error_names[] = {
#include "mysqld_ername.h"  // NOLINT
};
#endif

struct Mysqlclient_ername {
  const char *name;
  int code;
};

static const Mysqlclient_ername k_client_error_names[] = {
    {"CR_MIN_ERROR", CR_MIN_ERROR},
    {"CR_UNKNOWN_ERROR", CR_UNKNOWN_ERROR},
    {"CR_SOCKET_CREATE_ERROR", CR_SOCKET_CREATE_ERROR},
    {"CR_CONNECTION_ERROR", CR_CONNECTION_ERROR},
    {"CR_CONN_HOST_ERROR", CR_CONN_HOST_ERROR},
    {"CR_IPSOCK_ERROR", CR_IPSOCK_ERROR},
    {"CR_UNKNOWN_HOST", CR_UNKNOWN_HOST},
    {"CR_SERVER_GONE_ERROR", CR_SERVER_GONE_ERROR},
    {"CR_VERSION_ERROR", CR_VERSION_ERROR},
    {"CR_OUT_OF_MEMORY", CR_OUT_OF_MEMORY},
    {"CR_WRONG_HOST_INFO", CR_WRONG_HOST_INFO},
    {"CR_LOCALHOST_CONNECTION", CR_LOCALHOST_CONNECTION},
    {"CR_TCP_CONNECTION", CR_TCP_CONNECTION},
    {"CR_SERVER_HANDSHAKE_ERR", CR_SERVER_HANDSHAKE_ERR},
    {"CR_SERVER_LOST", CR_SERVER_LOST},
    {"CR_COMMANDS_OUT_OF_SYNC", CR_COMMANDS_OUT_OF_SYNC},
    {"CR_NAMEDPIPE_CONNECTION", CR_NAMEDPIPE_CONNECTION},
    {"CR_NAMEDPIPEWAIT_ERROR", CR_NAMEDPIPEWAIT_ERROR},
    {"CR_NAMEDPIPEOPEN_ERROR", CR_NAMEDPIPEOPEN_ERROR},
    {"CR_NAMEDPIPESETSTATE_ERROR", CR_NAMEDPIPESETSTATE_ERROR},
    {"CR_CANT_READ_CHARSET", CR_CANT_READ_CHARSET},
    {"CR_NET_PACKET_TOO_LARGE", CR_NET_PACKET_TOO_LARGE},
    {"CR_EMBEDDED_CONNECTION", CR_EMBEDDED_CONNECTION},
    {"CR_PROBE_SLAVE_STATUS", CR_PROBE_SLAVE_STATUS},
    {"CR_PROBE_SLAVE_HOSTS", CR_PROBE_SLAVE_HOSTS},
    {"CR_PROBE_SLAVE_CONNECT", CR_PROBE_SLAVE_CONNECT},
    {"CR_PROBE_MASTER_CONNECT", CR_PROBE_MASTER_CONNECT},
    {"CR_SSL_CONNECTION_ERROR", CR_SSL_CONNECTION_ERROR},
    {"CR_MALFORMED_PACKET", CR_MALFORMED_PACKET},
    {"CR_WRONG_LICENSE", CR_WRONG_LICENSE},
    {"CR_NULL_POINTER", CR_NULL_POINTER},
    {"CR_NO_PREPARE_STMT", CR_NO_PREPARE_STMT},
    {"CR_PARAMS_NOT_BOUND", CR_PARAMS_NOT_BOUND},
    {"CR_DATA_TRUNCATED", CR_DATA_TRUNCATED},
    {"CR_NO_PARAMETERS_EXISTS", CR_NO_PARAMETERS_EXISTS},
    {"CR_INVALID_PARAMETER_NO", CR_INVALID_PARAMETER_NO},
    {"CR_INVALID_BUFFER_USE", CR_INVALID_BUFFER_USE},
    {"CR_UNSUPPORTED_PARAM_TYPE", CR_UNSUPPORTED_PARAM_TYPE},
    {"CR_SHARED_MEMORY_CONNECTION", CR_SHARED_MEMORY_CONNECTION},
    {"CR_SHARED_MEMORY_CONNECT_REQUEST_ERROR",
     CR_SHARED_MEMORY_CONNECT_REQUEST_ERROR},
    {"CR_SHARED_MEMORY_CONNECT_ANSWER_ERROR",
     CR_SHARED_MEMORY_CONNECT_ANSWER_ERROR},
    {"CR_SHARED_MEMORY_CONNECT_FILE_MAP_ERROR",
     CR_SHARED_MEMORY_CONNECT_FILE_MAP_ERROR},
    {"CR_SHARED_MEMORY_CONNECT_MAP_ERROR", CR_SHARED_MEMORY_CONNECT_MAP_ERROR},
    {"CR_SHARED_MEMORY_FILE_MAP_ERROR", CR_SHARED_MEMORY_FILE_MAP_ERROR},
    {"CR_SHARED_MEMORY_MAP_ERROR", CR_SHARED_MEMORY_MAP_ERROR},
    {"CR_SHARED_MEMORY_EVENT_ERROR", CR_SHARED_MEMORY_EVENT_ERROR},
    {"CR_SHARED_MEMORY_CONNECT_ABANDONED_ERROR",
     CR_SHARED_MEMORY_CONNECT_ABANDONED_ERROR},
    {"CR_SHARED_MEMORY_CONNECT_SET_ERROR", CR_SHARED_MEMORY_CONNECT_SET_ERROR},
    {"CR_CONN_UNKNOW_PROTOCOL", CR_CONN_UNKNOW_PROTOCOL},
    {"CR_INVALID_CONN_HANDLE", CR_INVALID_CONN_HANDLE},
    {"CR_UNUSED_1", CR_UNUSED_1},
    {"CR_FETCH_CANCELED", CR_FETCH_CANCELED},
    {"CR_NO_DATA", CR_NO_DATA},
    {"CR_NO_STMT_METADATA", CR_NO_STMT_METADATA},
    {"CR_NO_RESULT_SET", CR_NO_RESULT_SET},
    {"CR_NOT_IMPLEMENTED", CR_NOT_IMPLEMENTED},
    {"CR_SERVER_LOST_EXTENDED", CR_SERVER_LOST_EXTENDED},
    {"CR_STMT_CLOSED", CR_STMT_CLOSED},
    {"CR_NEW_STMT_METADATA", CR_NEW_STMT_METADATA},
    {"CR_ALREADY_CONNECTED", CR_ALREADY_CONNECTED},
    {"CR_AUTH_PLUGIN_CANNOT_LOAD", CR_AUTH_PLUGIN_CANNOT_LOAD},
    {"CR_DUPLICATE_CONNECTION_ATTR", CR_DUPLICATE_CONNECTION_ATTR},
    {"CR_AUTH_PLUGIN_ERR", CR_AUTH_PLUGIN_ERR},
    {"CR_INSECURE_API_ERR", CR_INSECURE_API_ERR},
    {"CR_FILE_NAME_TOO_LONG", CR_FILE_NAME_TOO_LONG},
    {"CR_SSL_FIPS_MODE_ERR", CR_SSL_FIPS_MODE_ERR},
    {"CR_DEPRECATED_COMPRESSION_NOT_SUPPORTED",
     CR_DEPRECATED_COMPRESSION_NOT_SUPPORTED},
    {"CR_COMPRESSION_WRONGLY_CONFIGURED", CR_COMPRESSION_WRONGLY_CONFIGURED},
    {"CR_KERBEROS_USER_NOT_FOUND", CR_KERBEROS_USER_NOT_FOUND},
    {"CR_LOAD_DATA_LOCAL_INFILE_REJECTED", CR_LOAD_DATA_LOCAL_INFILE_REJECTED},
    {"CR_LOAD_DATA_LOCAL_INFILE_REALPATH_FAIL",
     CR_LOAD_DATA_LOCAL_INFILE_REALPATH_FAIL},
    {"CR_DNS_SRV_LOOKUP_FAILED", CR_DNS_SRV_LOOKUP_FAILED},
    {"CR_MANDATORY_TRACKER_NOT_FOUND", CR_MANDATORY_TRACKER_NOT_FOUND},
    {"CR_INVALID_FACTOR_NO", CR_INVALID_FACTOR_NO},
    {"CR_CANT_GET_SESSION_DATA", CR_CANT_GET_SESSION_DATA},
    {"CR_ERROR_LAST", CR_ERROR_LAST},
    {"CR_MAX_ERROR", CR_MAX_ERROR}};

// If this assert fails, the list of client error names has to be updated
static_assert(CR_ERROR_LAST <= 2073);

static shcore::Value g_error_code;

REGISTER_HELP_MODULE(mysql, shellapi);
REGISTER_HELP(MYSQL_GLOBAL_BRIEF,
              "Support for connecting to MySQL servers using the classic MySQL "
              "protocol.");
REGISTER_HELP(MYSQL_BRIEF,
              "Encloses the functions and classes available to interact with a "
              "MySQL Server using the traditional "
              "MySQL Protocol.");

REGISTER_HELP(MYSQL_DETAIL,
              "Use this module to create a session using the traditional MySQL "
              "Protocol, for example for MySQL Servers where "
              "the X Protocol is not available.");
REGISTER_HELP(MYSQL_DETAIL1,
              "Note that the API interface on this module is very limited, "
              "even you can load schemas, tables and views as "
              "objects there are no operations available on them.");
REGISTER_HELP(MYSQL_DETAIL2,
              "The purpose of this module is to allow SQL Execution on MySQL "
              "Servers where the X Protocol is not enabled.");
REGISTER_HELP(MYSQL_DETAIL3,
              "To use the properties and functions available on this module "
              "you first need to import it.");
REGISTER_HELP(MYSQL_DETAIL4,
              "When running the shell in interactive mode, this module is "
              "automatically imported.");

REGISTER_HELP_CONSTANTS(ErrorCode, mysql);
REGISTER_HELP(ERRORCODE_BRIEF, "MySQL server error codes.");
REGISTER_HELP(ERRORCODE_DETAIL,
              "Constants with the error code values that can be received from "
              "the MySQL server.");

REGISTER_MODULE(Mysql, mysql) {
  add_property("ErrorCode|ErrorCode");

  expose("getClassicSession", &Mysql::get_session, "connectionData",
         "?password");
  expose("getSession", &Mysql::get_session, "connectionData", "?password");

  expose("splitScript", &Mysql::split_script, "script");
  expose("parseStatementAst", &Mysql::parse_statement_ast, "statement");

  expose("quoteIdentifier", &Mysql::quote_identifier, "string");
  expose("unquoteIdentifier", &Mysql::unquote_identifier, "string");

  _type.reset(new Type());
}

// We need to hide this from doxygen to avoid warnings
#if !defined(DOXYGEN_JS) && !defined(DOXYGEN_PY)
shcore::Value Mysql::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "Type") {
    ret_val = shcore::Value(_type);
  } else if (prop == "ErrorCode") {
    if (!g_error_code) {
      auto dict = shcore::make_dict();

      for (const auto &e : k_client_error_names) {
        dict->set(e.name, shcore::Value(e.code));
      }

      for (const auto &e : k_error_names) {
        dict->set(e.name, shcore::Value(e.code));
      }
      g_error_code = shcore::Value(dict);
    }

    ret_val = g_error_code;
  } else {
    ret_val = Cpp_object_bridge::get_member(prop);
  }

  return ret_val;
}
#endif

REGISTER_HELP_FUNCTION(getClassicSession, mysql);
REGISTER_HELP(MYSQL_GETCLASSICSESSION_BRIEF,
              "Opens a classic MySQL protocol session to a MySQL server.");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_PARAM,
              "@param connectionData The connection data for the session");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_PARAM1,
              "@param password Optional password for the session");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_RETURNS, "@returns A ClassicSession");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_DETAIL,
              "A ClassicSession object uses the traditional MySQL Protocol to "
              "allow executing operations on the "
              "connected MySQL Server.");
REGISTER_HELP(MYSQL_GETCLASSICSESSION_DETAIL1, "${TOPIC_CONNECTION_DATA}");

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
 * Detailed description of the connection data format is available at \ref
 * connection_data
 *
 */

#if DOXYGEN_JS
ClassicSession getClassicSession(ConnectionData connectionData,
                                 String password) {}
#elif DOXYGEN_PY
ClassicSession get_classic_session(ConnectionData connectionData,
                                   str password) {}
#endif

REGISTER_HELP_FUNCTION(getSession, mysql);
REGISTER_HELP(MYSQL_GETSESSION_BRIEF,
              "Opens a classic MySQL protocol session to a MySQL server.");
REGISTER_HELP(MYSQL_GETSESSION_PARAM,
              "@param connectionData The connection data for the session");
REGISTER_HELP(MYSQL_GETSESSION_PARAM1,
              "@param password Optional password for the session");
REGISTER_HELP(MYSQL_GETSESSION_RETURNS, "@returns A ClassicSession");
REGISTER_HELP(MYSQL_GETSESSION_DETAIL,
              "A ClassicSession object uses the traditional MySQL Protocol to "
              "allow executing operations on the "
              "connected MySQL Server.");
REGISTER_HELP(MYSQL_GETSESSION_DETAIL1, "${TOPIC_CONNECTION_DATA}");
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
 * Detailed description of the connection data format is available at \ref
 * connection_data
 *
 */
#if DOXYGEN_JS
ClassicSession getSession(ConnectionData connectionData, String password) {}
#elif DOXYGEN_PY
ClassicSession get_session(ConnectionData connectionData, str password) {}
#endif
#if !defined(DOXYGEN_JS) && !defined(DOXYGEN_PY)
std::shared_ptr<shcore::Object_bridge> Mysql::get_session(
    const mysqlshdk::db::Connection_options &co_, const char *password) {
  auto co = co_;
  set_password_from_string(&co, password);

  // Sessions through the classic protocol will use the plugin dir from the
  // shell options if not included on the connection options already
  co.set_plugins_dir();

  return ClassicSession::create(co);
}
#endif

REGISTER_HELP_FUNCTION(splitScript, mysql);
REGISTER_HELP_FUNCTION_TEXT(MYSQL_SPLITSCRIPT, R"*(
Split a SQL script into individual statements.

@param script A SQL script as a string containing multiple statements

@returns A list of statements

The default statement delimiter is `;` but it can be changed with the
DELIMITER keyword, which must be followed by the delimiter character(s)
and a newline.
)*");

/**
 * \ingroup mysql
 * $(MYSQL_SPLITSCRIPT_BRIEF)
 *
 * $(MYSQL_SPLITSCRIPT)
 */
#if DOXYGEN_JS
Array splitScript(String script) {}
#elif DOXYGEN_PY
list split_script(str script) {}
#endif
shcore::Value Mysql::split_script(const std::string &script) const {
  shcore::Array_t array = shcore::make_array();
  for (auto s : mysqlshdk::utils::split_sql(script))
    array->emplace_back(std::move(s));
  return shcore::Value(array);
}

REGISTER_HELP_FUNCTION(parseStatementAst, mysql);
REGISTER_HELP_FUNCTION_TEXT(MYSQL_PARSESTATEMENTAST, R"*(
Parse MySQL statements and return its AST representation.

@param sql SQL statements to be parsed

@returns AST encoded as a JSON structure
)*");

/**
 * \ingroup mysql
 * $(MYSQL_PARSESTATEMENTAST_BRIEF)
 *
 * $(MYSQL_PARSESTATEMENTAST)
 */
#if DOXYGEN_JS
Dictionary parseStatementAst(String statements) {}
#elif DOXYGEN_PY
dict parse_statement_ast(str statements) {}
#endif
shcore::Value Mysql::parse_statement_ast(const std::string &sql) const {
  auto array = shcore::make_array();
  int errors = 0;
  shcore::Value root =
      shcore::Value(shcore::make_dict("children", shcore::Value(array)));

  mysqlshdk::parser::traverse_script_ast<shcore::Value>(
      sql, {}, false, false, &root, [](const std::string &, size_t, bool) {},
      [](const mysqlshdk::parser::AST_rule_node &node, bool enter,
         shcore::Value *parent) -> shcore::Value * {
        if (enter) {
          auto parent_list = parent->as_map()->get_array("children");
          parent_list->emplace_back(shcore::make_dict(
              "rule", node.name, "children", shcore::make_array()));
          return &parent_list->back();
        }
        return nullptr;
      },
      [](const mysqlshdk::parser::AST_terminal_node &node,
         shcore::Value *parent) {
        auto parent_list = parent->as_map()->get_array("children");

        parent_list->emplace_back(
            shcore::make_dict("symbol", node.name, "text", node.text));

        return &parent_list->back();
      },
      [&errors](const mysqlshdk::parser::AST_error_node &node,
                shcore::Value *parent) {
        auto parent_list = parent->as_map()->get_array("children");

        ++errors;
        parent_list->emplace_back(shcore::make_dict(
            "symbol", node.name, "text", node.text, "line", node.line, "offset",
            node.offset, "error", shcore::Value(1)));

        return &parent_list->back();
      });

  // this is only supposed to return 1 node, so remove the parent
  assert(array->size() <= 1);
  if (array->size() == 0) return {};

  return array->at(0);
}

REGISTER_HELP_FUNCTION(quoteIdentifier, mysql);
REGISTER_HELP_FUNCTION_TEXT(MYSQL_QUOTEIDENTIFIER, R"*(
Quote a string as a MySQL identifier, escaping characters when needed.

@param s the identifier name to be quoted

@returns Quoted identifier
)*");

/**
 * \ingroup mysql
 * $(MYSQL_QUOTEIDENTIFIER_BRIEF)
 *
 * $(MYSQL_QUOTEIDENTIFIER)
 */
#if DOXYGEN_JS
String quoteIdentifier(String s) {}
#elif DOXYGEN_PY
str quote_identifier(str s) {}
#endif
std::string Mysql::quote_identifier(const std::string &s) const {
  return shcore::quote_identifier(s);
}

REGISTER_HELP_FUNCTION(unquoteIdentifier, mysql);
REGISTER_HELP_FUNCTION_TEXT(MYSQL_UNQUOTEIDENTIFIER, R"*(
Unquote a MySQL identifier.

@param s String to unquote

@returns Unquoted string.

An exception is thrown if the input string is not quoted with backticks.
)*");

/**
 * \ingroup mysql
 * $(MYSQL_UNQUOTEIDENTIFIER_BRIEF)
 *
 * $(MYSQL_UNQUOTEIDENTIFIER)
 */
#if DOXYGEN_JS
String unquoteIdentifier(String s) {}
#elif DOXYGEN_PY
str unquote_identifier(str s) {}
#endif
std::string Mysql::unquote_identifier(const std::string &s) const {
  return shcore::unquote_identifier(s);
}

}  // namespace mysql
}  // namespace mysqlsh
