/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/shell_registry.h"
#include "shellcore/server_registry.h"
#include "shellcore/types.h"
#include "utils/utils_general.h"
#include "utils/utils_file.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

using namespace shcore;

boost::shared_ptr<Shell_registry> Shell_registry::_instance;

std::string Shell_registry::class_name() const
{
  return "ShellRegistry";
}

void Shell_registry::backup_passwords(Value::Map_type *pwd_backups) const
{
  Value::Map_type::const_iterator index, end = _connections->end();

  // Backups the connection pwds and sets dummy to be printed
  Value dummy_pwd("**********");
  for (index = _connections->begin(); index != end; index++)
  {
    Value::Map_type_ref connection = index->second.as_map();
    if (connection->has_key("dbPassword"))
    {
      (*pwd_backups)[index->first] = (*connection)["dbPassword"];
      (*connection)["dbPassword"] = dummy_pwd;
    }
  }
}

void Shell_registry::restore_passwords(Value::Map_type *pwd_backups) const
{
  Value::Map_type::const_iterator index, end = pwd_backups->end();
  for (index = pwd_backups->begin(); index != end; index++)
  {
    Value::Map_type_ref connection = (*_connections)[index->first].as_map();
    (*connection)["dbPassword"] = index->second;
  }
}

void Shell_registry::append_json(shcore::JSON_dumper& dumper) const
{
  Value::Map_type pwd_backups;

  backup_passwords(&pwd_backups);

  dumper.append_value(Value(_connections));

  restore_passwords(&pwd_backups);
}

std::string &Shell_registry::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  Value::Map_type pwd_backups;

  backup_passwords(&pwd_backups);

  Value(_connections).append_descr(s_out, indent, quote_strings);

  restore_passwords(&pwd_backups);

  return s_out;
}

bool Shell_registry::operator == (const Object_bridge &other) const
{
  throw Exception::logic_error("There's only one shell object!");
  return false;
};

std::vector<std::string> Shell_registry::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());

  Value::Map_type::const_iterator index, end = _connections->end();

  for (index = _connections->begin(); index != end; index++)
    members.push_back(index->first);

  return members;
}

Value Shell_registry::get_member(const std::string &prop) const
{
  Value ret_val;
  if (_connections->has_key(prop))
    ret_val = (*_connections)[prop];
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

bool Shell_registry::has_member(const std::string &prop) const
{
  return _connections->has_key(prop) || Cpp_object_bridge::has_member(prop);
}

Shell_registry::Shell_registry() :
_connections(new shcore::Value::Map_type)
{
  add_method("add", boost::bind(&Shell_registry::add, this, _1), "name", shcore::String, NULL);
  add_method("remove", boost::bind(&Shell_registry::remove, this, _1), "name", shcore::String, NULL);
  add_method("update", boost::bind(&Shell_registry::update, this, _1), "name", shcore::String, NULL);

  shcore::Server_registry sr(shcore::get_default_config_path());
  for (std::map<std::string, Connection_options>::const_iterator it = sr.begin(); it != sr.end(); ++it)
  {
    const Connection_options& cs = it->second;
    (*_connections)[cs.get_name()] = Value(fill_connection(cs));
  }
}

Shell_registry::~Shell_registry()
{
  if (_instance)
    _instance.reset();
}

Value Shell_registry::get()
{
  if (!_instance)
    _instance.reset(new Shell_registry());

  return Value(boost::static_pointer_cast<Object_bridge>(get_instance()));
}

boost::shared_ptr<Shell_registry> Shell_registry::get_instance()
{
  if (!_instance)
    _instance.reset(new Shell_registry());

  return _instance;
}

// Stores a connection based on given URI or connection data map
// This function will be called from the dev-api interface
shcore::Value Shell_registry::add(const shcore::Argument_list &args)
{
  args.ensure_count(2, 3, "ShellRegistry.add");

  Value ret_val;
  bool overwrite = false;

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error("ShellRegistry.add: Argument #1 expected to be a string");

  if (args.size() == 3)
  {
    if (args[2].type != shcore::Bool)
      throw shcore::Exception::argument_error("ShellRegistry.add: Argument #3 expected to be boolean");
    else
      overwrite = args[2].as_bool();
  }

  if (args[1].type == shcore::String)
    ret_val = Value(add_connection(args[0].as_string(), args[1].as_string(), overwrite));
  else if (args[1].type == shcore::Map)
    ret_val = store_connection(args[0].as_string(), args[1].as_map(), false, overwrite);
  else
    throw shcore::Exception::argument_error("ShellRegistry.add: Argument #2 expected to be either a URI or a connection data map");

  return ret_val;
}

shcore::Value Shell_registry::update(const shcore::Argument_list &args)
{
  args.ensure_count(2, "ShellRegistry.update");

  Value ret_val;

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error("ShellRegistry.update: Argument #1 expected to be a string");

  if (args[1].type == shcore::String)
    ret_val = Value(update_connection(args[0].as_string(), args[1].as_string()));
  else if (args[1].type == shcore::Map)
    ret_val = store_connection(args[0].as_string(), args[1].as_map(), true, false);
  else
    throw shcore::Exception::argument_error("ShellRegistry.update: Argument #2 expected to be either a URI or a connection data map");

  return ret_val;
}

// Stores a connection based on a URI
// This function can be called both from the dev-uri (through the function above) or from shell command
bool Shell_registry::add_connection(const std::string& name, const std::string& uri, bool overwrite)
{
  return store_connection(name, fill_connection(uri), false, overwrite);
}

// Updates a connection based on a URI
// This function can be called both from the dev-uri (through the function above) or from shell command
bool Shell_registry::update_connection(const std::string& name, const std::string& uri)
{
  return store_connection(name, fill_connection(uri), true, false);
}

// Takes a URI and converts it into a connection data map
Value::Map_type_ref Shell_registry::fill_connection(const std::string& uri)
{
  std::string protocol;
  std::string user;
  std::string password;
  std::string host = "localhost";
  int port = 0;
  std::string sock;
  std::string db;
  int pwd_found;
  std::string ssl_ca;
  std::string ssl_cert;
  std::string ssl_key;
  shcore::parse_mysql_connstring(uri, protocol, user, password, host, port, sock, db, pwd_found, ssl_ca, ssl_cert, ssl_key);

  shcore::Value::Map_type_ref connection_data(new shcore::Value::Map_type());

  if (!host.empty())
    (*connection_data)["host"] = Value(host);

  if (!user.empty())
    (*connection_data)["dbUser"] = Value(user);

  if (!password.empty())
    (*connection_data)["dbPassword"] = Value(password);

  if (port != 0)
    (*connection_data)["port"] = Value(port);

  if (!db.empty())
    (*connection_data)["schema"] = Value(db);

  if (!ssl_ca.empty())
    (*connection_data)["ssl_ca"] = Value(ssl_ca);

  if (!ssl_cert.empty())
    (*connection_data)["ssl_cert"] = Value(ssl_cert);

  if (!ssl_key.empty())
    (*connection_data)["ssl_key"] = Value(ssl_key);

  return connection_data;
}

Value::Map_type_ref Shell_registry::fill_connection(const Connection_options& options)
{
  shcore::Value::Map_type_ref connection_data(new shcore::Value::Map_type());

  std::string value;

  (*connection_data)["host"] = Value(options.get_server());

  (*connection_data)["dbUser"] = Value(options.get_user());

  value = options.get_password();
  if (!value.empty())
    (*connection_data)["dbPassword"] = Value(value);

  value = options.get_port();
  if (!value.empty())
    (*connection_data)["port"] = Value(boost::lexical_cast<int>(value));

  value = options.get_schema();
  if (!value.empty())
    (*connection_data)["schema"] = Value(value);

  value = options.get_value_if_exists("ssl_ca");
  if (!value.empty())
    (*connection_data)["ssl_ca"] = Value(value);

  value = options.get_value_if_exists("ssl_cert");
  if (!value.empty())
    (*connection_data)["ssl_cert"] = Value(value);

  value = options.get_value_if_exists("ssl_key");
  if (!value.empty())
    (*connection_data)["ssl_key"] = Value(value);

  return connection_data;
}

// Validates the connection information and if valid:
// - Stores it in the server registry
// - Makes it available into the shell.registry
shcore::Value Shell_registry::store_connection(const std::string& name, const shcore::Value::Map_type_ref& connection, bool update, bool overwrite)
{
  Value ret_val = Value::False();

  try
  {
    shcore::Server_registry sr(shcore::get_default_config_path());

    if (update)
      sr.update_connection_options(name, get_options_string(connection));
    else
      sr.add_connection_options(name, get_options_string(connection), overwrite);

    sr.merge();
    (*_connections)[name] = Value(connection);
    ret_val = Value::True();
  }
  catch (std::exception& e)
  {
    throw shcore::Exception::argument_error((boost::format("ShellRegistry.%1%: %2%") % (update ? "update" : "add") % e.what()).str());
  }

  return ret_val;
}

std::string Shell_registry::get_options_string(const Value::Map_type_ref& connection)
{
  std::string options;

  Value::Map_type::const_iterator index, end = connection->end();
  for (index = connection->begin(); index != end; index++)
    options += index->first + "=" + index->second.descr(false) + "; ";

  return options;
}

shcore::Value Shell_registry::remove(const shcore::Argument_list &args)
{
  args.ensure_count(1, "ShellRegistry.remove");
  Value ret_val = Value::False();

  if (args[0].type == String)
    ret_val = Value(remove_connection(args[0].as_string()));
  else
    throw Exception::argument_error("ShellRegistry.remove: Argument #1 expected to be a string");

  return ret_val;
}

bool Shell_registry::remove_connection(const std::string& name)
{
  bool ret_val = false;

  try
  {
    if (_connections->has_key(name))
    {
      shcore::Server_registry sr(shcore::get_default_config_path());
      shcore::Connection_options& cs = sr.get_connection_options(name);
      sr.remove_connection_options(cs);
      sr.merge();

      _connections->erase(name);
      ret_val = true;
    }
    else
      throw Exception::argument_error((boost::format("The app name '%1%' does not exist") % name).str());
  }
  catch (std::exception &e)
  {
    throw Exception::argument_error((boost::format("ShellRegistry.remove: %1%") % e.what()).str());
  }

  return ret_val;
}