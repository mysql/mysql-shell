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

#include "simple_shell_client.h"

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"

#include "modules/base_session.h"

using namespace shcore;

Simple_shell_client::Simple_shell_client()
{
  Shell_core::Mode mode = Shell_core::Mode_SQL;

  _delegate.user_data = this;
  _delegate.print = &Simple_shell_client::deleg_print;
  _delegate.print_error = &Simple_shell_client::deleg_print_error;
  _delegate.prompt = &Simple_shell_client::deleg_input;
  _delegate.password = &Simple_shell_client::deleg_password;
  _delegate.source = &Simple_shell_client::deleg_source;

  _shell.reset(new Shell_core(&_delegate));

  bool lang_initialized;
  _shell->switch_mode(mode, lang_initialized);

#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif

  _shell->set_global("connect",
    Value(Cpp_function::create("connect",
    boost::bind(&Simple_shell_client::connect_session, this, _1),
    "connection_string", String, NULL)));
}

shcore::Value Simple_shell_client::connect_session(const shcore::Argument_list &args)
{
  boost::shared_ptr<mysh::ShellBaseSession> new_session(mysh::connect_session(args, mysh::Node));
  _session.reset(new_session, new_session.get());

  _shell->set_global("session", Value(boost::static_pointer_cast<Object_bridge>(_session)));

  Value default_schema = _session->get_member("defaultSchema");
  if (default_schema)
  {
    _shell->set_global("db", default_schema);
  }
  else
  {
    // XXX assign a dummy placeholder to db
    if (_shell->interactive_mode() != Shell_core::Mode_SQL)
      _shell->print("No default schema selected.\n");
  }

  return Value::Null();
}

boost::shared_ptr<Result_set> Simple_shell_client::process_line(const std::string &line)
{
  Interactive_input_state state = Input_ok;
  boost::shared_ptr<Result_set> empty_result = boost::shared_ptr<Result_set>(new Result_set(-1, -1, ""));

  try
  {
    std::string l = line;
    _shell->handle_input(l, state, boost::bind(&Simple_shell_client::process_result, this, _1));

    return _last_result;
  }
  catch (std::exception &exc)
  {
    //print_err(exc.what());
    throw;
  }
}

void Simple_shell_client::process_result(shcore::Value result)
{
  _last_result = boost::shared_ptr<Result_set>(new Result_set(-1, -1, ""));

  if (result)
  {
    if (result.type == shcore::Object)
    {
      boost::shared_ptr<Object_bridge> object = result.as_object();

      shcore::Value affected_rows = object->has_member("getAffectedRowCount") ? object->call("getAffectedRowCount", Argument_list()) : shcore::Value(-1);
      shcore::Value warning_count = object->has_member("getWarningCount") ? object->call("getWarningCount", Argument_list()) : shcore::Value(-1);
      shcore::Value execution_time = object->has_member("getExecutionTime") ? object->call("getExecutionTime", Argument_list()) : shcore::Value("");

      shcore::Argument_list args;
      // If true returns as data array, if false, returns as document.
      if (!object->has_member("fetchAll") || !object->has_member("getColumns"))
        return;
      shcore::Value result = object->call("fetchAll", args);
      shcore::Value metadata = object->call("getColumns", Argument_list());

      boost::shared_ptr<shcore::Value::Array_type> arr_result = result.as_array();

      if (arr_result->size())
      {
        if (arr_result->at(0).type == shcore::Object)
        {
          // create tabular result
          boost::shared_ptr<std::vector<Result_set_metadata> > meta = populate_metadata(metadata);
          _last_result.reset(new Table_result_set(arr_result, meta, affected_rows.as_int(), warning_count.as_int(), execution_time.as_string()));
        }
        else if (arr_result->at(0).type == shcore::Map)
        {
          _last_result.reset(new Document_result_set(arr_result, -1, -1, ""));
        }
        else
        {
          throw std::runtime_error("Unknow data type returned from query.");
        }
        return;
      }
    }
    else if (result.type == shcore::Array)
    {
      boost::shared_ptr<shcore::Value::Array_type> arr_result = result.as_array();
      _last_result.reset(new Document_result_set(arr_result, -1, -1, ""));
    }
  }

  //std::string executed = _shell->get_handled_input();
  return;
}

boost::shared_ptr<std::vector<Result_set_metadata> > Simple_shell_client::populate_metadata(shcore::Value& metadata)
{
  boost::shared_ptr<std::vector<Result_set_metadata> > result =
    boost::shared_ptr<std::vector<Result_set_metadata> >(new std::vector<Result_set_metadata>());
  boost::shared_ptr<shcore::Value::Array_type> arr_result = metadata.as_array();

  std::vector<shcore::Value>::const_iterator myend = arr_result->end();
  for (std::vector<shcore::Value>::const_iterator it = arr_result->begin(); it != myend; ++it)
  {
    boost::shared_ptr<shcore::Value::Map_type> map = it->as_map();

    Result_set_metadata m = Result_set_metadata(
      map->get_string("catalog"), map->get_string("db"), map->get_string("table"), map->get_string("orig_table"),
      map->get_string("name"), map->get_string("orig_name"), map->get_int("charset"), map->get_int("length"),
      map->get_int("type"), map->get_int("flags"), map->get_int("decimal"));
    result->push_back(m);
  }

  return result;
}

bool Simple_shell_client::connect(const std::string &uri)
{
  Argument_list args;

  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  int port = 3306;
  std::string sock;
  std::string db;
  std::string ssl_ca;
  std::string ssl_cert;
  std::string ssl_key;
  int pwd_found;

  if (!mysh::parse_mysql_connstring(uri, protocol, user, pass, host, port, sock, db, pwd_found, ssl_ca, ssl_cert, ssl_key))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");
  else
  {
    if (!pwd_found)
    {
      throw std::runtime_error("Password is missing in the connection string");
    }
    else
    {
      args.push_back(Value(uri));
    }
  }

  try
  {
    if (_session && _session->is_connected())
    {
      shcore::print("Closing old connection...\n");
      _session->close(shcore::Argument_list());
    }

    // strip password from uri
    std::string uri_stripped = mysh::strip_password(uri);
    shcore::print("Connecting to " + uri_stripped + "...\n");

    connect_session(args);
  }
  catch (std::exception &exc)
  {
    //print_err(exc.what());
    throw;
    return false;
  }

  return true;
}

bool Simple_shell_client::do_shell_command(const std::string &line)
{
  bool handled = _shell->handle_shell_command(line);
  return handled;
}

Simple_shell_client::~Simple_shell_client()
{
  // TODO:
}

void Simple_shell_client::make_connection(const std::string& connstr)
{
  connect(connstr);
}

void Simple_shell_client::switch_mode(shcore::Shell_core::Mode mode)
{
  Shell_core::Mode old_mode = _shell->interactive_mode();
  bool lang_initialized = false;

  if (old_mode != mode)
  {
    switch (mode)
    {
      case Shell_core::Mode_None:
        break;
      case Shell_core::Mode_SQL:
        _shell->switch_mode(mode, lang_initialized);
        break;
      case Shell_core::Mode_JScript:
        _shell->switch_mode(mode, lang_initialized);
        break;
      case Shell_core::Mode_Python:
        _shell->switch_mode(mode, lang_initialized);
        break;
    }
  }
}

boost::shared_ptr<Result_set> Simple_shell_client::execute(const std::string query)
{
  boost::shared_ptr<Result_set> result = process_line(query);
  return result;
}

void Simple_shell_client::deleg_print(void *self, const char *text)
{
  Simple_shell_client* myself = (Simple_shell_client*)self;
  myself->print(text);
}

void Simple_shell_client::deleg_print_error(void *self, const char *text)
{
  Simple_shell_client* myself = (Simple_shell_client*)self;
  myself->print_error(text);
}

bool Simple_shell_client::deleg_input(void *self, const char *text, std::string &ret)
{
  Simple_shell_client* myself = (Simple_shell_client*)self;
  return myself->input(text, ret);
}

bool Simple_shell_client::deleg_password(void *self, const char *text, std::string &ret)
{
  Simple_shell_client* myself = (Simple_shell_client*)self;
  return myself->password(text, ret);
}

void Simple_shell_client::deleg_source(void *self, const char *module)
{
  Simple_shell_client* myself = (Simple_shell_client*)self;
  return myself->source(module);
}

void Simple_shell_client::print(const char *text)
{
}

void Simple_shell_client::print_error(const char *text)
{
}

bool Simple_shell_client::input(const char *text, std::string &ret)
{
  return false;
}

bool Simple_shell_client::password(const char *text, std::string &ret)
{
  return false;
}

void Simple_shell_client::source(const char* module)
{
}