/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "shell_client.h"

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "logger/logger.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"

using namespace shcore;

Shell_client::Shell_client()
{
  Shell_core::Mode mode = Shell_core::Mode_SQL;

  std::string log_path = shcore::get_user_config_path();
  log_path += "mysqlx_vs.log";
  ngcommon::Logger::create_instance(log_path.c_str(), false, ngcommon::Logger::LOG_ERROR);

 (*shcore::Shell_core_options::get())[SHCORE_MULTIPLE_INSTANCES] = shcore::Value::True();

  _delegate.user_data = this;
  _delegate.print = &Shell_client::deleg_print;
  _delegate.print_error = &Shell_client::deleg_print_error;
  _delegate.prompt = &Shell_client::deleg_input;
  _delegate.password = &Shell_client::deleg_password;
  _delegate.source = &Shell_client::deleg_source;

  _shell.reset(new Shell_core(&_delegate));

  bool lang_initialized;
  _shell->switch_mode(mode, lang_initialized);

#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif

  _shell->set_global("connect",
    Value(Cpp_function::create("connect",
    boost::bind(&Shell_client::connect_session, this, _1),
    "connection_string", String, NULL)));
}

shcore::Value Shell_client::connect_session(const shcore::Argument_list &args)
{
  boost::shared_ptr<mysh::ShellBaseSession> new_session(mysh::connect_session(args, mysh::Node));
  _session.reset(new_session, new_session.get());

  _shell->set_active_session(Value(boost::static_pointer_cast<Object_bridge>(_session)));

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

shcore::Value Shell_client::process_line(const std::string &line)
{
  Interactive_input_state state = Input_ok;
  //boost::shared_ptr<Result_set> empty_result = boost::shared_ptr<Result_set>(new Result_set(-1, -1, ""));

  try
  {
    std::string l = line;
    _shell->handle_input(l, state, boost::bind(&Shell_client::process_result, this, _1));

    return _last_result;
  }
  catch (std::exception &exc)
  {
    //print_err(exc.what());
    throw;
  }

  return _last_result;
}

void Shell_client::process_result(shcore::Value result)
{
  _last_result = result;
}

bool Shell_client::connect(const std::string &uri)
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

  shcore::parse_mysql_connstring(uri, protocol, user, pass, host, port, sock, db, pwd_found, ssl_ca, ssl_cert, ssl_key);

  if (!pwd_found)
  {
    throw std::runtime_error("Password is missing in the connection string");
  }
  else
  {
    args.push_back(Value(uri));
  }

  try
  {
    if (_session && _session->is_connected())
    {
      shcore::print("Closing old connection...\n");
      _session->close(shcore::Argument_list());
    }

    // strip password from uri
    std::string uri_stripped = shcore::strip_password(uri);
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

bool Shell_client::do_shell_command(const std::string &line)
{
  bool handled = _shell->handle_shell_command(line);
  return handled;
}

Shell_client::~Shell_client()
{
  if (_shell) _shell.reset();
  if (_session) _session.reset();
}

void Shell_client::make_connection(const std::string& connstr)
{
  connect(connstr);
}

void Shell_client::switch_mode(shcore::Shell_core::Mode mode)
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

shcore::Value Shell_client::execute(const std::string query)
{
  return process_line(query);
}

void Shell_client::deleg_print(void *self, const char *text)
{
  Shell_client* myself = (Shell_client*)self;
  myself->print(text);
}

void Shell_client::deleg_print_error(void *self, const char *text)
{
  Shell_client* myself = (Shell_client*)self;
  myself->print_error(text);
}

bool Shell_client::deleg_input(void *self, const char *text, std::string &ret)
{
  Shell_client* myself = (Shell_client*)self;
  return myself->input(text, ret);
}

bool Shell_client::deleg_password(void *self, const char *text, std::string &ret)
{
  Shell_client* myself = (Shell_client*)self;
  return myself->password(text, ret);
}

void Shell_client::deleg_source(void *self, const char *module)
{
  Shell_client* myself = (Shell_client*)self;
  return myself->source(module);
}

void Shell_client::print(const char *text)
{
}

void Shell_client::print_error(const char *text)
{
}

bool Shell_client::input(const char *text, std::string &ret)
{
  return false;
}

bool Shell_client::password(const char *text, std::string &ret)
{
  return false;
}

void Shell_client::source(const char* module)
{
}
