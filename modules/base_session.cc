/*
 * Copyright (c) 2015, 2016 Oracle and/or its affiliates. All rights reserved.
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

#include "base_session.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/common.h"
#include "shellcore/shell_notifications.h"

#include "shellcore/proxy_object.h"

#include "utils/utils_general.h"
#include "utils/utils_file.h"
#include "mysqlxtest_utils.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#ifdef HAVE_LIBMYSQLCLIENT
#include "mod_mysql_session.h"
#endif
#include "mod_mysqlx_session.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;

std::shared_ptr<mysh::ShellDevelopmentSession> mysh::connect_session(const shcore::Argument_list &args, SessionType session_type) {
  std::shared_ptr<ShellDevelopmentSession> ret_val;

  mysh::SessionType type(session_type);

  // Automatic protocol detection is ON
  // Attempts X Protocol first, then Classic
  if (type == mysh::SessionType::Auto) {
    ret_val.reset(new mysh::mysqlx::NodeSession());
    try {
      ret_val->connect(args);

      ShellNotifications::get()->notify("SN_SESSION_CONNECTED", ret_val);

      return ret_val;
    } catch (shcore::Exception &e) {
      // Unknown message received from server indicates an attempt to create
      // And X Protocol session through the MySQL protocol
      int code = 0;
      if (e.error()->has_key("code"))
        code = e.error()->get_int("code");

      if (code == 2027 || // Unknown message received from server 10
         code == 2002)    // No connection could be made because the target machine actively refused it connecting to host:port
        type = mysh::SessionType::Classic;
      else
        throw;
    }
  }

  switch (type) {
    case mysh::SessionType::X:
      ret_val.reset(new mysh::mysqlx::XSession());
      break;
    case mysh::SessionType::Node:
      ret_val.reset(new mysh::mysqlx::NodeSession());
      break;
#ifdef HAVE_LIBMYSQLCLIENT
    case mysh::SessionType::Classic:
      ret_val.reset(new mysql::ClassicSession());
      break;
#endif
    default:
      throw shcore::Exception::argument_error("Invalid session type specified for MySQL connection.");
      break;
  }

  ret_val->connect(args);

  ShellNotifications::get()->notify("SN_SESSION_CONNECTED", ret_val);

  return ret_val;
}

ShellBaseSession::ShellBaseSession() :
_port(0) {
  init();
}

ShellBaseSession::ShellBaseSession(const ShellBaseSession& s) :
_user(s._user), _password(s._password), _host(s._host), _port(s._port), _sock(s._sock), _schema(s._schema),
_ssl_ca(s._ssl_ca), _ssl_cert(s._ssl_cert), _ssl_key(s._ssl_key) {
  init();
}

void ShellBaseSession::init() {
  add_property("uri", "getUri");
  add_method("isOpen", std::bind(&ShellBaseSession::is_open, this, _1), NULL);
}

std::string &ShellBaseSession::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const {
  if (!is_connected())
    s_out.append("<" + class_name() + ":disconnected>");
  else
    s_out.append("<" + class_name() + ":" + _uri + ">");
  return s_out;
}

std::string &ShellBaseSession::append_repr(std::string &s_out) const {
  return append_descr(s_out, false);
}

void ShellBaseSession::append_json(shcore::JSON_dumper& dumper) const {
  dumper.start_object();

  dumper.append_string("class", class_name());
  dumper.append_bool("connected", is_connected());

  if (is_connected())
    dumper.append_string("uri", _uri);

  dumper.end_object();
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function. The content of the returned value depends on the property being requested. The next list shows the valid properties as well as the returned value for each of them:
 *
 * \li uri: returns a String object with connection information in URI format.
 */
#endif
shcore::Value ShellBaseSession::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "uri")
    ret_val = shcore::Value(_uri);
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

void ShellBaseSession::load_connection_data(const shcore::Argument_list &args) {
  // The connection data can come from different sources
  std::string uri;
  std::string auth_method;
  std::string connections_file; // The default connection file or the indicated on the map as dataSourceFile
  shcore::Value::Map_type_ref options; // Map with the connection data

  //-----------------------------------------------------
  // STEP 1: Identifies the source of the connection data
  //-----------------------------------------------------
  if (args[0].type == String) {
    std::string temp = args.string_at(0);
    uri = temp;
  }

  // Connection data comes in a dictionary
  else if (args[0].type == Map) {
    options = args.map_at(0);

    // Use a custom stored sessions file, rather than the default one
    if (options->has_key("dataSourceFile"))
      connections_file = (*options)["dataSourceFile"].as_string();
  } else
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  //-------------------------------------------------------------------------
  // STEP 2: Gets the individual connection parameters whatever the source is
  //-------------------------------------------------------------------------
  // Handles the case where an URI was received
  if (!uri.empty()) {
    std::string protocol;
    int pwd_found;
    parse_mysql_connstring(uri, protocol, _user, _password, _host, _port, _sock, _schema, pwd_found, _ssl_ca, _ssl_cert, _ssl_key);
  }
  
  // If the connection data came in a dictionary, the values in the dictionary override whatever
  // is already loaded: i.e. if the dictionary indicated a stored session, that info is already
  // loaded but will be overriden with whatever extra values exist on the dictionary
  if (options) {
    if (options->has_key("host"))
      _host = (*options)["host"].as_string();

    if (options->has_key("port"))
      _port = (*options)["port"].as_int();

    if (options->has_key("socket"))
      _sock = (*options)["socket"].as_string();

    if (options->has_key("schema"))
      _schema = (*options)["schema"].as_string();

    if (options->has_key("dbUser"))
      _user = (*options)["dbUser"].as_string();
    else if (options->has_key("user"))
      _user = (*options)["user"].as_string();

    if (options->has_key("dbPassword"))
      _password = (*options)["dbPassword"].as_string();
    else if (options->has_key("password"))
      _password = (*options)["password"].as_string();

    if (options->has_key("ssl_ca"))
      _ssl_ca = (*options)["ssl_ca"].as_string();

    if (options->has_key("ssl_cert"))
      _ssl_cert = (*options)["ssl_cert"].as_string();

    if (options->has_key("ssl_key"))
      _ssl_key = (*options)["ssl_key"].as_string();

    if (options->has_key("authMethod"))
      _auth_method = (*options)["authMethod"].as_string();
  }

  // If password is received as parameter, then it overwrites
  // Anything found on any of the indicated sources: URI, options map and stored session
  if (2 == args.size())
    _password = args.string_at(1).c_str();

  if (_port == 0 && _sock.empty())
    _port = get_default_port();

  std::string sock_port = (_port == 0) ? _sock : boost::lexical_cast<std::string>(_port);

  if (_schema.empty())
    _uri = (boost::format("%1%@%2%:%3%") % _user % _host % sock_port).str();
  else
    _uri = (boost::format("%1%@%2%:%3%/%4%") % _user % _host % sock_port % _schema).str();
}

bool ShellBaseSession::operator == (const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

std::string ShellBaseSession::get_quoted_name(const std::string& name) {
  size_t index = 0;
  std::string quoted_name(name);

  while ((index = quoted_name.find("`", index)) != std::string::npos) {
    quoted_name.replace(index, 1, "``");
    index += 2;
  }

  quoted_name = "`" + quoted_name + "`";

  return quoted_name;
}

shcore::Value ShellBaseSession::is_open(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("isOpen").c_str());

  return shcore::Value(is_connected());
}

void ShellBaseSession::reconnect() {
  shcore::Argument_list args;
  args.push_back(shcore::Value(_uri));
  args.push_back(shcore::Value(_password));

  connect(args);
}

ShellDevelopmentSession::ShellDevelopmentSession() :
ShellBaseSession() {
  init();
}

ShellDevelopmentSession::ShellDevelopmentSession(const ShellDevelopmentSession& s) :
ShellBaseSession(s) {
  init();
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function. The content of the returned value depends on the property being requested. The next list shows the valid properties as well as the returned value for each of them:
 *
 * \li defaultSchema: returns Schema or ClassicSchema object representing the default schema defined on the connectio ninformatio used to create the session. If none was specified, returns Null.
 */
#endif
shcore::Value ShellDevelopmentSession::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "defaultSchema") {
    if (!_default_schema.empty()) {
      shcore::Argument_list args;
      args.push_back(shcore::Value(_default_schema));
      ret_val = get_schema(args);
    } else
      ret_val = Value::Null();
  } else
    ret_val = ShellBaseSession::get_member(prop);

  return ret_val;
}

void ShellDevelopmentSession::init() {
  add_property("defaultSchema", "getDefaultSchema");

  add_method("createSchema", std::bind(&ShellDevelopmentSession::create_schema, this, _1), "name", shcore::String, NULL);
  add_method("getSchema", std::bind(&ShellDevelopmentSession::get_schema, this, _1), "name", shcore::String, NULL);
  add_method("getSchemas", std::bind(&ShellDevelopmentSession::get_schemas, this, _1), NULL);

  _tx_deep = 0;
}

void ShellDevelopmentSession::start_transaction() {
  if (_tx_deep == 0)
    execute_sql("start transaction", shcore::Argument_list());

  _tx_deep++;
}

void ShellDevelopmentSession::commit() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute_sql("commit", shcore::Argument_list());
}

void ShellDevelopmentSession::rollback() {
  _tx_deep--;

  assert(_tx_deep >= 0);

  if (_tx_deep == 0)
    execute_sql("rollback", shcore::Argument_list());
}
