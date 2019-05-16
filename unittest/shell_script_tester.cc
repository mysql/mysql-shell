/* Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms, as
   designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.
   This program is distributed in the hope that it will be useful,  but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "unittest/shell_script_tester.h"
#include <iostream>
#include <string>
#include <utility>
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "shellcore/ishell_core.h"
#include "src/mysqlsh/cmdline_shell.h"
#include "utils/process_launcher.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_lexing.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

using namespace shcore;
extern "C" const char *g_test_home;
extern bool g_generate_validation_file;

//-----------------------------------------------------------------------------

static bool do_print(void *udata, const char *s) {
  printf("%s", s);
  fflush(stdout);
  return true;
}

static shcore::Prompt_result do_prompt(void *udata, const char *prompt,
                                       std::string *ret_input) {
  printf("%s", prompt);
  fflush(stdout);
  std::cin >> *ret_input;
  return shcore::Prompt_result::Ok;
}

namespace mysqlsh {
class Test_debugger {
 public:
  enum class Action { Skip_execute, Continue, Abort };

  Test_debugger()
      : m_deleg(this, do_print, do_prompt, nullptr, nullptr, nullptr) {
    m_console.reset(new mysqlsh::Shell_console(&m_deleg));
  }

  void enable(bool step) {
    println("Enabling...");
    m_enabled = true;
    m_stepping = step;
  }

  void on_test_begin(Shell_core_test_wrapper *test) { m_test = test; }

  void on_reset_shell(std::shared_ptr<mysqlsh::Command_line_shell> shell) {
    m_shell = shell;
  }

  Action will_execute(const std::string &source, int lnum,
                      const std::string &code) {
    if (m_main_source.empty()) m_main_source = source;

    // Line containing //BREAK in the script will enter cmd loop
    if (m_enabled) {
      if (m_stepping) {
        if (source != m_main_source && m_skip_include) {
          // skip over include files
          return Action::Continue;
        }
        m_stepping = false;
        return interact();
      } else if (shcore::str_strip(code) == "//BREAK") {
        println("//BREAK hit");
        return interact();
      }
    }
    return Action::Continue;
  }

  Action on_validate_fail(const std::string &chunk_id) {
    if (m_exit_on_test_error) exit(1);
    if (m_enabled) {
      println("Validation '" + chunk_id + "' failed");
      return interact(true);
    } else if (g_test_fail_early) {
      return Action::Abort;
    }
    return Action::Continue;
  }

  void did_execute(int lnum, const std::string &code) {}

  Action did_execute_test_failure() {
    if (m_exit_on_test_error) exit(1);
    if (m_enabled) {
      println("Test failures found");
      return interact();
    } else if (g_test_fail_early) {
      return Action::Abort;
    }
    return Action::Continue;
  }

  void did_throw(int lnum, const std::string &code) {
    if (m_enabled && m_break_on_throw) interact();
  }

 private:
  void println(const std::string &s) {
    puts(mysqlshdk::textui::bold("TDB: " + s).c_str());
  }

  bool debug() { return true; }

  bool handle_command(const std::string &cmd, bool *exit_interactive) {
    if (cmd == "\\next") {
      m_stepping = true;
      m_skip_include = true;
      *exit_interactive = true;
      return true;
    } else if (cmd == "\\step") {
      m_stepping = true;
      m_skip_include = false;
      *exit_interactive = true;
      return true;
    } else if (cmd == "\\dhelp") {
      std::cout << "TDB Help\n"
                << "--------\n"
                << "\\next line (skip INCLUDE)\n"
                << "\\step line (enter INCLUDE)\n"
                << "\\cont continue execution\n"
                << "\\abort let test fail (when handling validation failures)\n"
                << "\\quit exit\n"
                << "\n";
      return true;
    }
    return false;
  }

  Action interact(bool abortable = false) {
    if (m_first_interact) {
      if (abortable)
        println(
            "Entering interactive loop... \\cont to continue, ^D or \\abort to "
            "exit. \\dhelp for debugger help");
      else
        println(
            "Entering interactive loop... \\cont or ^D to continue. \\dhelp "
            "for debugger help");
      m_first_interact = false;
    }

    // save stdout and stderr contents
    std::string std_err = m_test->output_handler.std_err;
    std::string std_out = m_test->output_handler.std_out;

    Action result = do_interact(abortable);
    if (result == Action::Continue)
      println("Continuing...");
    else
      println("Aborting...");

    m_test->output_handler.std_err = std_err;
    m_test->output_handler.std_out = std_out;

    return result;
  }

#define CTRL_C_STR "\003"
  Action do_interact(bool abortable = false) {
    Action result = abortable ? Action::Abort : Action::Continue;
    bool interrupted = false;
    bool exit_interactive = false;
    std::string cmd;
    while (!exit_interactive) {
      char *tmp = mysqlsh::Command_line_shell::readline(
          makebold(shell()->prompt()).c_str());
      if (tmp && strcmp(tmp, CTRL_C_STR) != 0) {
        cmd = tmp;
        free(tmp);
      } else {
        if (tmp) {
          if (strcmp(tmp, CTRL_C_STR) == 0) interrupted = true;
          free(tmp);
        }
        if (interrupted) {
          shell()->clear_input();
          interrupted = false;
          continue;
        }
        break;
      }

      if (cmd == "") {
        // re-execute last command
        cmd = m_last_cmd;
      } else {
        m_last_cmd = cmd;
      }

      if (cmd == "\\cont") {
        result = Action::Continue;
        break;
      } else if (cmd == "\\abort" && abortable) {
        result = Action::Abort;
        break;
      } else if (cmd == "\\quit") {
        exit(1);
      }

      if (!handle_command(cmd, &exit_interactive)) shell()->process_line(cmd);
    }
    return result;
  }

 private:
  bool m_enabled = false;
  bool m_break_on_throw = false;
  bool m_stepping = false;
  bool m_skip_include = false;
  bool m_exit_on_test_error = false;
  bool m_first_interact = true;
  Shell_core_test_wrapper *m_test = nullptr;
  std::string m_last_cmd;
  std::string m_main_source;

  std::shared_ptr<mysqlsh::Shell_console> m_console;
  shcore::Interpreter_delegate m_deleg;
  std::weak_ptr<mysqlsh::Command_line_shell> m_shell;

  std::shared_ptr<mysqlsh::Command_line_shell> shell() {
    return m_shell.lock();
  }
};
}  // namespace mysqlsh

mysqlsh::Test_debugger *g_tdb = nullptr;

void init_tdb() {
  if (!g_tdb) g_tdb = new mysqlsh::Test_debugger();
}

void enable_tdb(bool step) {
  init_tdb();
  g_tdb->enable(step);
}

void fini_tdb() {
  delete g_tdb;
  g_tdb = nullptr;
}

//-----------------------------------------------------------------------------

Shell_script_tester::Shell_script_tester() {
  init_tdb();

  // Default home folder for scripts
  _shell_scripts_home = shcore::path::join_path(g_test_home, "scripts");
  _new_format = false;
}

void Shell_script_tester::SetUp() {
  Crud_test_wrapper::SetUp();

  if (_options->trace_protocol) {
    // Redirect cout
    _cout_backup = std::cout.rdbuf();
    std::cout.rdbuf(_cout.rdbuf());
  }

  g_tdb->on_test_begin(this);
}

void Shell_script_tester::TearDown() {
  Crud_test_wrapper::TearDown();

  if (_options->trace_protocol) {
    // Restore old cout.
    std::cout.rdbuf(_cout_backup);
  }

  g_tdb->on_test_begin(this);
}

void Shell_script_tester::reset_shell() {
  Crud_test_wrapper::reset_shell();
  g_tdb->on_reset_shell(_interactive_shell);
}

void Shell_script_tester::set_config_folder(const std::string &name) {
  // Custom home folder for scripts
  _shell_scripts_home = shcore::path::join_path(g_test_home, "scripts", name);

  // Currently hardcoded since scripts are on the shell repo
  // but can easily be updated to be setup on an ENV VAR so
  // the scripts can by dynamically imported from the dev-api docs
  // with the sctract tool Jan is working on
  _scripts_home = _shell_scripts_home + "/scripts";
}

void Shell_script_tester::set_setup_script(const std::string &name) {
  // if name is an absolute path, join_path will just return name
  _setup_script = shcore::path::join_path(_shell_scripts_home, "setup", name);
}

size_t find_token(const std::string &source, const std::string &find,
                  const std::string &is_not, size_t start_pos) {
  size_t ret_val = std::string::npos;

  while (true) {
    size_t found = source.find(find, start_pos);
    size_t proto = source.find(is_not, start_pos);

    if (found != std::string::npos && found == proto)
      start_pos = proto + is_not.size();
    else {
      ret_val = found;
      break;
    }
  }

  return ret_val;
}

std::string Shell_script_tester::resolve_string(const std::string &source) {
  std::string updated(source);

  size_t start = find_token(updated, "<<<", "<<<< RECEIVE", 0);
  size_t end;

  while (start != std::string::npos) {
    bool strip_trailing_newline = false;

    end = find_token(updated, ">>>", ">>>> SEND", start);

    if (end == std::string::npos)
      throw std::logic_error("Unterminated <<< in test");
    //<<<"fooo"\>>>\n is stripped into fooo (without the trailing newline)
    if (updated.compare(end - 1, 5, "\\>>>\n") == 0) {
      strip_trailing_newline = true;
      --end;
    }
    std::string token = updated.substr(start + 3, end - start - 3);

    // This will make the variable to be printed on the stdout
    output_handler.wipe_out();

    std::string value;
    // If the token was registered in C++ uses it
    if (_output_tokens.count(token)) {
      value = _output_tokens[token];
    } else {
      // If not, we use whatever is defined on the scripting language
      execute_internal(token);
      value = output_handler.std_out;
    }
    // strip the trailing newline added by execute, but not all whitespace
    if (str_endswith(value, "\n")) value = value.substr(0, value.size() - 1);
    if (strip_trailing_newline) {
      updated.replace(start, end - start + 5, value);
    } else {
      updated.replace(start, end - start + 3, value);
    }
    start = find_token(updated, "<<<", "<<<< RECEIVE", 0);
  }

  output_handler.wipe_out();

  return updated;
}

bool Shell_script_tester::validate_line_by_line(const std::string &context,
                                                const std::string &chunk_id,
                                                const std::string &stream,
                                                const std::string &expected,
                                                const std::string &actual,
                                                int srcline, int valline) {
  std::string new_expected(expected);
  std::vector<std::string> expected_lines;
  bool changed = false;
  // Takes this as the posibility of the presense of conditional lines.
  size_t end_pos = expected.find("?{}");
  if (end_pos != std::string::npos) {
    expected_lines = shcore::split_string(expected, "\n");

    // Lines in the format of ?{<condition>} might be the start/end of a
    // conditional Section of expectations, the section ends on a equal to ?{}
    for (size_t index = 0; index < expected_lines.size(); index++) {
      std::string line = expected_lines[index];
      if (!line.empty() && line[0] == '?') {
        auto size = line.size();
        if (size > 1 && line[1] == '{' && line[size - 1] == '}') {
          std::string condition = line.substr(2, size - 3);

          // Empty condition is simply ignored
          if (!condition.empty()) {
            expected_lines.erase(expected_lines.begin() + index);

            // If condition not satisfied deletes all the lines in the middle
            // until the closing tag is found
            bool erase = !context_enabled(condition);
            while (expected_lines.size() > index &&
                   expected_lines[index] != "?{}") {
              if (erase)
                expected_lines.erase(expected_lines.begin() + index);
              else
                index++;
            }

            expected_lines.erase(expected_lines.begin() + index);

            changed = true;

            // Goes back on the index so the new current line is analyzed as
            // well
            index--;
          }
        }
      }
    }
  }

  if (changed) new_expected = shcore::str_join(expected_lines, "\n");

  return check_multiline_expect(context + "@" + chunk_id, stream, new_expected,
                                actual, srcline, valline);
}

bool Shell_script_tester::validate(const std::string &context,
                                   const std::string &chunk_id, bool optional) {
  std::string original_std_out = output_handler.std_out;
  std::string original_std_err = output_handler.std_err;

  size_t out_position = 0;
  size_t err_position = 0;

  std::string validation_id = _chunks[chunk_id].def->validation_id;

  if (_chunk_validations.find(validation_id) != _chunk_validations.end()) {
    bool expect_failures = false;

    // Identifies the validations to be done based on the context
    std::vector<std::shared_ptr<Validation>> validations;
    for (const auto &val : _chunk_validations[validation_id]) {
      bool enabled = false;
      try {
        enabled = context_enabled(val->def->context);

        if (val->def->stream == "PROTOCOL" && !_options->trace_protocol) {
          ADD_FAILURE_AT("validation file", val->def->linenum)
              << "ERROR TESTING PROTOCOL: Protocol tracing is disabled."
              << "\n"
              << "\tCHUNK: " << val->def->line << "\n";
        }
      } catch (const std::invalid_argument &e) {
        ADD_FAILURE_AT("validation file", val->def->linenum)
            << "ERROR EVALUATING VALIDATION CONTEXT: " << e.what() << "\n"
            << "\tCHUNK: " << val->def->line << "\n";
      }

      if (enabled) {
        validations.push_back(val);

        if (!val->expected_error.empty()) expect_failures = true;
      } else {
        SKIP_VALIDATION(val->def->line);
      }
    }

    std::string full_statement;
    // The validations will be performed ONLY if the context is enabled
    for (size_t valindex = 0; valindex < validations.size(); valindex++) {
      // Validation goes against validation code
      if (!validations[valindex]->code.empty()) {
        // Before cleaning up, prints any error found on the script execution
        if (valindex == 0 && !original_std_err.empty()) {
          ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                         _chunks[chunk_id].code[0].first)
              << makered("\tUnexpected Error: " + original_std_err) << "\n";
          return false;
        }

        output_handler.wipe_all();
        _cout.str("");
        _cout.clear();

        std::string backup = _custom_context;
        full_statement.append(validations[valindex]->code);
        _custom_context += "[" + full_statement + "]";
        execute(validations[valindex]->code);
        _custom_context = backup;
        if (_interactive_shell->input_state() == shcore::Input_state::Ok)
          full_statement.clear();
        else
          full_statement.append("\n");

        original_std_err = output_handler.std_err;
        original_std_out = output_handler.std_out;

        out_position = 0;
        err_position = 0;

        output_handler.wipe_all();
        _cout.str("");
        _cout.clear();
      }

      // Validates unexpected error
      if (!expect_failures && !original_std_err.empty()) {
        ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                       _chunks[chunk_id].code[0].first)
            << "while executing chunk: " + _chunks[chunk_id].def->line << "\n"
            << makered("\tUnexpected Error: ") << original_std_err << "\n";
        return false;
      }

      // Validates expected output if any
      if (!validations[valindex]->expected_output.empty()) {
        std::string out = validations[valindex]->expected_output;

        out = resolve_string(out);

        if (out != "*") {
          if (validations[valindex]->def->validation ==
              ValidationType::Simple) {
            auto pos = original_std_out.find(out, out_position);
            if (pos == std::string::npos) {
              if (out_position == 0) {
                ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                               _chunks[chunk_id].code[0].first)
                    << "while executing chunk: " + _chunks[chunk_id].def->line
                    << "\nwith validation at "
                    << validations[valindex]->def->linenum << "\n"
                    << makeyellow("\tSTDOUT missing: ") << out << "\n"
                    << makeyellow("\tSTDOUT actual: ") + original_std_out
                    << "\n";
              } else {
                ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                               _chunks[chunk_id].code[0].first)
                    << "while executing chunk: " + _chunks[chunk_id].def->line
                    << "\nwith validation at "
                    << validations[valindex]->def->linenum << "\n"
                    << makeyellow("\tSTDOUT missing: ") << out << "\n"
                    << makeyellow("\tSTDOUT actual: ") +
                           original_std_out.substr(out_position)
                    << "\n"
                    << makeyellow("\tSTDOUT original: ") + original_std_out
                    << "\n";
              }
              return false;
            } else {
              // Consumes the already found output
              out_position = pos + out.length();
            }
          } else {
            SCOPED_TRACE(_chunks[chunk_id].source);
            if (!validate_line_by_line(
                    context, chunk_id, "STDOUT", out,
                    validations[valindex]->def->stream == "PROTOCOL"
                        ? _cout.str()
                        : original_std_out,
                    _chunks[chunk_id].code[0].first,
                    validations[valindex]->def->linenum))
              return false;
          }
        }
      }

      // Validates unexpected output if any
      if (!validations[valindex]->unexpected_output.empty()) {
        std::string out = validations[valindex]->unexpected_output;

        out = resolve_string(out);
        size_t pos = std::string::npos;
        if (validations[valindex]->def->stream == "PROTOCOL")
          pos = _cout.str().find(out);
        else
          pos = original_std_out.find(out);

        if (pos != std::string::npos) {
          ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                         _chunks[chunk_id].code[0].first)
              << "while executing chunk: " + _chunks[chunk_id].def->line << "\n"
              << "with validation at " << validations[valindex]->def->linenum
              << "\n"
              << makeyellow("\tSTDOUT unexpected: ") << out << "\n"
              << makeyellow("\tSTDOUT actual: ") << original_std_out << "\n";
          return false;
        }
      }

      // Validates expected error if any
      if (!validations[valindex]->expected_error.empty()) {
        std::string error = validations[valindex]->expected_error;

        error = resolve_string(error);

        if (error != "*") {
          if (validations[valindex]->def->validation ==
              ValidationType::Simple) {
            auto pos = original_std_err.find(error, err_position);
            if (pos == std::string::npos) {
              if (err_position == 0) {
                ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                               _chunks[chunk_id].code[0].first)
                    << "while executing chunk: " + _chunks[chunk_id].def->line
                    << "\n"
                    << "with validation at "
                    << validations[valindex]->def->linenum << "\n"
                    << makeyellow("\tSTDERR missing: ") + error << "\n"
                    << makeyellow("\tSTDERR actual: ") + original_std_err
                    << "\n";
              } else {
                ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                               _chunks[chunk_id].code[0].first)
                    << "while executing chunk: " + _chunks[chunk_id].def->line
                    << "\n"
                    << "with validation at "
                    << validations[valindex]->def->linenum << "\n"
                    << makeyellow("\tSTDERR missing: ") + error << "\n"
                    << makeyellow("\tSTDERR actual: ") +
                           original_std_err.substr(err_position)
                    << "\n"
                    << makeyellow("\tSTDERR original: ") + original_std_err
                    << "\n";
              }
              return false;
            } else {
              // Consumes the already found error
              err_position = pos + error.length();
            }
          } else {
            SCOPED_TRACE(_chunks[chunk_id].source);
            if (!validate_line_by_line(context, chunk_id, "STDERR", error,
                                       original_std_err,
                                       _chunks[chunk_id].code[0].first,
                                       validations[valindex]->def->linenum))
              return false;
          }
        }
      }
    }

    if (validations.empty()) {
      ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                     _chunks[chunk_id].code[0].first)
          << makered("MISSING VALIDATIONS FOR CHUNK ")
          << _chunks[chunk_id].def->line << "\n"
          << makeyellow("\tSTDOUT: ") << original_std_out << "\n"
          << makeyellow("\tSTDERR: ") << original_std_err << "\n";
      return false;
    }
    output_handler.wipe_all();
    _cout.str("");
    _cout.clear();
  } else {
    // There were errors
    if (!original_std_err.empty()) {
      ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                     _chunks[chunk_id].code[0].first)
          << "while executing chunk: " + _chunks[chunk_id].def->line << "\n"
          << makered("\tUnexpected Error: ") + original_std_err << "\n";
    } else if (!optional && _chunks.find(chunk_id) != _chunks.end()) {
      // The error is that there are no validations
      ADD_FAILURE_AT(_chunks[chunk_id].source.c_str(),
                     _chunks[chunk_id].code[0].first)
          << makered("MISSING VALIDATIONS FOR CHUNK ")
          << _chunks[chunk_id].def->line << "\n"
          << makeyellow("\tSTDOUT: ") << original_std_out << "\n"
          << makeyellow("\tSTDERR: ") << original_std_err << "\n";
    }
    output_handler.wipe_all();
    _cout.str("");
    _cout.clear();
  }

  return true;
}

void Shell_script_tester::validate_interactive(const std::string &script) {
  _filename = script;
  try {
    if (output_handler.passwords.size() || output_handler.prompts.size()) {
      std::cerr << "Starting with " << output_handler.passwords.size()
                << " password prompts and " << output_handler.prompts.size()
                << " regular prompts queued\n";
    }
    execute_script(script, true);
    if (testutil->test_skipped()) {
      SKIP_TEST(testutil->test_skip_reason());
    }
  } catch (std::exception &e) {
    std::string error = e.what();
    FAIL() << makered("Unexpected exception executing test script: ")
           << e.what() << "\n";
  }
}

static bool is_identifier_assignment(const std::string &s) {
  std::string::size_type p = 0, pp;
  p = mysqlshdk::utils::span_keyword(s, p);
  if (p == 0) return false;
  p = mysqlshdk::utils::span_spaces(s, p);
  if (s[p] != '=') return false;
  ++p;
  pp = mysqlshdk::utils::span_spaces(s, p);
  pp = mysqlshdk::utils::span_keyword(s, pp);
  if (pp == p) return false;
  if (s[pp] == ';') ++pp;
  if (pp < s.size()) return false;
  return true;
}

bool Shell_script_tester::load_source_chunks(const std::string &path,
                                             std::istream &stream,
                                             const std::string &prefix) {
  std::string last_id;

  auto last_chunk = [this, &last_id]() { return &_chunks.at(last_id); };

  int linenum = 0;
  bool ret_val = true;

  while (ret_val && !stream.eof()) {
    std::string line;
    std::getline(stream, line);
    linenum++;

    line = str_rstrip(line, "\r\n");

    std::shared_ptr<Chunk_definition> chunk_def = load_chunk_definition(line);
    if (chunk_def) {
      if (!prefix.empty()) {
        chunk_def->id = prefix + chunk_def->id;
        chunk_def->validation_id = prefix + chunk_def->validation_id;
      }

      // Full Script Context Validation is supported by defining context
      // validation at the __global__ scope.
      // The way to do it is with an anonymous chunk with the context
      // validation i.e.
      // #@ {<context validation>}
      if (chunk_def->id.empty()) {
        if ((last_id.empty() || last_id == prefix + "__global__") &&
            !context_enabled(chunk_def->context)) {
          ADD_SKIPPED_TEST(line);
          ret_val = false;
        }
      } else {
        chunk_def->linenum = linenum;

        last_id = chunk_def->id;

        // Starts the new chunk
        {
          Chunk_t chunk;
          chunk.source = path;
          chunk.def = chunk_def;
          add_source_chunk(path, chunk);
        }
      }

      // Handle include files... we make them look like chunks even if they
      // aren't to make it obvious the previous chunk is over
      if (str_beginswith(line, "//@ INCLUDE ")) {
        // Syntax:
        //@ INCLUDE file.inc
        //@ INCLUDE tag file.inc
        auto tokens = str_split(line, " ", -1, true);
        if (tokens.size() > 2) {
          std::string include = tokens[2];
          std::string tag;
          if (tokens.size() > 3) {
            tag = tokens[2];
            include = tokens[3];
          }

          std::string inc_path =
              shcore::path::join_path(shcore::path::dirname(path), include);
          std::ifstream inc_stream(inc_path.c_str());

          if (!inc_stream.fail()) {
            std::string namespc =
                std::get<0>(shcore::path::split_extension(include));
            if (!tag.empty()) namespc = tag + "::" + namespc;
            load_source_chunks(include, inc_stream, namespc + "::");
          } else {
            ADD_FAILURE_AT(path.c_str(), linenum - 1)
                << makered("Could not load include file " + inc_path) << "\n";
          }
        } else {
          ADD_FAILURE_AT(path.c_str(), linenum - 1)
              << makered("Invalid INCLUDE directive") << "\n";
        }
      }
    } else {
      // Only adds the lines that are NO snippet specifier
      if (line.find("//! [") != 0) {
        if (last_id.empty()) {
          // Add __global__ at the 1st line we find
          Chunk_t chunk;
          chunk.source = path;
          chunk.def->id = last_id = prefix + "__global__";
          chunk.def->validation_id = prefix + "__global__";
          chunk.def->validation = ValidationType::Optional;
          chunk.code.push_back({linenum, line});
          add_source_chunk(path, chunk);
        } else {
          // To simplify validation error handling and reporting, we limit
          // what can appear in an INCLUDE chunk to:
          // - comments
          // - empty space
          // - identifier assignments (x = y)
          if (str_beginswith(last_chunk()->def->id, "INCLUDE ")) {
            if (!line.empty() && !str_beginswith(line, "//") &&
                !is_identifier_assignment(line)) {
              ADD_FAILURE_AT(path.c_str(), linenum - 1)
                  << makered("No code allowed after an //@ INCLUDE directive")
                  << "\n";
            }
          }
          last_chunk()->code.push_back({linenum, line});
        }
      }
    }
  }

  return ret_val;
}

void Shell_script_tester::add_source_chunk(const std::string &path,
                                           const Chunk_t &chunk) {
  if (_chunks.find(chunk.def->id) == _chunks.end()) {
    _chunks[chunk.def->id] = chunk;
    // normalize Windows paths
    _chunks[chunk.def->id].source = str_replace(chunk.source, "\\", "/");
    _chunk_order.push_back(chunk.def->id);
  } else {
    ADD_FAILURE_AT(path.c_str(), chunk.def->linenum - 1)
        << makered("REDEFINITION OF CHUNK: \"") + chunk.source << ":"
        << chunk.def->line << "\"\n"
        << "\tInitially defined at " << _chunks[chunk.def->id].source << ":"
        << (_chunks[chunk.def->id].code[0].first - 1) << "\n";
  }
}

void Shell_script_tester::add_validation(
    const std::shared_ptr<Chunk_definition> &chunk_def,
    const std::vector<std::string> &source) {
  if (source.size() == 3) {
    if (_chunk_validations.find(chunk_def->id) == _chunk_validations.end())
      _chunk_validations[chunk_def->id] = Chunk_validations();

    auto val = std::shared_ptr<Validation>(new Validation(source, chunk_def));

    _chunk_validations[chunk_def->id].push_back(val);
  } else {
    std::string text(makered("WRONG VALIDATION FORMAT FOR CHUNK ") +
                     chunk_def->line);
    text += "\nLine: " + shcore::str_join(source, "|");
    SCOPED_TRACE(text.c_str());
    ADD_FAILURE();
  }
}

/**
 * Process the given line to determine if it is a chunk definition.
 * @param line The line to be processed.
 *
 * @returns The chunk definition if the line is in the right format.
 */
std::shared_ptr<Chunk_definition> Shell_script_tester::load_chunk_definition(
    const std::string &line) {
  std::shared_ptr<Chunk_definition> ret_val;

  if (line.find(get_chunk_token()) == 0) {
    std::string chunk_id;
    std::string validation_id;
    std::string chunk_context;
    ValidationType val_type = ValidationType::Simple;
    std::string stream;

    chunk_id = line.substr(get_chunk_token().size());
    if (chunk_id[0] == '#') chunk_id = chunk_id.substr(1);

    // Identifies the version for the chunk expectations
    // If no version is specified assigns '*'
    auto start = chunk_id.find("{");
    auto end = chunk_id.find("}");

    if (start != std::string::npos && end != std::string::npos && start < end) {
      chunk_context = chunk_id.substr(start + 1, end - start - 1);
      chunk_id = chunk_id.substr(0, start);
    }

    // Identifies the validation type and the stream if applicable
    if (chunk_id.find("<OUT>") == 0) {
      stream = "OUT";
      chunk_id = chunk_id.substr(5);
      chunk_id = str_strip(chunk_id);
      val_type = ValidationType::Multiline;
    } else if (chunk_id.find("<ERR>") == 0) {
      stream = "ERR";
      chunk_id = chunk_id.substr(5);
      chunk_id = str_strip(chunk_id);
      val_type = ValidationType::Multiline;
    } else if (chunk_id.find("<PROTOCOL>") == 0) {
      stream = "PROTOCOL";
      chunk_id = chunk_id.substr(10);
      chunk_id = str_strip(chunk_id);
      val_type = ValidationType::Multiline;
    } else if (chunk_id.find("<>") == 0) {
      stream = "";
      chunk_id = chunk_id.substr(2);
      chunk_id = str_strip(chunk_id);
      val_type = ValidationType::Optional;
    }

    // Identifies the version for the chunk expectations
    // If no version is specified assigns '*'
    start = chunk_id.find("[USE:");
    end = chunk_id.find("]", start);

    if (start != std::string::npos && end != std::string::npos) {
      validation_id = chunk_id.substr(start + 5, end - start - 5);
      chunk_id = chunk_id.substr(0, start);
    } else {
      validation_id = chunk_id;
    }

    chunk_id = str_strip(chunk_id);
    validation_id = str_strip(validation_id);

    ret_val.reset(new Chunk_definition());

    ret_val->line = line;
    ret_val->id = chunk_id;
    ret_val->context = chunk_context;
    ret_val->validation = val_type;
    ret_val->stream = stream;
    ret_val->validation_id = validation_id;
  }

  return ret_val;
}

void Shell_script_tester::load_validations(const std::string &path) {
  std::ifstream file(path.c_str());
  std::vector<std::string> lines;
  bool skip_chunk = false;

  _chunk_validations.clear();

  bool chunk_verification = !_chunk_order.empty();
  size_t chunk_index = 0;
  Chunk_t *current_chunk = &_chunks[_chunk_order[chunk_index]];

  if (g_generate_validation_file) chunk_verification = false;

  int line_no = 0;

  if (!file.fail()) {
    std::shared_ptr<Chunk_definition> current_val_def;
    std::shared_ptr<Chunk_definition> new_val_def;
    while (!file.eof()) {
      std::string line;
      std::getline(file, line);
      line_no++;

      line = shcore::str_rstrip(line);

      // If a new chunk definition is found
      // Adds the accumulated validations to the previous chunk
      new_val_def = load_chunk_definition(line);

      if (new_val_def) {
        new_val_def->linenum = line_no;
        skip_chunk = false;

        // Adds the previous validations
        if (current_val_def && lines.size()) {
          std::string value = multiline(lines);

          value = shcore::str_rstrip(value);

          if (current_val_def->stream == "OUT" ||
              current_val_def->stream == "PROTOCOL")
            add_validation(current_val_def, {"", value, ""});
          else if (current_val_def->stream == "ERR")
            add_validation(current_val_def, {"", "", value});

          current_val_def = new_val_def;

          lines.clear();
        } else {
          current_val_def = new_val_def;
        }

        // When the script is loaded in chunks, the validations should come in
        // the same order the chunks were loaded
        if (chunk_verification) {
          // Ensures the found validation is for a valid chunk
          if (_chunks.find(current_val_def->id) == _chunks.end() &&
              !g_generate_validation_file) {
            ADD_FAILURE_AT(path.c_str(), line_no)
                << makered("FOUND VALIDATION FOR UNEXISTING CHUNK ")
                << current_val_def->line << "\n"
                << "\tLINE: " << line_no << "\n";
            skip_chunk = true;
            continue;
          }

          bool match = current_val_def->id == current_chunk->def->id;

          // If the new validation no longer match the current chunk, steps
          // to the next chunk
          if (!match) {
            chunk_index++;
            current_chunk = &_chunks[_chunk_order[chunk_index]];
            match = current_val_def->id == current_chunk->def->id;
          }

          bool optional = current_chunk->is_validation_optional();
          bool reference =
              current_chunk->def->id != current_chunk->def->validation_id;

          while ((optional || reference) && !match &&
                 chunk_index < _chunk_order.size()) {
            if (reference) {
              auto index =
                  _chunk_validations.find(current_chunk->def->validation_id);

              if (index == _chunk_validations.end()) {
                ADD_FAILURE_AT(path.c_str(), line_no)
                    << makered("CHUNK REFERENCES AN UNEXISTING VALIDATION")
                    << current_chunk->source << ":" << current_chunk->def->line
                    << "\n"
                    << "\tLINE: " << line_no << "\n";
              }
            }

            chunk_index++;
            current_chunk = &_chunks[_chunk_order[chunk_index]];
            optional = current_chunk->is_validation_optional();
            reference =
                current_chunk->def->id != current_chunk->def->validation_id;
            match = current_val_def->id == current_chunk->def->id;
          }

          if (!optional && !match && !g_generate_validation_file) {
            ADD_FAILURE_AT(path.c_str(), line_no)
                << makered("EXPECTED VALIDATIONS FOR CHUNK ")
                << current_chunk->source << ":" << current_chunk->def->line
                << "\n"
                << "INSTEAD FOUND FOR CHUNK " << current_val_def->line << "\n"
                << "\tLINE: " << line_no << "\n";
            skip_chunk = true;
            continue;
          }

          if (chunk_index >= _chunk_order.size() &&
              !g_generate_validation_file) {
            ADD_FAILURE_AT(path.c_str(), line_no)
                << makered("UNEXPECTED VALIDATIONS FOR CHUNK ")
                << current_val_def->line << "\n"
                << "\tLINE: " << line_no << "\n";
            skip_chunk = true;
          }
        }

        // If the new chunk is wrong, ignores it
        // if (!skip_chunk)
        //  current_chunk = new_val_def;
      } else {
        if (!skip_chunk && current_val_def) {
          if (current_val_def->validation != ValidationType::Multiline) {
            // When processing single line validations, lines as comments are
            // ignored
            if (!shcore::str_beginswith(line.c_str(),
                                        get_comment_token().c_str())) {
              line = str_strip(line);
              if (!line.empty()) {
                std::vector<std::string> tokens;
                tokens = split_string(line, "|", false);
                add_validation(current_val_def, tokens);
              }
            }
          } else {
            lines.push_back(line);
          }
        }
      }
    }

    // Adds final formatted value if any
    if (current_val_def &&
        current_val_def->validation == ValidationType::Multiline) {
      std::string value = multiline(lines);

      value = str_rstrip(value);

      if (current_val_def->stream == "OUT" ||
          current_val_def->stream == "PROTOCOL")
        add_validation(current_val_def, {"", value, ""});
      else if (current_val_def->stream == "ERR")
        add_validation(current_val_def, {"", "", value});
    }

    file.close();
  } else {
    if (_new_format) {
      std::string text("Unable to locate validation script: " + path);
      SCOPED_TRACE(text.c_str());
      ADD_FAILURE();
    }
  }
}

void Shell_script_tester::execute_script(const std::string &path,
                                         bool in_chunks, bool is_pre_script) {
  // If no path is provided then executes the setup script
  std::string script(path.empty()
                         ? _setup_script
                         : is_pre_script ? PRE_SCRIPT(path)
                                         : _new_format ? NEW_TEST_SCRIPT(path)
                                                       : TEST_SCRIPT(path));
  std::ifstream stream(script.c_str());

  if (!stream.fail()) {
    // When it is a test script, preprocesses it so the
    // right execution scenario is in place
    if (!path.empty()) {
      if (!is_pre_script) {
        // Processes independent preprocessing file
        std::string pre_script = PRE_SCRIPT(path);
        std::ifstream pre_stream(pre_script.c_str());
        if (!pre_stream.fail()) {
          pre_stream.close();
          _custom_context = "Preprocessing";
          execute_script(path, false, true);
        }
      }

      // Preprocesses the test file itself
      _custom_context = "Setup";
      process_setup(stream);
    }

    // Process the file
    if (in_chunks) {
      _options->interactive = true;
      if (load_source_chunks(script, stream)) {
        // Loads the validations
        load_validations(_new_format ? VAL_SCRIPT(path)
                                     : VALIDATION_SCRIPT(path));
      }

      // Abort the script processing if something went wrong on the validation
      // loading
      if (testutil->test_skipped() && ::testing::Test::HasFailure()) return;

      std::ofstream ofile;
      if (g_generate_validation_file) {
        std::string vfile_name = VALIDATION_SCRIPT(path);
        if (!shcore::is_file(vfile_name)) {
          ofile.open(vfile_name, std::ofstream::out | std::ofstream::trunc);
        } else {
          vfile_name.append(".new");
          ofile.open(vfile_name, std::ofstream::out | std::ofstream::trunc);
        }
      }

      for (size_t index = 0; index < _chunk_order.size(); index++) {
        // Prints debugging information
        _cout.str("");
        _cout.clear();
        if (str_beginswith(_chunk_order[index], "INCLUDE ")) {
          std::string chunk_log = _chunk_order[index];
          std::string splitter(chunk_log.length(), '=');
          output_handler.debug_print(makelblue(splitter));
          output_handler.debug_print(makelblue(chunk_log));
          output_handler.debug_print(makelblue(splitter));
        } else {
          std::string chunk_log = "CHUNK: " + _chunk_order[index];
          std::string splitter(chunk_log.length(), '-');
          output_handler.debug_print(makeyellow(splitter));
          output_handler.debug_print(makeyellow(chunk_log));
          output_handler.debug_print(makeyellow(splitter));
        }

        // Gets the chunks for the next id
        auto &chunk = _chunks[_chunk_order[index]];

        bool enabled;
        try {
          enabled = context_enabled(chunk.def->context);
        } catch (const std::invalid_argument &e) {
          ADD_FAILURE_AT(chunk.source.c_str(), chunk.code[0].first)
              << makered("ERROR EVALUATING CONTEXT: ") << e.what() << "\n"
              << "\tCHUNK: " << chunk.def->line << "\n";
          break;
        }

        // Executes the file line by line
        if (enabled) {
          _custom_context = "while executing chunk \"" + chunk.def->line +
                            "\" at " + chunk.source + ":" +
                            std::to_string(chunk.def->linenum);
          set_scripting_context();
          auto &code = chunk.code;
          std::string full_statement;
          for (size_t chunk_item = 0; chunk_item < code.size(); chunk_item++) {
            std::string line(code[chunk_item].second);

            full_statement.append(line);
            // Execution context is at line (statement actually) level
            _custom_context = chunk.source + "@[" + _chunk_order[index] + "][" +
                              std::to_string(chunk.code[chunk_item].first) +
                              ":" + full_statement + "]";

            // There's chance to do preprocessing
            pre_process_line(path, line);

            if (testutil)
              testutil->set_test_execution_context(
                  chunk.source.c_str(), code[chunk_item].first, this);

            if (g_tdb->will_execute(chunk.source, chunk.code[chunk_item].first,
                                    line) ==
                mysqlsh::Test_debugger::Action::Skip_execute) {
              continue;
            }

            try {
              execute(chunk.code[chunk_item].first, line);
            } catch (...) {
              g_tdb->did_throw(chunk.code[chunk_item].first, line);
              throw;
            }
            if (testutil->test_skipped()) return;

            if (_interactive_shell->input_state() == shcore::Input_state::Ok)
              full_statement.clear();
            else
              full_statement.append("\n");

            g_tdb->did_execute(chunk.code[chunk_item].first, line);

            if (::testing::Test::HasFailure()) {
              if (g_tdb->did_execute_test_failure() ==
                  mysqlsh::Test_debugger::Action::Abort)
                FAIL();
            }
          }

          execute("");

          if (g_generate_validation_file) {
            auto chunk = _chunks[_chunk_order[index]];

            // Only saves the data if the chunk is not a reference
            if (chunk.def->id == chunk.def->validation_id) {
              if (_options->trace_protocol) {
                std::string protocol_text = _cout.str();
                if (!protocol_text.empty()) {
                  ofile << get_chunk_token() << "<PROTOCOL> "
                        << _chunk_order[index] << std::endl;
                  ofile << protocol_text << std::endl;
                }
              }

              if (!output_handler.std_out.empty()) {
                ofile << get_chunk_token() << "<OUT> " << _chunk_order[index]
                      << std::endl;
                ofile << output_handler.std_out << std::endl;
              }

              if (!output_handler.std_err.empty()) {
                ofile << get_chunk_token() << "<ERR> " << _chunk_order[index]
                      << std::endl;
                ofile << output_handler.std_err << std::endl;
              }
            }
            output_handler.wipe_all();
            _cout.str("");
            _cout.clear();
          } else {
            // Validation contexts is at chunk level
            _custom_context =
                path + "@[" + _chunk_order[index] + " validation]";
            if (!validate(path, _chunk_order[index],
                          chunk.is_validation_optional())) {
              if (g_tdb->on_validate_fail(_chunk_order[index]) ==
                  mysqlsh::Test_debugger::Action::Abort) {
                FAIL();
              }

              if (!g_test_trace_scripts) {
                std::cerr
                    << makeredbg(
                           "----------vvvv Failure Log Begin vvvv----------")
                    << std::endl;
                output_handler.flush_debug_log();
                std::cerr
                    << makeredbg(
                           "----------^^^^ Failure Log End ^^^^------------")
                    << std::endl;
              }
            } else {
              output_handler.whipe_debug_log();
            }
          }
        } else {
          SKIP_CHUNK(chunk.def->line);
        }
      }

      if (g_generate_validation_file) {
        ofile.close();
      }
    } else {
      _options->interactive = false;

      // Loads the validations, the validation is to exclude
      // - Loading validations for a pre_script
      // - Loading validations for a setup script (path empty)
      if (!is_pre_script && !path.empty())
        load_validations(_new_format ? VAL_SCRIPT(path)
                                     : VALIDATION_SCRIPT(path));

      // Abort the script processing if something went wrong on the validation
      // loading
      if (testutil->test_skipped() && ::testing::Test::HasFailure()) return;

      // Processes the script
      _interactive_shell->process_stream(stream, script, {}, true);

      // When path is empty it is processing a setup script
      // If an error is found it will be printed here
      if (path.empty() || is_pre_script) {
        if (!output_handler.std_err.empty()) {
          SCOPED_TRACE(output_handler.std_err);
          std::string text("Setup Script: " + _setup_script);
          SCOPED_TRACE(text.c_str());
          ADD_FAILURE();
        }

        output_handler.wipe_all();
        _cout.str("");
        _cout.clear();
      } else {
        // If processing a tets script, performs the validations over it
        _options->interactive = true;
        if (!validate(script)) {
          if (g_test_fail_early) {
            // Failure logs are printed on the fly in debug mode
            FAIL();
          }
          std::cerr << makeredbg("----------vvvv Failure Log vvvv----------")
                    << std::endl;
          output_handler.flush_debug_log();
          std::cerr << makeredbg("----------^^^^^^^^^^^^^^^^^^^^^----------")
                    << std::endl;
        } else {
          output_handler.whipe_debug_log();
        }
      }
    }

    stream.close();
  } else {
    std::string text("Unable to open test script: " + script);
    SCOPED_TRACE(text.c_str());
    ADD_FAILURE();
  }
}

void Shell_script_tester::validate_chunks(const std::string &path,
                                          const std::string &val_path,
                                          const std::string &pre_path) {
  try {
    // If no path is provided then executes the setup script
    std::string script(_new_format ? NEW_TEST_SCRIPT(path) : TEST_SCRIPT(path));
    std::ifstream stream(script.c_str());

    if (!stream.fail()) {
      // Pre-process a .pre file if it exists
      // ------------------------------------
      std::string file_name;
      if (pre_path.empty())
        file_name = PRE_SCRIPT(path);
      else
        file_name = PRE_SCRIPT(pre_path);

      std::ifstream pre_stream(file_name.c_str());
      if (!pre_stream.fail()) {
        pre_stream.close();
        _custom_context = "Preprocessing";
        execute_script(path, false, true);
      }
      // ------------------------------------

      // Preprocesses the test file itself
      _custom_context = "Setup";
      process_setup(stream);

      // Loads the validations
      if (val_path.empty())
        file_name = _new_format ? VAL_SCRIPT(path) : VALIDATION_SCRIPT(path);
      else
        file_name =
            _new_format ? VAL_SCRIPT(val_path) : VALIDATION_SCRIPT(val_path);

      // Process the file
      _options->interactive = true;
      if (load_source_chunks(script, stream)) load_validations(file_name);
      for (size_t index = 0; index < _chunk_order.size(); index++) {
        // Prints debugging information
        std::string chunk_log = "CHUNK: " + _chunk_order[index];
        std::string splitter(chunk_log.length(), '-');
        output_handler.debug_print(makeyellow(splitter));
        output_handler.debug_print(makeyellow(chunk_log));
        output_handler.debug_print(makeyellow(splitter));

        auto chunk = _chunks[_chunk_order[index]];

        bool enabled = false;
        try {
          enabled = context_enabled(chunk.def->context);
        } catch (const std::invalid_argument &e) {
          ADD_FAILURE_AT(script.c_str(), chunk.code[0].first)
              << makered("ERROR EVALUATING CONTEXT: ") << e.what() << "\n"
              << "\tCHUNK: " << chunk.def->line << "\n";
        }

        // Executes the file line by line
        if (enabled) {
          auto &code = chunk.code;
          for (size_t chunk_item = 0; chunk_item < code.size(); chunk_item++) {
            std::string line(code[chunk_item].second);

            // Execution context is at line level
            _custom_context =
                path + "@[" + _chunk_order[index] + "][" + line + "]";

            // There's chance to do preprocessing
            pre_process_line(path, line);

            if (testutil)
              testutil->set_test_execution_context(
                  _filename, (code[chunk_item].first), this);

            if (g_tdb->will_execute(chunk.source, chunk.code[chunk_item].first,
                                    line) ==
                mysqlsh::Test_debugger::Action::Skip_execute) {
              continue;
            }
            try {
              execute(chunk.code[chunk_item].first, line);
            } catch (...) {
              g_tdb->did_throw(chunk.code[chunk_item].first, line);
              throw;
            }

            g_tdb->did_execute(chunk.code[chunk_item].first, line);

            if (testutil->test_skipped()) return;
          }

          execute("");
          execute("");

          // Validation contexts is at chunk level
          _custom_context = path + "@[" + _chunk_order[index] + " validation]";
          if (!validate(path, _chunk_order[index],
                        chunk.is_validation_optional())) {
            if (g_tdb->on_validate_fail(_chunk_order[index]) ==
                mysqlsh::Test_debugger::Action::Abort) {
              FAIL();
            }
            if (!g_test_trace_scripts) {
              std::cerr
                  << makeredbg(
                         "----------vvvv Failure Log Begin vvvv----------")
                  << std::endl;
              output_handler.flush_debug_log();
              std::cerr
                  << makeredbg(
                         "----------^^^^ Failure Log End ^^^^------------")
                  << std::endl;
            }
          } else {
            output_handler.whipe_debug_log();
          }
        }
      }

      stream.close();
    } else {
      std::string text("Unable to open test script: " + script);
      SCOPED_TRACE(text.c_str());
      ADD_FAILURE();
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}

// Searches for // Assumpsions: comment, if found, creates the __assumptions__
// array And processes the _assumption_script
void Shell_script_tester::process_setup(std::istream &stream) {
  bool done = false;
  while (!done && !stream.eof()) {
    std::string line;
    std::getline(stream, line);

    if (line.find(get_assumptions_token()) == 0) {
      // Removes the assumptions header and parses the rest
      std::vector<std::string> tokens;
      tokens = split_string(line, ":", true);

      // Now parses the real assumptions
      std::string assumptions = tokens[1];
      tokens = split_string(assumptions, ",", true);

      // Now quotes the assumptions
      for (size_t index = 0; index < tokens.size(); index++) {
        tokens[index] = str_strip(tokens[index]);
        tokens[index] = "'" + tokens[index] + "'";
      }

      // Creates an assumptions array to be processed on the setup script
      std::string code = get_variable_prefix() + "__assumptions__ = [" +
                         str_join(tokens, ",") + "];";
      execute(code);

      if (_setup_script.empty())
        throw std::logic_error(
            "A setup script must be specified when there are assumptions on "
            "the tested scripts.");
      else
        execute_script();  // Executes the active setup script
    } else {
      done = true;
    }
  }

  // Once the assumptions are processed, rewinds the read position
  // To the beggining of the script
  stream.clear();  // To clean up the eof flag in case it was set
  stream.seekg(0, stream.beg);
}

void Shell_script_tester::validate_batch(const std::string &script) {
  _filename = script;
  execute_script(script, false);
}

void Shell_script_tester::def_var(const std::string &var,
                                  const std::string &value) {
  std::string code = get_variable_prefix() + var + " = " + value;
  exec_and_out_equals(code);
}

void Shell_script_tester::set_defaults() {
  output_handler.wipe_all();
  _cout.str("");
  _cout.clear();

  Crud_test_wrapper::set_defaults();

  std::string test_mode;
  switch (g_test_recording_mode) {
    case mysqlshdk::db::replay::Mode::Direct:
      test_mode = "direct";
      break;
    case mysqlshdk::db::replay::Mode::Record:
      test_mode = "record";
      break;
    case mysqlshdk::db::replay::Mode::Replay:
      test_mode = "replay";
      break;
  }

  std::string code =
      get_variable_prefix() + "__test_execution_mode = '" + test_mode + "'";
  exec_and_out_equals(code);

  code = get_variable_prefix() + "__package_year = '" PACKAGE_YEAR "'";
  exec_and_out_equals(code);

  code = get_variable_prefix() + "__mysh_full_version = '" +
         std::string(MYSH_FULL_VERSION) + "'";
  exec_and_out_equals(code);

  code = get_variable_prefix() + "__mysh_version = '" +
         std::string(MYSH_VERSION) + "'";
  exec_and_out_equals(code);

  code = get_variable_prefix() + "__version = '" +
         _target_server_version.get_base() + "'";
  exec_and_out_equals(code);

  code = get_variable_prefix() + "__version_num = " +
         std::to_string(_target_server_version.get_major() * 10000 +
                        _target_server_version.get_minor() * 100 +
                        _target_server_version.get_patch());
  exec_and_out_equals(code);

#ifdef WITH_OCI
  def_var("__with_oci", "1");
#else
  def_var("__with_oci", "0");
#endif

#ifdef DBUG_OFF
  def_var("__dbug_off", "1");
#else
  def_var("__dbug_off", "0");
#endif
}

void Shell_js_script_tester::set_defaults() {
  _interactive_shell->process_line("\\js");
  Shell_script_tester::set_defaults();

#ifdef HAVE_V8
  exec_and_out_equals("var __have_javascript = true");
#else
  exec_and_out_equals("var __have_javascript = false");
#endif
}

void Shell_py_script_tester::set_defaults() {
  _interactive_shell->process_line("\\py");
  Shell_script_tester::set_defaults();

#ifdef HAVE_V8
  exec_and_out_equals("__have_javascript = True");
#else
  exec_and_out_equals("__have_javascript = False");
#endif
}

std::string Shell_script_tester::get_current_mode_command() {
  switch (_interactive_shell->shell_context()->interactive_mode()) {
    case shcore::IShell_core::Mode::SQL:
      return "\\sql";
    case shcore::IShell_core::Mode::JavaScript:
      return "\\js";
    case shcore::IShell_core::Mode::Python:
      return "\\py";
    case shcore::IShell_core::Mode::None:
      return "";
  }
  return "";
}

void Shell_script_tester::set_scripting_context() {
  std::string current = get_current_mode_command();
  std::string required = get_switch_mode_command();

  std::string context = context_identifier();
  context = shcore::str_replace(context, "'", "\\'");
  std::string code = get_variable_prefix() += "__test_context = '";
  code += context + "';";

  if (current != required) {
    execute_internal(required);
    execute_internal(code);
    execute_internal(current);
  } else {
    execute_internal(code);
  }
}

void Shell_script_tester::execute_setup() {
  const std::vector<std::string> argv;
  _interactive_shell->process_file(_setup_script, argv);
}

// Append option to the end of the given config file.
void Shell_script_tester::add_to_cfg_file(const std::string &cfgfile_path,
                                          const std::string &option) {
  std::ofstream cfgfile(cfgfile_path, std::ios_base::app);
  cfgfile << option << std::endl;
  cfgfile.close();
}

// Delete lines with the option from the given config file.
void Shell_script_tester::remove_from_cfg_file(const std::string &cfgfile_path,
                                               const std::string &option) {
  std::string new_cfgfile_path = cfgfile_path + ".new";
  std::ofstream new_cfgfile(new_cfgfile_path);
  std::ifstream cfgfile(cfgfile_path);
  std::string line;
  while (std::getline(cfgfile, line)) {
    if (line.find(option) != 0) new_cfgfile << line << std::endl;
  }
  cfgfile.close();
  new_cfgfile.close();
  std::remove(cfgfile_path.c_str());
  std::rename(new_cfgfile_path.c_str(), cfgfile_path.c_str());
}

// Check whether openssl executable is accessible via PATH
bool Shell_script_tester::has_openssl_binary() {
  const char *argv[] = {"openssl", "version", nullptr};
  shcore::Process_launcher p(argv);
  p.start();
  std::string s = p.read_line();
  if (p.wait() == 0) {
    return shcore::str_beginswith(s, "OpenSSL");
  }
  return false;
}

bool Shell_script_tester::context_enabled(const std::string &context) {
  bool ret_val = true;
  std::string code(context);
  if (!context.empty()) {
    size_t function_pos = code.find("VER(");
    while (function_pos != std::string::npos) {
      size_t version_pos = code.find_first_of("0123456789", function_pos);
      size_t closing_pos = code.find(")", version_pos);

      if (version_pos == std::string::npos || closing_pos == std::string::npos)
        throw std::invalid_argument("Invalid syntax for VER(#.#.#) macro.");

      std::string old_func =
          code.substr(function_pos, closing_pos - function_pos + 1);
      std::string op = shcore::str_strip(
          code.substr(function_pos + 4, version_pos - function_pos - 4));
      std::string ver = shcore::str_strip(
          code.substr(version_pos, closing_pos - version_pos));
      std::string fname =
          shcore::get_member_name("versionCheck", get_naming_style());
      std::string new_func =
          "testutil." + fname + "(__version, '" + op + "', '" + ver + "')";

      code = shcore::str_replace(code, old_func, new_func);
      function_pos = code.find("VER(");
    }

    output_handler.wipe_out();

    std::string value;
    execute_internal(code);
    value = output_handler.std_out;
    value = str_strip(value);
    if (value == "true" || value == "True") {
      ret_val = true;
    } else if (value == "false" || value == "False") {
      ret_val = false;
    } else {
      if (!output_handler.std_err.empty()) {
        throw std::invalid_argument(output_handler.std_err);
      } else {
        throw std::invalid_argument(
            "Context does not evaluate to a boolean "
            "expression");
      }
    }
  }

  return ret_val;
}

void Shell_script_tester::execute(int location, const std::string &code) {
  // save location in test script that is being currently executed
  _current_entry_point = context_identifier();
  try {
    Crud_test_wrapper::execute(location, code);
    _current_entry_point.clear();
  } catch (...) {
    _current_entry_point.clear();
    throw;
  }
}

void Shell_script_tester::execute(const std::string &code) {
  // save location in test script that is being currently executed
  _current_entry_point = context_identifier();
  try {
    Crud_test_wrapper::execute(code);
    _current_entry_point.clear();
  } catch (...) {
    _current_entry_point.clear();
    throw;
  }
}
