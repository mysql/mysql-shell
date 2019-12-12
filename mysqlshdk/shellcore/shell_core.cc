/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/shell_core.h"
#include <fstream>
#include <locale>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "scripting/lang_base.h"
#include "scripting/object_registry.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_jscript.h"
#include "shellcore/shell_python.h"
#include "shellcore/shell_sql.h"
#include "utils/base_tokenizer.h"
#include "utils/debug.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

using std::placeholders::_1;

DEBUG_OBJ_ENABLE(Shell_core);
REGISTER_HELP_TOPIC(Shell Commands, CATEGORY, commands, Contents, ALL);
REGISTER_HELP(COMMANDS_BRIEF,
              "Provides details about the available built-in shell commands.");
REGISTER_HELP(COMMANDS_DETAIL,
              "The shell commands allow executing specific operations "
              "including updating the shell configuration.");
REGISTER_HELP(COMMANDS_CHILDS_DESC,
              "The following shell commands are available:");
REGISTER_HELP(COMMANDS_CLOSING,
              "For help on a specific command use <b>\\?</b> <command>");
REGISTER_HELP(COMMANDS_EXAMPLE, "<b>\\?</b> \\connect");
REGISTER_HELP(COMMANDS_EXAMPLE_DESC,
              "Displays information about the <b>\\connect</b> command.");
namespace shcore {

Shell_core::Shell_core() : IShell_core() {
  DEBUG_OBJ_ALLOC(Shell_core);
  _mode = Mode::None;
  _registry = new Object_registry();
}

Shell_core::~Shell_core() {
  DEBUG_OBJ_DEALLOC(Shell_core);

  _global_dev_session.reset();

  delete _registry;

  // unset all globals from all interpreters
  for (const auto &g : _globals) {
    for (const auto &l : _langs) {
      if (g.second.first.is_set(l.first) && l.second) {
        l.second->set_global(g.first, shcore::Value());
      }
    }
  }
  _globals.clear();

  if (_langs[Mode::JavaScript]) delete _langs[Mode::JavaScript];

  if (_langs[Mode::Python]) delete _langs[Mode::Python];

  if (_langs[Mode::SQL]) delete _langs[Mode::SQL];
}

std::string Shell_core::preprocess_input_line(const std::string &s) {
  return _langs[_mode]->preprocess_input_line(s);
}

void Shell_core::handle_input(std::string &code, Input_state &state) {
  try {
    _langs[_mode]->handle_input(code, state);
  } catch (...) {
    throw;
  }
}

std::string Shell_core::get_handled_input() {
  return _langs[_mode]->get_handled_input();
}

/*
 * process_stream will process the content of an opened stream until EOF is
 * found.
 *
 * return values:
 * - 1 in case of any processing error is found.
 * - 0 if no processing errors were found.
 */
int Shell_core::process_stream(std::istream &stream, const std::string &source,
                               const std::vector<std::string> &argv) {
  // NOTE: global return code is unused at the moment
  //       return code should be determined at application level on
  //       process_result this global return code may be used again once the
  //       exit() function is in place
  Input_state state = Input_state::Ok;
  _global_return_code = 0;

  _input_source = source;
  _input_args = argv;

  if (_mode == Shell_core::Mode::SQL) {
    if (!_langs[_mode]->handle_input_stream(&stream)) _global_return_code = 1;
  } else {
    std::string data;
    if (&std::cin == &stream) {
      while (!stream.eof()) {
        std::string line;

        shcore::getline(stream, line);
        data.append(line).append("\n");
      }
    } else {
      stream.seekg(0, stream.end);
      std::streamsize fsize = stream.tellg();
      stream.seekg(0, stream.beg);
      data.resize(fsize);
      stream.read(const_cast<char *>(data.data()), fsize);
    }

    // When processing JavaScript files, validates the very first line to start
    // with #! If that's the case, it is replaced by a comment indicator //
    if (_mode == IShell_core::Mode::JavaScript && data.size() > 1 &&
        data[0] == '#' && data[1] == '!')
      data.replace(0, 2, "//");

    handle_input(data, state);
  }

  _input_source.clear();
  _input_args.clear();

  return _global_return_code;
}

bool Shell_core::switch_mode(Mode mode) {
  assert(mode != Mode::None);
  // Updates the shell help mode
  m_help.set_mode(mode);

  if (_mode != mode) {
    _mode = mode;
    init_mode(_mode);
    return true;
  }
  return false;
}

void Shell_core::init_mode(Mode mode) {
  if (_langs.find(mode) == _langs.end()) {
    switch (mode) {
      case Mode::None:
        break;
      case Mode::SQL:
        init_sql();
        break;
      case Mode::JavaScript:
        init_js();
        break;
      case Mode::Python:
        init_py();
        break;
    }
  }
}

void Shell_core::init_sql() { _langs[Mode::SQL] = new Shell_sql(this); }

void Shell_core::init_js() {
#ifdef HAVE_V8
  Shell_javascript *js;
  _langs[Mode::JavaScript] = js = new Shell_javascript(this);

  for (std::map<std::string, std::pair<Mode_mask, Value>>::const_iterator iter =
           _globals.begin();
       iter != _globals.end(); ++iter) {
    if (iter->second.first.is_set(Mode::JavaScript))
      js->set_global(iter->first, iter->second.second);
  }
#endif
}

void Shell_core::init_py() {
#ifdef HAVE_PYTHON
  Shell_python *py;
  if (_langs.find(Mode::Python) == _langs.end()) {
    _langs[Mode::Python] = py = new Shell_python(this);

    for (std::map<std::string, std::pair<Mode_mask, Value>>::const_iterator
             iter = _globals.begin();
         iter != _globals.end(); ++iter) {
      if (iter->second.first.is_set(Mode::Python))
        py->set_global(iter->first, iter->second.second);
    }
  }
#endif
}

void Shell_core::set_global(const std::string &name, const Value &value,
                            Mode_mask mode) {
  _globals[name] = std::make_pair(mode, value);

  for (std::map<Mode, Shell_language *>::const_iterator iter = _langs.begin();
       iter != _langs.end(); ++iter) {
    // Only sets the global where applicable
    if (mode.is_set(iter->first)) {
      iter->second->set_global(name, value);
    }
  }
}

bool Shell_core::is_global(const std::string &name) {
  return _globals.find(name) != _globals.end();
}

Value Shell_core::get_global(const std::string &name) {
  return (_globals.count(name) > 0) ? _globals[name].second : Value();
}

std::vector<std::string> Shell_core::get_global_objects(Mode mode) {
  std::vector<std::string> globals;

  for (auto entry : _globals) {
    if (entry.second.first.is_set(mode) &&
        entry.second.second.type == shcore::Object)
      globals.push_back(entry.first);
  }

  return globals;
}

void Shell_core::clear_input() { _langs[interactive_mode()]->clear_input(); }

bool Shell_core::handle_shell_command(const std::string &line) {
  if (!_langs[_mode]->command_handler()->process(line)) {
    return m_command_handler.process(line);
  }
  return false;
}

size_t Shell_core::handle_inline_shell_command(const std::string &line) {
  size_t skip = _langs[_mode]->command_handler()->process_inline(line);
  if (skip == 0) {
    skip = m_command_handler.process_inline(line);
  }
  return skip;
}

/**
 * Configures the received session as the global development session.
 * \param session: The session to be set as global.
 *
 * If there's a selected schema on the received session, it will be made
 * available to the scripting interfaces on the global *db* variable
 */
std::shared_ptr<mysqlsh::ShellBaseSession> Shell_core::set_dev_session(
    const std::shared_ptr<mysqlsh::ShellBaseSession> &session) {
  _global_dev_session = session;

  return _global_dev_session;
}

/**
 * Returns the global development session.
 */
std::shared_ptr<mysqlsh::ShellBaseSession> Shell_core::get_dev_session() {
  return _global_dev_session;
}

std::string Shell_core::get_main_delimiter() const {
  auto it = _langs.find(Mode::SQL);
  if (it == _langs.end()) return ";";
  return dynamic_cast<Shell_sql *>(it->second)->get_main_delimiter();
}

void Shell_core::execute_module(const std::string &file_name,
                                const std::vector<std::string> &argv) {
  _input_args = argv;

  _langs[_mode]->execute_module(file_name);
}

bool Shell_core::load_plugin(Mode mode, const Plugin_definition &plugin) {
  assert(mode != Mode::None);

  init_mode(mode);

  return _langs[mode]->load_plugin(plugin);
}

//------------------ COMMAND HANDLER FUNCTIONS ------------------//
std::vector<std::string> Shell_command_handler::split_command_line(
    const std::string &command_line) {
  shcore::BaseTokenizer _tokenizer;

  static constexpr auto k_quote = "\"";

  _tokenizer.set_complex_token("escaped-quote",
                               std::vector<std::string>{"\\", k_quote});
  _tokenizer.set_complex_token("quote", k_quote);
  _tokenizer.set_complex_token_callback(
      "space",
      [](const std::string &input, size_t &index, std::string &text) -> bool {
        std::locale locale;
        while (std::isspace(input[index], locale)) text += input[index++];

        return !text.empty();
      });
  _tokenizer.set_allow_unknown_tokens(true);

  _tokenizer.set_allow_spaces(true);

  _tokenizer.set_input(command_line);
  _tokenizer.process({0, command_line.length()});

  std::vector<std::string> ret_val;
  std::string param;

  const auto too_many_quotes = [](std::size_t length) {
    return shcore::Exception::runtime_error(
        "Too many consecutive quote characters: " + std::to_string(length));
  };

  const auto missing_space = []() {
    return shcore::Exception::runtime_error(
        "Expected space after closing quote character");
  };

  while (_tokenizer.tokens_available()) {
    if (_tokenizer.cur_token_type_is("quote")) {
      // quoted parameter

      // eat quotes
      auto quote = _tokenizer.consume_any_token().get_text();

      if (quote.length() == 1) {
        // only one quote character, beginning of a quoted parameter
        param = quote;

        // read till the end of string or till next token is a quote
        while (_tokenizer.tokens_available() &&
               !_tokenizer.cur_token_type_is("quote")) {
          param += _tokenizer.consume_any_token().get_text();
        }

        if (!_tokenizer.tokens_available()) {
          // no tokens left, quote was no closed
          throw shcore::Exception::runtime_error(
              "Missing closing quotes on command parameter");
        } else {
          // eat quotes
          quote = _tokenizer.consume_any_token().get_text();

          if (quote.length() == 1) {
            // only one quote character
            if (!_tokenizer.tokens_available() ||
                _tokenizer.cur_token_type_is("space")) {
              // string has finished or quoted parameter was followed by space
              // -> end of quoted parameter
              param += quote;
              ret_val.emplace_back(unquote_string(param, k_quote[0]));
            } else {
              // quote was not followed by a space, two parameters were not
              // properly separated
              throw missing_space();
            }
          } else {
            // two or more quote characters -> error
            throw too_many_quotes(quote.length());
          }
        }
      } else if (quote.length() == 2) {
        // two quote characters, this has to be an empty parameter
        if (!_tokenizer.tokens_available() ||
            _tokenizer.cur_token_type_is("space")) {
          // string has finished or empty parameter was followed by space
          ret_val.emplace_back();
        } else {
          // no space after double quotes, two parameters were not properly
          // separated
          throw missing_space();
        }
      } else {
        // multiple quote characters -> error
        throw too_many_quotes(quote.length());
      }
    } else if (_tokenizer.cur_token_type_is("space")) {
      // space(s) separating parameters
      _tokenizer.consume_any_token();
    } else {
      // unquoted parameter
      param.clear();

      while (_tokenizer.tokens_available() &&
             !_tokenizer.cur_token_type_is("space")) {
        if (_tokenizer.cur_token_type_is("quote")) {
          // quotes are not allowed in an unquoted string
          throw shcore::Exception::runtime_error(
              "Unexpected quote token in an unquoted sequence");
        } else if (_tokenizer.cur_token_type_is("escaped-quote")) {
          // escaped quotes need to have the '\' character removed
          const auto quotes = _tokenizer.consume_any_token().get_text();
          param += std::string(quotes.length() / 2, k_quote[0]);
        } else {
          param += _tokenizer.consume_any_token().get_text();
        }
      }

      ret_val.emplace_back(param);
    }
  }

  return ret_val;
}

bool Shell_command_handler::process(const std::string &command_line) {
  bool ret_val = false;
  std::vector<std::string> tokens;

  if (!_command_dict.empty()) {
    std::locale locale;
    // Identifies if the line is a registered command
    size_t index = 0;
    while (index < command_line.size() &&
           std::isspace(command_line[index], locale))
      index++;

    size_t start = index;
    while (index < command_line.size() &&
           !std::isspace(command_line[index], locale))
      index++;

    std::string command = command_line.substr(start, index - start);

    // Srearch on the registered command list and processes it if it exists
    Command_registry::iterator item = _command_dict.find(command);
    if (item != _command_dict.end() && item->second->function) {
      // Parses the command
      if (item->second->auto_parse_arguments)
        tokens = split_command_line(command_line);
      else
        tokens.resize(1);

      // Updates the first element to contain the whole command line
      tokens[0] = command_line;

      ret_val = item->second->function(tokens);
    }
  }

  return ret_val;
}

size_t Shell_command_handler::process_inline(const std::string &command) {
  auto cmd = _command_dict.find(command.substr(0, 2));
  if (cmd != _command_dict.end()) {
    // currently there are no inline \X commands that accept params, but when
    // they get added, they should be normalized and still return the number
    // of consumed chars from the original input
    bool has_params = false;
    if (!has_params) {
      if (process(command.substr(0, 2))) return 2;
    } else {
      if (process(command.substr(0, 2) + " " + command.substr(3)))
        return command.size();
    }
  }
  return 0;
}

void Shell_command_handler::add_command(const std::string &triggers,
                                        const std::string &help_tag,
                                        Shell_command_function function,
                                        bool case_sensitive_help,
                                        Mode_mask mode,
                                        bool auto_parse_arguments) {
  Shell_command command = {triggers, function, auto_parse_arguments};
  _commands.push_back(command);

  std::vector<std::string> tokens;
  tokens = split_string(triggers, "|", true);

  std::vector<std::string>::iterator index = tokens.begin(), end = tokens.end();

  // Inserts a mapping for each given token
  while (index != end) {
    _command_dict.insert(std::pair<const std::string &, Shell_command *>(
        *index, &_commands.back()));
    index++;
  }

  if (m_use_help) {
    // Verifies if the command is already registered to avoid double entry
    auto topics = Help_registry::get()->search_topics(tokens[0], mode,
                                                      case_sensitive_help);

    if (topics.empty()) {
      Help_topic *topic;
      topic = Help_registry::get()->add_help_topic(
          tokens[0], shcore::Topic_type::COMMAND, help_tag, "Commands", mode);

      // If case insensitive, first trigger is already registered
      if (!case_sensitive_help) tokens.erase(tokens.begin());

      for (auto &token : tokens) {
        Help_registry::get()->register_keyword(token, mode, topic,
                                               case_sensitive_help);
      }

      // If case sensitive, we need now to remove the first trigger
      if (case_sensitive_help) tokens.erase(tokens.begin());

      if (!tokens.empty()) {
        std::string alias = "(" + shcore::str_join(tokens, ",") + ")";
        Help_registry::get()->add_help(help_tag + "_ALIAS", alias);
      }
    }
  }
}

std::vector<std::string> Shell_command_handler::get_command_names_matching(
    const std::string &prefix) const {
  std::vector<std::string> names;

  for (auto cmd : _commands) {
    std::vector<std::string> tokens;
    tokens = split_string(cmd.triggers, "|", true);
    if (!tokens.empty()) {
      if (shcore::str_beginswith(tokens.front(), prefix)) {
        names.push_back(tokens.front());
      }
    }
  }
  return names;
}

}  // namespace shcore
