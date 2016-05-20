/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/shell_core.h"
#include "shellcore/shell_sql.h"
#include "shellcore/shell_jscript.h"
#include "shellcore/shell_python.h"
#include "shellcore/object_registry.h"
#include "modules/base_session.h"
#include "interactive_global_schema.h"
#include "interactive_global_session.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/random.hpp>
#pragma GCC diagnostic pop

#include "shellcore/lang_base.h"
#include "uuid_gen.h"
#include <fstream>

using namespace shcore;

Shell_core::Shell_core(Interpreter_delegate *shdelegate)
  : IShell_core(), _lang_delegate(shdelegate), _running_query(false)
{
  // Use a random seed for UUIDs
  std::time_t now = std::time(NULL);
  boost::uniform_int<> dist(INT_MIN, INT_MAX);
  boost::mt19937 gen;
  boost::variate_generator<boost::mt19937 &, boost::uniform_int<> > vargen(gen, dist);
  gen.seed(now);
  init_uuid(vargen());

  _mode = Mode_None;
  _registry = new Object_registry();

  // When using wizards, the global variables are set to the Interactive Wrappers
  // from the beggining, they will allow interactive resolution when the variables
  // are used by the first time
  if ((*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool())
  {
    set_global("db", shcore::Value::wrap<Global_schema>(new Global_schema(*this)));
    set_global("session", shcore::Value::wrap<Global_session>(new Global_session(*this)));
  }

  shcore::print = boost::bind(&shcore::Shell_core::print, this, _1);
}

Shell_core::~Shell_core()
{
  delete _registry;
  shcore::print = shcore::default_print;

  if (_langs[Mode_JScript])
    delete _langs[Mode_JScript];

  if (_langs[Mode_Python])
    delete _langs[Mode_Python];

  if (_langs[Mode_SQL])
    delete _langs[Mode_SQL];
}

bool Shell_core::print_help(const std::string& topic)
{
  return _langs[_mode]->print_help(topic);
}

void Shell_core::print(const std::string &s)
{
  _lang_delegate->print(_lang_delegate->user_data, s.c_str());
}

void Shell_core::print_error(const std::string &s)
{
  _lang_delegate->print_error(_lang_delegate->user_data, s.c_str());
}

bool Shell_core::password(const std::string &s, std::string &ret_pass)
{
  return _lang_delegate->password(_lang_delegate->user_data, s.c_str(), ret_pass);
}

std::string Shell_core::preprocess_input_line(const std::string &s)
{
  return _langs[_mode]->preprocess_input_line(s);
}

void Shell_core::handle_input(std::string &code, Interactive_input_state &state, boost::function<void(shcore::Value)> result_processor)
{
  try
  {
    _running_query = true;
    _langs[_mode]->handle_input(code, state, result_processor);
  }
  catch (...)
  {
    _running_query = false;
    throw;
  }
}

void Shell_core::abort()
{
  _langs[_mode]->abort();
}

std::string Shell_core::get_handled_input()
{
  return _langs[_mode]->get_handled_input();
}

/*
* process_stream will process the content of an opened stream until EOF is found.
* return values:
* - 1 in case of any processing error is found.
* - 0 if no processing errors were found.
*/
int Shell_core::process_stream(std::istream& stream, const std::string& source, boost::function<void(shcore::Value)> result_processor)
{
  // NOTE: global return code is unused at the moment
  //       return code should be determined at application level on process_result
  //       this global return code may be used again once the exit() function is in place
  Interactive_input_state state;
  _global_return_code = 0;

  _input_source = source;

  // In SQL Mode the stdin and file are processed line by line
  if (_mode == Shell_core::Mode_SQL)
  {
    while (!stream.eof())
    {
      std::string line;

      std::getline(stream, line);

      handle_input(line, state, result_processor);

      if (_global_return_code && !(*Shell_core_options::get())[SHCORE_BATCH_CONTINUE_ON_ERROR].as_bool())
        break;
    }

    if (state != Interactive_input_state::Input_ok)
    {
      std::string delimiter = ";";
      handle_input(delimiter, state, result_processor);
    }
  }
  else
  {
    std::string data;
    if (&std::cin == &stream)
    {
      while (!stream.eof())
      {
        std::string line;

        std::getline(stream, line);
        data.append(line).append("\n");
      }
    }
    else
    {
      stream.seekg(0, stream.end);
      std::streamsize fsize = stream.tellg();
      stream.seekg(0, stream.beg);
      data.resize(fsize);
      stream.read(const_cast<char*>(data.data()), fsize);
    }
    handle_input(data, state, result_processor);
  }

  _input_source.clear();

  return _global_return_code;
}

bool Shell_core::switch_mode(Mode mode, bool &lang_initialized)
{
  lang_initialized = false;

  if (_mode != mode)
  {
    _mode = mode;
    if (!_langs[_mode])
    {
      switch (_mode)
      {
        case Mode_None:
          break;
        case Mode_SQL:
          init_sql();
          break;
        case Mode_JScript:
          init_js();
          lang_initialized = true;
          break;
        case Mode_Python:
          init_py();
          lang_initialized = true;
          break;
      }
    }
    return true;
  }
  return false;
}

void Shell_core::init_sql()
{
  _langs[Mode_SQL] = new Shell_sql(this);
}

void Shell_core::init_js()
{
#ifdef HAVE_V8
  Shell_javascript *js;
  _langs[Mode_JScript] = js = new Shell_javascript(this);

  for (std::map<std::string, Value>::const_iterator iter = _globals.begin();
       iter != _globals.end(); ++iter)
    js->set_global(iter->first, iter->second);
#endif
}

void Shell_core::init_py()
{
#ifdef HAVE_PYTHON
  Shell_python *py;
  _langs[Mode_Python] = py = new Shell_python(this);

  for (std::map<std::string, Value>::const_iterator iter = _globals.begin();
       iter != _globals.end(); ++iter)
    py->set_global(iter->first, iter->second);
#endif
}

void Shell_core::set_global(const std::string &name, const Value &value)
{
  // Exception to ensure consistency, if wizard usage is ON then the global variables
  // Can't be replaced, they were set already and integrators (WB/VS) should use
  // set_dev_session or set_current_schema to set the variables
  if ((!name.compare("db") || !name.compare("session")) &&
  _globals.count(name) != 0 &&
  (*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool())
  {
    std::string error = "Can't override the global variables when using wizards is ON. ";

    if (!name.compare("db"))
     error.append("Use set_dev_session instead.");
    else
       error.append("Use set_current_schema instead.");

    throw std::logic_error(error);
  }

  _globals[name] = value;

  for (std::map<Mode, Shell_language*>::const_iterator iter = _langs.begin();
       iter != _langs.end(); ++iter)
    iter->second->set_global(name, value);
}

Value Shell_core::get_global(const std::string &name)
{
  return (_globals.count(name) > 0) ? _globals[name] : Value();
}

void Shell_core::set_active_session(const Value &session)
{
  _active_session = session;
  set_global("session", session);
}

std::string Shell_core::prompt()
{
  return _langs[interactive_mode()]->prompt();
}

bool Shell_core::handle_shell_command(const std::string &line)
{
  return _langs[_mode]->handle_shell_command(line);
}

/**
* Creates a Development session of the given type using the received connection parameters.
* \param args The connection parameters to be used creating the session.
*
* The args list should be filled with a Connectio Data Dictionary and optionally a Password
*
* The Connection Data Dictionary which supports the next elements:
*
*  - host, the host to use for the connection (can be an IP or DNS name)
*  - port, the TCP port where the server is listening (default value is 33060).
*  - schema, the current database for the connection's session.
*  - dbUser, the user to authenticate against.
*  - dbPassword, the password of the user user to authenticate against.
*  - ssl_ca, the path to the X509 certificate authority in PEM format.
*  - ssl_cert, the path to the X509 certificate in PEM format.
*  - ssl_key, the path to the X509 key in PEM format.
*
* If a Password is added to the args list, it will override any password coming on the Connection Data Dictionary.
*
* Since this function creates a Development Session, it can be any of:
*
* - XSession
* - NodeSession
* - ClassicSession
*
* Once the session is established, it will be made available on a global *session* variable.
*
* If the Connection Data contained the *schema* attribute, the schema will be made available to the scripting interfaces on the global *db* variable.
*/
boost::shared_ptr<mysh::ShellDevelopmentSession> Shell_core::connect_dev_session(const Argument_list &args, mysh::SessionType session_type)
{
  return set_dev_session(mysh::connect_session(args, session_type));
}

/**
* Configures the received session as the global development session.
* \param session: The session to be set as global.
*
* If there's a selected schema on the received session, it will be made available to the scripting interfaces on the global *db* variable
*/
boost::shared_ptr<mysh::ShellDevelopmentSession> Shell_core::set_dev_session(boost::shared_ptr<mysh::ShellDevelopmentSession> session)
{
  _global_dev_session.reset(session, session.get());

  // X Session can't have a currentSchema so we set on db the default schema
  shcore::Value currentSchema;
  if (!_global_dev_session->class_name().compare("XSession"))
    currentSchema = _global_dev_session->get_member("defaultSchema");

  // Non X Sessions can have currentSchema so we set on db that one
  else
    currentSchema = _global_dev_session->get_member("currentSchema");

  // When using the interactive wrappers instead of setting the global variables
  // The target Objects on the wrappers are set
  if ((*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool())
  {
    get_global("session").as_object<Interactive_object_wrapper>()->set_target(boost::static_pointer_cast<Cpp_object_bridge>(_global_dev_session));

    if (currentSchema)
      get_global("db").as_object<Interactive_object_wrapper>()->set_target(currentSchema.as_object<Cpp_object_bridge>());
    else
      get_global("db").as_object<Interactive_object_wrapper>()->set_target(boost::shared_ptr<Cpp_object_bridge>());
  }

  // Use the db/session objects directly if the wizards are OFF
  else
  {
    set_global("session", shcore::Value(boost::static_pointer_cast<Object_bridge>(_global_dev_session)));
    set_global("db", currentSchema);
  }

  return _global_dev_session;
}

/**
* Returns the global development session.
*/
boost::shared_ptr<mysh::ShellDevelopmentSession> Shell_core::get_dev_session()
{
  return _global_dev_session;
}

/**
* Sets the received schema as the selected one on the global development session.
* \param name: The name of the schema to be selected.
*
* If the global development session is active, the received schema will be selected as the active one.
*
* The new active schema will be made available to the scripting interfaces on the global *db* variable.
*/
shcore::Value Shell_core::set_current_schema(const std::string& name)
{
  shcore::Value new_schema;

  if (!name.empty())
  {
    if (_global_dev_session && _global_dev_session->is_connected())
    {
      shcore::Argument_list args;
      args.push_back(shcore::Value(name));

      if (!_global_dev_session->class_name().compare("XSession"))
        new_schema = _global_dev_session->get_schema(args);
      else
        new_schema = _global_dev_session->call("setCurrentSchema", args);
    }
  }

  // Updates the Target Object of the global schema if the wizard interaction is
  // turned ON
  if ((*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool())
  {
    if (new_schema)
      get_global("db").as_object<Interactive_object_wrapper>()->set_target(new_schema.as_object<Cpp_object_bridge>());
    else
      get_global("db").as_object<Interactive_object_wrapper>()->set_target(boost::shared_ptr<Cpp_object_bridge>());
  }
  else
    set_global("db", new_schema);

  return new_schema;
}

//------------------ COMMAND HANDLER FUNCTIONS ------------------//
bool Shell_command_handler::process(const std::string& command_line)
{
  bool ret_val = false;
  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, command_line, boost::is_any_of(" "), boost::token_compress_on);

  Command_registry::iterator item = _command_dict.find(tokens[0]);
  if (item != _command_dict.end())
  {
    // Sends the original line on the first element
    tokens[0] = command_line;

    ret_val = item->second->function(tokens);
  }

  return ret_val;
}

void Shell_command_handler::add_command(const std::string& triggers, const std::string& description, const std::string& help, Shell_command_function function)
{
  Shell_command command = { triggers, description, help, function };
  _commands.push_back(command);

  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, triggers, boost::is_any_of("|"), boost::token_compress_on);

  std::vector<std::string>::iterator index = tokens.begin(), end = tokens.end();

  // Inserts a mapping for each given token
  while (index != end)
  {
    _command_dict.insert(std::pair < const std::string&, Shell_command* >(*index, &_commands.back()));
    index++;
  }
}

void Shell_command_handler::print_commands(const std::string& title)
{
  // Gets the length of the longest command
  Command_list::iterator index, end = _commands.end();
  int max_length = 0;
  int max_alias_length = 0;

  std::vector<std::string> tmp_commands;
  std::vector<std::string> tmp_alias;

  for (index = _commands.begin(); index != end; index++)
  {
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, (*index).triggers, boost::is_any_of("|"), boost::token_compress_on);

    tmp_commands.push_back(tokens[0]);
    tokens.erase(tokens.begin());

    if (!tokens.empty())
      tmp_alias.push_back("(" + boost::algorithm::join(tokens, ",") + ")");
    else
      tmp_alias.push_back(" ");

    int tmp_alias_length = tmp_alias.back().length();
    if (tmp_alias_length > max_alias_length)
      max_alias_length = tmp_alias_length;

    int tmp_length = tmp_commands.back().length();
    if (tmp_length > max_length)
      max_length = tmp_length;
  }

  // Prints the command list title
  shcore::print(title);

  // Prints the command list
  std::string format = "%-";
  format += (boost::format("%d") % (max_length)).str();
  format += "s %-";
  format += (boost::format("%d") % (max_alias_length)).str();
  format += "s %s\n";

  shcore::print("\n");

  size_t tmpindex = 0;
  for (index = _commands.begin(); index != end; index++, tmpindex++)
  {
    shcore::print((boost::format(format.c_str()) % tmp_commands[tmpindex] % tmp_alias[tmpindex] % (*index).description.c_str()).str().c_str());
  }
}

bool Shell_command_handler::print_command_help(const std::string& command)
{
  bool ret_val = false;
  std::string cmd = command;

  Command_registry::iterator item = _command_dict.find(command);

  // Add the escape char in order to get the help on an escaped command
  // even without escaping it on the call: \? <command>
  if (item == _command_dict.end())
  {
    if (cmd[0] != '\\')
    {
      cmd.insert(0, 1, '\\');
      item = _command_dict.find(cmd);
    }
  }

  if (item != _command_dict.end())
  {
    // Prints the command description.
    shcore::print(item->second->description + "\n");

    // Prints additional triggers if any
    if (item->second->triggers != command)
    {
      std::vector<std::string> triggers;
      boost::algorithm::split(triggers, item->second->triggers, boost::is_any_of("|"), boost::token_compress_on);
      shcore::print("\nTRIGGERS: " + boost::algorithm::join(triggers, " or ") + "\n");
    }

    // Prints the additional help
    if (!item->second->help.empty())
      shcore::print("\n" + item->second->help + "\n");

    ret_val = true;
  }

  return ret_val;
}