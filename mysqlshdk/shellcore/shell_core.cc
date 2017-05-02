/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "scripting/object_registry.h"
#include "shellcore/base_session.h"
#include "utils/utils_general.h"
#include "utils/base_tokenizer.h"
#include "scripting/lang_base.h"
#include <fstream>
#include <locale>
#include "utils/utils_string.h"
#ifdef WIN32
#include <windows.h>
#endif

using namespace std::placeholders;
using namespace shcore;

Shell_core::Shell_core(Interpreter_delegate *shdelegate)
  : IShell_core(), _client_delegate(shdelegate), _running_query(false), _reconnect_session(false) {
  _mode = Mode::None;
  _registry = new Object_registry();

  // The sys global is for JavaScript only

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

  if (_langs[Mode::JavaScript])
    delete _langs[Mode::JavaScript];

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
    if (_mode == IShell_core::Mode::JavaScript && data.size() > 1 &&
        data[0] == '#' && data[1] == '!')
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
        case Mode::JavaScript:
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
  _langs[Mode::JavaScript] = js = new Shell_javascript(this);

  for (std::map<std::string, std::pair<Mode_mask, Value> >::const_iterator
           iter = _globals.begin();
       iter != _globals.end(); ++iter) {
    if (iter->second.first.matches(Mode::JavaScript))
      js->set_global(iter->first, iter->second.second);
  }
#endif
}

void Shell_core::init_py() {
#ifdef HAVE_PYTHON
  Shell_python *py;
  _langs[Mode::Python] = py = new Shell_python(this);

  for (std::map<std::string, std::pair<Mode_mask, Value> >::const_iterator
           iter = _globals.begin();
       iter != _globals.end(); ++iter) {
    if (iter->second.first.matches(Mode::Python))
      py->set_global(iter->first, iter->second.second);
  }
#endif
}

void Shell_core::set_global(const std::string &name, const Value &value,
                            Mode_mask mode) {
  _globals[name] = std::make_pair(mode, value);

  for (std::map<Mode, Shell_language*>::const_iterator iter = _langs.begin();
       iter != _langs.end(); ++iter) {
    // Only sets the global where applicable
    if (mode.matches(iter->first)) {
      iter->second->set_global(name, value);
    }
  }
}

Value Shell_core::get_global(const std::string &name) {
  return (_globals.count(name) > 0) ? _globals[name].second : Value();
}

std::vector<std::string> Shell_core::get_global_objects(Mode mode) {
  std::vector<std::string> globals;

  for (auto entry : _globals) {
    if (entry.second.first.matches(mode) &&
        entry.second.second.type == shcore::Object)
      globals.push_back(entry.first);
  }

  return globals;
}

std::string Shell_core::prompt() {
  return _langs[interactive_mode()]->prompt();
}

bool Shell_core::handle_shell_command(const std::string &line) {
  return _langs[_mode]->handle_shell_command(line);
}

/**
* Configures the received session as the global development session.
* \param session: The session to be set as global.
*
* If there's a selected schema on the received session, it will be made available to the scripting interfaces on the global *db* variable
*/
std::shared_ptr<mysqlsh::ShellBaseSession> Shell_core::set_dev_session(const std::shared_ptr<mysqlsh::ShellBaseSession>& session) {
  _global_dev_session = session;

  return _global_dev_session;
}

/**
* Returns the global development session.
*/
std::shared_ptr<mysqlsh::ShellBaseSession> Shell_core::get_dev_session() {
  return _global_dev_session;
}

void Shell_core::handle_notification(const std::string &name, const shcore::Object_bridge_ref& sender, shcore::Value::Map_type_ref data) {
  if (name == "SN_SESSION_CONNECTION_LOST") {
    auto session = std::dynamic_pointer_cast<mysqlsh::ShellBaseSession>(sender);

    if (session && session == get_dev_session())
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
      print("The global session got disconnected.\nAttempting to reconnect to '" + _global_dev_session->uri() + "'..");

#ifdef _WIN32
      Sleep(500);
#else
      usleep(500000);
#endif
      int attempts = 6;
      while (!ret_val && attempts > 0) {
        ret_val = reconnect();

        if (!ret_val) {
          print("..");
          attempts--;
          if (attempts > 0) {
            // Try again
#ifdef _WIN32
            Sleep(1500);
#else
            usleep(1500000);
#endif
          }
        }
      }

      if (ret_val)
        print("\nThe global session was successfully reconnected.\n");
      else
        print("\nThe global session could not be reconnected automatically.\nPlease use '\\connect " + _global_dev_session->uri() + "' instead to manually reconnect.\n");
    }
  }

  _reconnect_session = false;

  return ret_val;
}

void Shell_core::execute_module(const std::string &file_name, std::function<void(shcore::Value)> result_processor, const std::vector<std::string> &argv) {
  _input_args = argv;

  _langs[_mode]->execute_module(file_name, result_processor);
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
std::vector<std::string> Shell_command_handler::split_command_line(const std::string &command_line) {
  shcore::BaseTokenizer _tokenizer;

  std::vector<std::string> escaped_quotes = {"\\", "\""};
  std::string quote("\"");

  _tokenizer.set_complex_token("escaped-quote", escaped_quotes);
  _tokenizer.set_complex_token("quote", quote);
  _tokenizer.set_complex_token("space", [](const std::string& input, size_t& index, std::string& text)->bool {
    std::locale locale;
    while (std::isspace(input[index], locale))
      text += input[index++];

    return !text.empty();
  });
  _tokenizer.set_allow_unknown_tokens(true);;
  _tokenizer.set_allow_spaces(true);

  _tokenizer.set_input(command_line);
  _tokenizer.process({0, command_line.length()});

  std::vector<std::string> ret_val;
  bool quoted_param = false;
  std::string param;

  while (_tokenizer.tokens_available()) {
    if (_tokenizer.cur_token_type_is("quote"))
      quoted_param = !quoted_param;

    // Quoted params will get accumulated into a single
    // command argument
    if (quoted_param)
      param += _tokenizer.consume_any_token().get_text();
    else {
      auto token = _tokenizer.consume_any_token();
      if (token.get_type() != "space")
        param += token.get_text();
      else {
        if (!param.empty()) {
          ret_val.push_back(param);
          param.clear();
        }
      }
    }
  }

  // Adds the last argument or raises an error if needed
  if (!param.empty()) {
    if (quoted_param)
      throw shcore::Exception::runtime_error("Missing closing quotes on command parameter");
    else
      ret_val.push_back(param);
  }

  return ret_val;
}

bool Shell_command_handler::process(const std::string& command_line) {
  bool ret_val = false;
  std::vector<std::string> tokens;

  if (!_command_dict.empty()) {
    std::locale locale;
    // Identifies if the line is a registered command
    size_t index = 0;
    while (index < command_line.size() && std::isspace(command_line[index], locale))
      index++;

    size_t start = index;
    while (index < command_line.size() && !std::isspace(command_line[index], locale))
      index++;

    std::string command = command_line.substr(start, index - start);

    // Srearch on the registered command list and processes it if it exists
    Command_registry::iterator item = _command_dict.find(command);
    if (item != _command_dict.end() && item->second->function) {
      // Parses the command
      tokens = split_command_line(command_line);

      // Updates the first element to contain the whole command line
      tokens[0] = command_line;

      ret_val = item->second->function(tokens);
    }
  }

  return ret_val;
}

void Shell_command_handler::add_command(const std::string& triggers, const std::string& description, const std::string& help, Shell_command_function function) {
  Shell_command command = {triggers, description, help, function};
  _commands.push_back(command);

  std::vector<std::string> tokens;
  tokens = split_string(triggers, "|", true);

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
    tokens = split_string((*index).triggers, "|", true);

    tmp_commands.push_back(tokens[0]);
    tokens.erase(tokens.begin());

    if (!tokens.empty())
      tmp_alias.push_back("(" + join_strings(tokens, ",") + ")");
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
  format += str_format("%d", max_length);
  format += "s %-";
  format += str_format("%d", max_alias_length);
  format += "s %s\n";

  ret_val += "\n";

  size_t tmpindex = 0;
  for (index = _commands.begin(); index != end; index++, tmpindex++) {
    ret_val += str_format(format.c_str(), tmp_commands[tmpindex].c_str(), tmp_alias[tmpindex].c_str(), (*index).description.c_str());
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
      triggers = split_string(item->second->triggers, "|", true);
      help += "\n\nTRIGGERS: " + join_strings(triggers, " or ");
    }

    // Prints the additional help
    if (!item->second->help.empty())
      help += "\n\n" + item->second->help;

    ret_val = true;
  }

  return ret_val;
}
