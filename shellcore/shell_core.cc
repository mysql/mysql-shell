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
#include "interactive/interactive_global_schema.h"
#include "interactive/interactive_global_session.h"
#include "interactive/interactive_global_shell.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "modules/mod_mysqlx.h"
#include "modules/mod_mysql.h"
#include "modules/mod_shell.h"
#include "utils/utils_general.h"

#include "interactive/interactive_global_dba.h"
#include "modules/adminapi/mod_dba.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <boost/random.hpp>
#pragma GCC diagnostic pop

#include "shellcore/lang_base.h"
#include "uuid_gen.h"
#include <fstream>
#ifdef WIN32
#include <windows.h>
#endif

using namespace std::placeholders;
using namespace shcore;

Shell_core::Shell_core(Interpreter_delegate *shdelegate)
  : IShell_core(), _client_delegate(shdelegate), _running_query(false), _reconnect_session(false) {
  INIT_MODULE(mysqlsh::mysqlx::Mysqlx);
  INIT_MODULE(mysqlsh::mysql::Mysql);

  // Use a random seed for UUIDs
  std::time_t now = std::time(NULL);
  boost::uniform_int<> dist(INT_MIN, INT_MAX);
  boost::mt19937 gen;
  boost::variate_generator<boost::mt19937 &, boost::uniform_int<> > vargen(gen, dist);
  gen.seed(now);
  init_uuid(vargen());

  _mode = Mode::None;
  _registry = new Object_registry();

  // When using wizards, the global variables are set to the Interactive Wrappers
  // from the beggining, they will allow interactive resolution when the variables
  // are used by the first time
  if ((*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool()) {
    set_global("db", shcore::Value::wrap<Global_schema>(new Global_schema(*this)));
    set_global("session", shcore::Value::wrap<Global_session>(new Global_session(*this)));
    set_global("dba", shcore::Value::wrap<Global_dba>(new Global_dba(*this)));
    set_global("shell", shcore::Value::wrap<Global_shell>(new Global_shell(*this)));
  }

  set_dba_global();
  set_shell_global();

  observe_notification("SN_SESSION_CONNECTION_LOST");

  // Setus the shell core delegate
  _delegate.user_data = this;
  _delegate.user_data = this;
  _delegate.print = &Shell_core::deleg_print;
  _delegate.print_error = &Shell_core::deleg_print_error;
  _delegate.prompt = &Shell_core::deleg_prompt;
  _delegate.password = &Shell_core::deleg_password;
  _delegate.source = &Shell_core::deleg_source;
  _delegate.print_value = &Shell_core::deleg_print_value;
}

Shell_core::~Shell_core() {
  delete _registry;

  if (_langs[Mode::JScript])
    delete _langs[Mode::JScript];

  if (_langs[Mode::Python])
    delete _langs[Mode::Python];

  if (_langs[Mode::SQL])
    delete _langs[Mode::SQL];
}

bool Shell_core::print_help(const std::string& topic) {
  return _langs[_mode]->print_help(topic);
}

void Shell_core::print(const std::string &s) {
  _delegate.print(_delegate.user_data, s.c_str());
}

void Shell_core::println(const std::string &s, const std::string& tag) {
  std::string output(s);
  bool add_new_line = true;

  // When using JSON output ALL must be JSON
  if (!s.empty()) {
    std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();
    if (format.find("json") != std::string::npos) {
      output = format_json_output(output, tag.empty() ? "info" : tag);
      add_new_line = false;
    }
  }

  if (add_new_line)
    output += "\n";

  _client_delegate->print(_client_delegate->user_data, output.c_str());
}

void Shell_core::print_value(const shcore::Value &value, const std::string& tag) {
  _delegate.print_value(_delegate.user_data, value, tag.c_str());
}

std::string Shell_core::format_json_output(const std::string &info, const std::string& tag) {
  // Splits the incoming text in lines
  auto lines = shcore::split_string(info, "\n");

  std::string target;
  target.reserve(info.length());

  auto index = lines.begin(), end = lines.end();
  if (index != end)
    target = *index++;

  while (index != end) {
    if (!(*index).empty())
      target += " " + *index;

    index++;
  }

  return format_json_output(shcore::Value(target), tag);
}

std::string Shell_core::format_json_output(const shcore::Value &info, const std::string& tag) {
  shcore::JSON_dumper dumper((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string() == "json");
  dumper.start_object();
  dumper.append_value(tag, info);
  dumper.end_object();

  return dumper.str() + "\n";
}

void Shell_core::print_error(const std::string &s) {
  std::string output;
  // When using JSON output ALL must be JSON
  std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();
  if (format.find("json") != std::string::npos)
    output = format_json_output(output, "error");
  else
    output = s;

  _client_delegate->print_error(_client_delegate->user_data, output.c_str());
}

bool Shell_core::password(const std::string &s, std::string &ret_pass) {
  std::string prompt(s);

  // When using JSON output ALL must be JSON
  std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();
  if (format.find("json") != std::string::npos)
    prompt = format_json_output(prompt, "password");

  return _client_delegate->password(_client_delegate->user_data, prompt.c_str(), ret_pass);
}

bool Shell_core::prompt(const std::string &s, std::string &ret_val) {
  std::string prompt(s);

  // When using JSON output ALL must be JSON
  std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();
  if (format.find("json") != std::string::npos)
    prompt = format_json_output(prompt, "prompt");

  return _client_delegate->prompt(_client_delegate->user_data, prompt.c_str(), ret_val);
}

std::string Shell_core::preprocess_input_line(const std::string &s) {
  return _langs[_mode]->preprocess_input_line(s);
}

void Shell_core::handle_input(std::string &code, Input_state &state, std::function<void(shcore::Value)> result_processor) {
  try {
    _running_query = true;
    _langs[_mode]->handle_input(code, state, result_processor);
  } catch (...) {
    _running_query = false;
    throw;
  }
}

void Shell_core::abort() {
  _langs[_mode]->abort();
}

std::string Shell_core::get_handled_input() {
  return _langs[_mode]->get_handled_input();
}

/*
* process_stream will process the content of an opened stream until EOF is found.
* return values:
* - 1 in case of any processing error is found.
* - 0 if no processing errors were found.
*/
int Shell_core::process_stream(std::istream& stream, const std::string& source,
                            std::function<void(shcore::Value)> result_processor,
                            const std::vector<std::string> &argv) {
  // NOTE: global return code is unused at the moment
  //       return code should be determined at application level on process_result
  //       this global return code may be used again once the exit() function is in place
  Input_state state = Input_state::Ok;
  _global_return_code = 0;

  _input_source = source;
  _input_args = argv;

  // In SQL Mode the stdin and file are processed line by line
  if (_mode == Shell_core::Mode::SQL) {
    while (!stream.eof()) {
      std::string line;

      std::getline(stream, line);

      handle_input(line, state, result_processor);

      if (_global_return_code && !(*Shell_core_options::get())[SHCORE_BATCH_CONTINUE_ON_ERROR].as_bool())
        break;
    }

    if (state != Input_state::Ok) {
      std::string delimiter = ";";
      handle_input(delimiter, state, result_processor);
    }
  } else {
    std::string data;
    if (&std::cin == &stream) {
      while (!stream.eof()) {
        std::string line;

        std::getline(stream, line);
        data.append(line).append("\n");
      }
    } else {
      stream.seekg(0, stream.end);
      std::streamsize fsize = stream.tellg();
      stream.seekg(0, stream.beg);
      data.resize(fsize);
      stream.read(const_cast<char*>(data.data()), fsize);
    }

    // When processing JavaScript files, validates the very first line to start with #!
    // If that's the case, it is replaced by a comment indicator //
    if (_mode == IShell_core::Mode::JScript && data.size() > 1 && data[0] == '#' && data[1] == '!')
      data.replace(0, 2, "//");

    handle_input(data, state, result_processor);
  }

  _input_source.clear();
  _input_args.clear();

  return _global_return_code;
}

bool Shell_core::switch_mode(Mode mode, bool &lang_initialized) {
  lang_initialized = false;

  if (_mode != mode) {
    _mode = mode;
    if (!_langs[_mode]) {
      switch (_mode) {
        case Mode::None:
          break;
        case Mode::SQL:
          init_sql();
          break;
        case Mode::JScript:
          init_js();
          lang_initialized = true;
          break;
        case Mode::Python:
          init_py();
          lang_initialized = true;
          break;
      }
    }
    return true;
  }
  return false;
}

void Shell_core::init_sql() {
  _langs[Mode::SQL] = new Shell_sql(this);
}

void Shell_core::init_js() {
#ifdef HAVE_V8
  Shell_javascript *js;
  _langs[Mode::JScript] = js = new Shell_javascript(this);

  for (std::map<std::string, Value>::const_iterator iter = _globals.begin();
       iter != _globals.end(); ++iter)
    js->set_global(iter->first, iter->second);
#endif
}

void Shell_core::init_py() {
#ifdef HAVE_PYTHON
  Shell_python *py;
  _langs[Mode::Python] = py = new Shell_python(this);

  for (std::map<std::string, Value>::const_iterator iter = _globals.begin();
       iter != _globals.end(); ++iter)
    py->set_global(iter->first, iter->second);
#endif
}

void Shell_core::set_global(const std::string &name, const Value &value) {
  // Exception to ensure consistency, if wizard usage is ON then the global variables
  // Can't be replaced, they were set already and integrators (WB/VS) should use
  // set_dev_session or set_current_schema to set the variables
  if ((!name.compare("db") || !name.compare("session")) &&
  _globals.count(name) != 0 &&
  (*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool()) {
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

Value Shell_core::get_global(const std::string &name) {
  return (_globals.count(name) > 0) ? _globals[name] : Value();
}

std::vector<std::string> Shell_core::get_global_objects() {
  std::vector<std::string> globals;

  for (auto entry : _globals) {
    if (entry.second.type == shcore::Object)
      globals.push_back(entry.first);
  }

  return globals;
}

void Shell_core::set_active_session(const Value &session) {
  _active_session = session;
  set_global("session", session);
}

std::string Shell_core::prompt() {
  return _langs[interactive_mode()]->prompt();
}

bool Shell_core::handle_shell_command(const std::string &line) {
  return _langs[_mode]->handle_shell_command(line);
}

/**
* Creates a Development session of the given type using the received connection parameters.
* \param args The connection parameters to be used creating the session.
*
* The args list should be filled with a Connection Data Dictionary and optionally a Password
*
* The Connection Data Dictionary supports the next elements:
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
std::shared_ptr<mysqlsh::ShellDevelopmentSession> Shell_core::connect_dev_session(const Argument_list &args, mysqlsh::SessionType session_type) {
  return set_dev_session(mysqlsh::connect_session(args, session_type));
}

/**
* Configures the received session as the global development session.
* \param session: The session to be set as global.
*
* If there's a selected schema on the received session, it will be made available to the scripting interfaces on the global *db* variable
*/
std::shared_ptr<mysqlsh::ShellDevelopmentSession> Shell_core::set_dev_session(std::shared_ptr<mysqlsh::ShellDevelopmentSession> session) {
  _global_dev_session.swap(session);

  // X Session can't have a currentSchema so we set on db the default schema
  shcore::Value currentSchema;
  if (!_global_dev_session->class_name().compare("XSession"))
    currentSchema = _global_dev_session->get_member("defaultSchema");

  // Non X Sessions can have currentSchema so we set on db that one
  else
    currentSchema = _global_dev_session->get_member("currentSchema");

  // When using the interactive wrappers instead of setting the global variables
  // The target Objects on the wrappers are set
  if ((*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool()) {
    get_global("session").as_object<Interactive_object_wrapper>()->set_target(std::static_pointer_cast<Cpp_object_bridge>(_global_dev_session));

    if (currentSchema)
      get_global("db").as_object<Interactive_object_wrapper>()->set_target(currentSchema.as_object<Cpp_object_bridge>());
    else
      get_global("db").as_object<Interactive_object_wrapper>()->set_target(std::shared_ptr<Cpp_object_bridge>());
  }

  // Use the db/session objects directly if the wizards are OFF
  else {
    set_global("session", shcore::Value(std::static_pointer_cast<Object_bridge>(_global_dev_session)));
    set_global("db", currentSchema);
  }

  return _global_dev_session;
}

/**
* Returns the global development session.
*/
std::shared_ptr<mysqlsh::ShellDevelopmentSession> Shell_core::get_dev_session() {
  return _global_dev_session;
}

/**
 * Configures the received session as the global admin session.
 * \param session: The session to be set as global.
 *
 * If there's unique farm on the received session, it will be made available to the scripting interfaces on the global *farm* variable
 */
void Shell_core::set_dba_global() {
  std::shared_ptr<mysqlsh::dba::Dba>dba(new mysqlsh::dba::Dba(this));

  // When using the interactive wrappers instead of setting the global variables
  // The target Objects on the wrappers are set
  if ((*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool())
    get_global("dba").as_object<Interactive_object_wrapper>()->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(dba));

  // Use the admin session objects directly if the wizards are OFF
  else {
    set_global("dba", shcore::Value(std::dynamic_pointer_cast<Object_bridge>(dba)));
  }
}

void Shell_core::set_shell_global() {
  std::shared_ptr<mysqlsh::Shell>shell(new mysqlsh::Shell(this));

  // When using the interactive wrappers instead of setting the global variables
  // The target Objects on the wrappers are set
  if ((*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool())
    get_global("shell").as_object<Interactive_object_wrapper>()->set_target(std::dynamic_pointer_cast<Cpp_object_bridge>(shell));

  // Use the admin session objects directly if the wizards are OFF
  else {
    set_global("shell", shcore::Value(std::dynamic_pointer_cast<Object_bridge>(shell)));
  }
}

/**
* Sets the received schema as the selected one on the global development session.
* \param name: The name of the schema to be selected.
*
* If the global development session is active, the received schema will be selected as the active one.
*
* The new active schema will be made available to the scripting interfaces on the global *db* variable.
*/
shcore::Value Shell_core::set_current_schema(const std::string& name) {
  shcore::Value new_schema;

  if (!name.empty()) {
    if (_global_dev_session && _global_dev_session->is_connected()) {
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
  if ((*Shell_core_options::get())[SHCORE_USE_WIZARDS].as_bool()) {
    if (new_schema)
      get_global("db").as_object<Interactive_object_wrapper>()->set_target(new_schema.as_object<Cpp_object_bridge>());
    else
      get_global("db").as_object<Interactive_object_wrapper>()->set_target(std::shared_ptr<Cpp_object_bridge>());
  } else
    set_global("db", new_schema);

  return new_schema;
}

void Shell_core::handle_notification(const std::string &name, shcore::Object_bridge_ref sender, shcore::Value::Map_type_ref data) {
  if (name == "SN_SESSION_CONNECTION_LOST") {
    auto session = std::dynamic_pointer_cast<mysqlsh::ShellDevelopmentSession>(sender);

    if (session && session == _global_dev_session)
      _reconnect_session = true;
  }
}

bool Shell_core::reconnect() {
  bool ret_val = false;

  try {
    _global_dev_session->reconnect();
    ret_val = true;
  } catch (shcore::Exception &e) {
    ret_val = false;
  }

  return ret_val;
}

bool Shell_core::reconnect_if_needed() {
  bool ret_val = false;
  if (_reconnect_session) {
    {
      print("The global session got disconnected.\nAttempting to reconnect to '" + _global_dev_session->uri() + "'...\n");
      try {
#ifdef _WIN32
        Sleep(500);
#else
        usleep(500000);
#endif
        if (!reconnect()) {
          // Try again
#ifdef _WIN32
          Sleep(1500);
#else
          usleep(1500000);
#endif
          _global_dev_session->reconnect();
        }

        print("The global session was successfully reconnected.\n");
        ret_val = true;
      } catch (shcore::Exception &e) {
        print("The global session could not be reconnected automatically.\nPlease use '\\connect " + _global_dev_session->uri() + "' instead to manually reconnect.\n");
      }
    }

    _reconnect_session = false;
  }

  return ret_val;
}

void Shell_core::deleg_print(void *self, const char *text) {
  Shell_core *shcore = (Shell_core*)self;

  std::string output(text);

  // When using JSON output ALL must be JSON
  std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();
  if (format.find("json") != std::string::npos)
    output = shcore->format_json_output(output, "info");

  auto deleg = shcore->_client_delegate;
  deleg->print(deleg->user_data, output.c_str());
}

void Shell_core::deleg_print_error(void *self, const char *text) {
  Shell_core *shcore = (Shell_core*)self;
  auto deleg = shcore->_client_delegate;

  std::string output;

  if (text)
    output.assign(text);

  // When using JSON output ALL must be JSON
  std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();
  if (format.find("json") != std::string::npos)
    output = shcore->format_json_output(output, "error");

  if (output.length() && output[output.length() - 1] != '\n')
    output += "\n";

  deleg->print_error(deleg->user_data, output.c_str());
}

bool Shell_core::deleg_prompt(void *self, const char *text, std::string &ret) {
  Shell_core *shcore = (Shell_core*)self;
  auto deleg = shcore->_client_delegate;
  return deleg->prompt(deleg->user_data, text, ret);
}

bool Shell_core::deleg_password(void *self, const char *text, std::string &ret) {
  Shell_core *shcore = (Shell_core*)self;
  auto deleg = shcore->_client_delegate;
  return deleg->password(deleg->user_data, text, ret);
}

void Shell_core::deleg_source(void *self, const char *module) {
  Shell_core *shcore = (Shell_core*)self;
  auto deleg = shcore->_client_delegate;
  deleg->source(deleg->user_data, module);
}

void Shell_core::deleg_print_value(void *self, const shcore::Value &value, const char *tag) {
  Shell_core *shcore = (Shell_core*)self;
  auto deleg = shcore->_client_delegate;
  std::string mtag;

  if (tag)
    mtag.assign(tag);

  // If the client delegate has it's own logic to print errors then let it go
  // not expected to be the case.
  if (deleg->print_value)
    deleg->print_value(deleg->user_data, value, tag);
  else {
    std::string output;
    bool add_new_line = true;
    // When using JSON output ALL must be JSON
    std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();
    if (format.find("json") != std::string::npos) {
      // If no tag is provided, prints the JSON representation of the Value
      if (mtag.empty()) {
        output = value.json((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string() == "json");
      } else {
        if (value.type == shcore::String)
          output = shcore->format_json_output(value.as_string(), mtag);
        else
          output = shcore->format_json_output(value, mtag);

        add_new_line = false;
      }
    } else {
      if (mtag == "error" && value.type == shcore::Map) {
        output = "ERROR: ";
        Value::Map_type_ref error_map = value.as_map();

        if (error_map->has_key("code")) {
          //message.append(" ");
          output.append(((*error_map)["code"].repr()));

          if (error_map->has_key("state") && (*error_map)["state"])
            output.append(" (" + (*error_map)["state"].as_string() + ")");

          output.append(": ");
        }

        if (error_map->has_key("message"))
          output.append((*error_map)["message"].as_string());
        else
          output.append("?");
      } else
        output = value.descr(true);
    }

    if (add_new_line)
      output += "\n";

    if (mtag == "error")
      deleg->print_error(deleg->user_data, output.c_str());
    else
      deleg->print(deleg->user_data, output.c_str());
  }
}

//------------------ COMMAND HANDLER FUNCTIONS ------------------//
bool Shell_command_handler::process(const std::string& command_line) {
  bool ret_val = false;
  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, command_line, boost::is_any_of(" "), boost::token_compress_on);

  Command_registry::iterator item = _command_dict.find(tokens[0]);
  if (item != _command_dict.end()) {
    // Sends the original line on the first element
    tokens[0] = command_line;

    ret_val = item->second->function(tokens);
  }

  return ret_val;
}

void Shell_command_handler::add_command(const std::string& triggers, const std::string& description, const std::string& help, Shell_command_function function) {
  Shell_command command = {triggers, description, help, function};
  _commands.push_back(command);

  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, triggers, boost::is_any_of("|"), boost::token_compress_on);

  std::vector<std::string>::iterator index = tokens.begin(), end = tokens.end();

  // Inserts a mapping for each given token
  while (index != end) {
    _command_dict.insert(std::pair < const std::string&, Shell_command* >(*index, &_commands.back()));
    index++;
  }
}

std::string Shell_command_handler::get_commands(const std::string& title) {
  // Gets the length of the longest command
  Command_list::iterator index, end = _commands.end();
  int max_length = 0;
  int max_alias_length = 0;

  std::vector<std::string> tmp_commands;
  std::vector<std::string> tmp_alias;

  for (index = _commands.begin(); index != end; index++) {
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
  std::string ret_val;

  ret_val = title;

  // Prints the command list
  std::string format = "%-";
  format += (boost::format("%d") % (max_length)).str();
  format += "s %-";
  format += (boost::format("%d") % (max_alias_length)).str();
  format += "s %s\n";

  ret_val += "\n";

  size_t tmpindex = 0;
  for (index = _commands.begin(); index != end; index++, tmpindex++) {
    ret_val += (boost::format(format.c_str()) % tmp_commands[tmpindex] % tmp_alias[tmpindex] % (*index).description.c_str()).str();
  }

  return ret_val;
}

bool Shell_command_handler::get_command_help(const std::string& command, std::string &help) {
  bool ret_val = false;
  std::string cmd = command;

  Command_registry::iterator item = _command_dict.find(command);

  // Add the escape char in order to get the help on an escaped command
  // even without escaping it on the call: \? <command>
  if (item == _command_dict.end()) {
    if (cmd[0] != '\\') {
      cmd.insert(0, 1, '\\');
      item = _command_dict.find(cmd);
    }
  }

  if (item != _command_dict.end()) {
    // Prints the command description.
    help += item->second->description;

    // Prints additional triggers if any
    if (item->second->triggers != command) {
      std::vector<std::string> triggers;
      boost::algorithm::split(triggers, item->second->triggers, boost::is_any_of("|"), boost::token_compress_on);
      help += "\n\nTRIGGERS: " + boost::algorithm::join(triggers, " or ");
    }

    // Prints the additional help
    if (!item->second->help.empty())
      help += "\n\n" + item->second->help;

    ret_val = true;
  }

  return ret_val;
}
