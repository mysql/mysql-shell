/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

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

#include "shell_script_tester.h"
#include <string>
#include <utility>
#include "shellcore/ishell_core.h"
#include "utils/process_launcher.h"
#include "utils/utils_file.h"
#include "utils/utils_path.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

using namespace shcore;
extern "C" const char *g_test_home;

static int debug_level() {
  if (const char *level = getenv("TEST_DEBUG")) {
    return std::atoi(level);
  }
  return 0;
}

Shell_script_tester::Shell_script_tester() {
  // Default home folder for scripts
  _shell_scripts_home = shcore::path::join_path(g_test_home, "scripts");
  _new_format = false;
}

void Shell_script_tester::SetUp() {
  Crud_test_wrapper::SetUp();
}

void Shell_script_tester::set_config_folder(const std::string& name) {
  // Custom home folder for scripts
  _shell_scripts_home = shcore::path::join_path(g_test_home, "scripts", name);

  // Currently hardcoded since scripts are on the shell repo
  // but can easily be updated to be setup on an ENV VAR so
  // the scripts can by dynamically imported from the dev-api docs
  // with the sctract tool Jan is working on
  _scripts_home = _shell_scripts_home + "/scripts";
}

void Shell_script_tester::set_setup_script(const std::string& name) {
  // if name is an absolute path, join_path will just return name
  _setup_script = shcore::path::join_path(_shell_scripts_home, "setup", name);
}

std::string Shell_script_tester::resolve_string(const std::string& source) {
  std::string updated(source);

  size_t start;
  size_t end;

  start = updated.find("<<<");
  while (start != std::string::npos) {
    end = updated.find(">>>", start);

    std::string token = updated.substr(start + 3, end - start - 3);

    // This will make the variable to be printed on the stdout
    output_handler.wipe_out();

    std::string value;
    // If the token was registered in C++ uses it
    if (_output_tokens.count(token)) {
      value = _output_tokens[token];
    } else {
      // If not, we use whatever is defined on the scripting language
      execute(token);
      value = output_handler.std_out;
    }

    value = str_strip(value);

    updated.replace(start, end - start + 3, value);

    start = updated.find("<<<");
  }

  output_handler.wipe_out();

  return updated;
}

bool Shell_script_tester::validate_line_by_line(const std::string& context,
                                                const std::string& chunk_id,
                                                const std::string& stream,
                                                const std::string& expected,
                                                const std::string& actual) {
  return check_multiline_expect(context + "@" + chunk_id, stream, expected,
                                actual);
}

bool Shell_script_tester::validate(const std::string& context,
                                   const std::string& chunk_id) {
  bool ret_val = true;
  std::string original_std_out = output_handler.std_out;
  std::string original_std_err = output_handler.std_err;
  size_t out_position = 0;
  size_t err_position = 0;

  if (_chunk_validations.count(chunk_id)) {
    Validation_t validations;
    if (_chunk_validations[chunk_id].count(_test_context))
      validations = _chunk_validations[chunk_id][_test_context];
    else
      validations = _chunk_validations[chunk_id]["*"];

    for (size_t valindex = 0; valindex < validations.size(); valindex++) {
      // Validation goes against validation code
      if (!validations[valindex].code.empty()) {
        // Before cleaning up, prints any error found on the script execution
        if (valindex == 0 && !original_std_err.empty()) {
          ret_val = false;
          ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id][0].first)
              << "\tUnexpected Error: " + original_std_err << "\n";
        }

        output_handler.wipe_all();
        std::string backup = _custom_context;
        _custom_context += "[" + validations[valindex].code + "]";
        execute(validations[valindex].code);
        _custom_context = backup;

        original_std_err = output_handler.std_err;
        original_std_out = output_handler.std_out;
        out_position = 0;
        err_position = 0;

        output_handler.wipe_all();
      }

      // Validates unexpected error
      if (validations[valindex].expected_error.empty() &&
          !original_std_err.empty()) {
        ret_val = false;
        ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id][0].first)
            << "while executing chunk: //@ " + chunk_id << "\n"
            << "\tUnexpected Error: " + original_std_err << "\n";
      }

      // Validates expected output if any
      if (!validations[valindex].expected_output.empty()) {
        std::string out = validations[valindex].expected_output;

        out = resolve_string(out);

        if (out != "*") {
          if (validations[valindex].type == ValidationType::Simple) {
            auto pos = original_std_out.find(out, out_position);
            if (pos == std::string::npos) {
              ret_val = false;
              ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id][0].first)
                  << "while executing chunk: //@ " + chunk_id << "\n"
                  << "\tSTDOUT missing: " + out << "\n"
                  << "\tSTDOUT actual: " + original_std_out.substr(out_position) << "\n"
                  << "\tSTDOUT original: " + original_std_out << "\n";
            } else {
              // Consumes the already found output
              out_position = pos + out.length();
            }
          } else {
            ret_val = validate_line_by_line(context, chunk_id, "STDOUT", out,
                                            original_std_out);
          }
        }
      }

      // Validates unexpected output if any
      if (!validations[valindex].unexpected_output.empty()) {
        std::string out = validations[valindex].unexpected_output;

        out = resolve_string(out);

        if (original_std_out.find(out) != std::string::npos) {
          ret_val = false;
          ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id][0].first)
               << "while executing chunk: //@ " + chunk_id << "\n"
               << "\tSTDOUT unexpected: " + out << "\n"
               << "\tSTDOUT actual: " + original_std_out << "\n";
        }
      }

      // Validates expected error if any
      if (!validations[valindex].expected_error.empty()) {
        std::string error = validations[valindex].expected_error;

        error = resolve_string(error);

        if (error != "*") {
          if (validations[valindex].type == ValidationType::Simple) {
            auto pos = original_std_err.find(error, err_position);
            if (pos == std::string::npos) {
              ret_val = false;
              ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id][0].first)
                   << "while executing chunk: //@ " + chunk_id << "\n"
                   << "\tSTDERR missing: " + error << "\n"
                   << "\tSTDERR actual: " + original_std_err.substr(err_position) << "\n"
                   << "\tSTDERR original: " + original_std_err << "\n";
            } else {
              // Consumes the already found error
              err_position = pos + error.length();
            }
          } else {
            ret_val = validate_line_by_line(context, chunk_id, "STDERR", error,
                                            original_std_err);
          }
        }
      }
    }
    output_handler.wipe_all();
  } else {
    // Every defined validation chunk MUST have validations
    // or should not be defined as a validation chunk
    // OTOH we always check for errors in global context
    if (chunk_id != "__global__" || !original_std_err.empty()) {
      if (chunk_id == "__global__") {
        ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id][0].first)
            << "while executing chunk: //@ " + chunk_id << "\n"
            << "\tUnexpected Error: " + original_std_err << "\n";
        output_handler.wipe_all();
      } else {
        ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id][0].first)
            << "MISSING VALIDATIONS FOR CHUNK //@ " << chunk_id << "\n"
            << "\tSTDOUT: " << original_std_out << "\n"
            << "\tSTDERR: " << original_std_err << "\n";
      }
      ret_val = false;
    } else {
      output_handler.wipe_all();
    }
  }

  return ret_val;
}

void Shell_script_tester::validate_interactive(const std::string& script) {
  _filename = script;
  try {
    execute_script(script, true);
  } catch (std::exception& e) {
    std::string error = e.what();
  }
}

void Shell_script_tester::load_source_chunks(std::istream& stream) {
  std::string chunk_id = "__global__";
  std::vector<std::pair<size_t, std::string>> current_chunk;
  int linenum = 0;

  while (!stream.eof()) {
    std::string line;
    std::getline(stream, line);
    linenum++;

    line = str_rstrip(line, "\r\n");

    if (line.find(get_chunk_token()) == 0) {
      if (!current_chunk.empty()) {
        _chunks[chunk_id] = current_chunk;
        _chunk_order.push_back(chunk_id);
      }

      chunk_id = line.substr(get_chunk_token().size());
      if (chunk_id[0] == '#')
        chunk_id = chunk_id.substr(1);
      chunk_id = str_strip(chunk_id);

      if (chunk_id.find("<OUT>") == 0 || chunk_id.find("<ERR>") == 0) {
        chunk_id = chunk_id.substr(5);
        chunk_id = str_strip(chunk_id);
      }

      current_chunk = {};
    } else {
      if (current_chunk.empty())
        current_chunk = std::vector<std::pair<size_t, std::string>>();

      // Only adds the lines that are NO snippet specifier
      if (line.find("//! [") != 0)
        current_chunk.push_back({linenum, line});
    }
  }

  // Inserts the remaining code chunk
  if (!current_chunk.empty()) {
    _chunks[chunk_id] = current_chunk;
    _chunk_order.push_back(chunk_id);
  }
}

void Shell_script_tester::add_validation(const std::string& chunk_id,
                                         const std::string& version_id,
                                         const std::vector<std::string>& source,
                                         ValidationType type) {
  if (source.size() == 3) {
    if (!_chunk_validations.count(chunk_id))
      _chunk_validations[chunk_id] = Context_validation_t();

    if (!_chunk_validations[chunk_id].count(version_id))
      _chunk_validations[chunk_id][version_id] = Validation_t();

    // Line by line validation may be complement of an existing validation
    if (type == ValidationType::LineByLine) {
      if (_chunk_validations[chunk_id][version_id].size() == 0)
        _chunk_validations[chunk_id][version_id].push_back(
            Validation(source, type));
      else if (_chunk_validations[chunk_id][version_id].size() == 1) {
        if (_chunk_validations[chunk_id][version_id].at(0).type ==
            ValidationType::LineByLine) {
          if (source[1].size() > 0)
            _chunk_validations[chunk_id][version_id].at(0).expected_output =
                source[1];
          if (source[2].size() > 0)
            _chunk_validations[chunk_id][version_id].at(0).expected_error =
                source[2];
        } else
          throw std::runtime_error(
              "Unable to mix Single and Line by Line validations");
      } else {
        throw std::runtime_error(
            "Unable to mix Single and Line by Line validations");
      }
    } else {
      _chunk_validations[chunk_id][version_id].push_back(
          Validation(source, type));
    }
  } else {
      std::string text("WRONG VALIDATION FORMAT FOR CHUNK //@ " + chunk_id);
      text += "\nLine: " + shcore::str_join(source, "|");
      SCOPED_TRACE(text.c_str());
      ADD_FAILURE();
  }
}

void Shell_script_tester::load_validations(const std::string& path,
                                           bool in_chunks) {
  std::ifstream file(path.c_str());
  std::string chunk_id = "__global__";
  std::string version_id = "*";
  std::string format;
  std::vector<std::string> format_lines;

  _chunk_validations.clear();

  if (!file.fail()) {
    while (!file.eof()) {
      std::string line;
      std::getline(file, line);

      line = shcore::str_rstrip(line);

      if (line.find(get_chunk_token()) == 0) {
        if (format_lines.size()) {
          std::string value = multiline(format_lines);

          value = str_strip(value);

          if (format == "OUT")
            add_validation(chunk_id, version_id, {"", value, ""},
                           ValidationType::LineByLine);
          else if (format == "ERR")
            add_validation(chunk_id, version_id, {"", "", value},
                           ValidationType::LineByLine);

          format_lines.clear();
        }
        if (in_chunks) {
          chunk_id = line.substr(get_chunk_token().size());
          if (chunk_id[0] == '#')
            chunk_id = chunk_id.substr(1);

          // Identifies the version for the chunk expectations
          // If no version is specified assigns '*'
          auto start = chunk_id.find("{");
          auto end = chunk_id.find("}");
          if (start != std::string::npos &&
              end != std::string::npos &&
              start < end) {
            version_id = chunk_id.substr(start + 1, end - start - 1);
            chunk_id = chunk_id.substr(0, start);
          } else {
            version_id = "*";
          }

          chunk_id = str_strip(chunk_id);

          if (chunk_id.find("<OUT>") == 0) {
            format = "OUT";
            chunk_id = chunk_id.substr(5);
            chunk_id = str_strip(chunk_id);
          } else if (chunk_id.find("<ERR>") == 0) {
            format = "ERR";
            chunk_id = chunk_id.substr(5);
            chunk_id = str_strip(chunk_id);
          } else
            format = "";
        }
      } else {
        if (format.empty()) {
          // When processing single line validations, lines as comments are
          // ignored
          if (!shcore::str_beginswith(line.c_str(),
                                      get_comment_token().c_str())) {
            line = str_strip(line);
            if (!line.empty()) {
              std::vector<std::string> tokens;
              tokens = split_string(line, "|", false);
              add_validation(chunk_id, version_id, tokens);
            }
          }
        } else {
          format_lines.push_back(line);
        }
      }
    }

    // Adds final formatted value if any
    if (format_lines.size()) {
      std::string value = multiline(format_lines);

      value = str_strip(value);

      if (format == "OUT")
        add_validation(chunk_id, version_id, {"", value, ""},
                       ValidationType::LineByLine);
      else if (format == "ERR")
        add_validation(chunk_id, version_id, {"", "", value},
                       ValidationType::LineByLine);
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

void Shell_script_tester::execute_script(const std::string& path,
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

      // Loads the validations
      if (!is_pre_script)
        load_validations(
            _new_format ? VAL_SCRIPT(path) : VALIDATION_SCRIPT(path),
            in_chunks);
    }

    // Process the file
    if (in_chunks) {
      _options->interactive = true;
      load_source_chunks(stream);
      for (size_t index = 0; index < _chunk_order.size(); index++) {
        // Prints debugging information
        std::string chunk_log = "CHUNK: " + _chunk_order[index];
        std::string splitter(chunk_log.length(), '-');
        output_handler.debug_print(makeyellow(splitter));
        output_handler.debug_print(makeyellow(chunk_log));
        output_handler.debug_print(makeyellow(splitter));

        // Executes the file line by line
        for (size_t chunk_item = 0;
             chunk_item < _chunks[_chunk_order[index]].size(); chunk_item++) {
          std::string line((_chunks[_chunk_order[index]])[chunk_item].second);

          // Execution context is at line level
          _custom_context =
              path + "@[" + _chunk_order[index] + "][" + line + "]";

          // There's chance to do preprocessing
          pre_process_line(path, line);

          if (testutil)
            testutil->set_test_execution_context(
                _filename, (_chunks[_chunk_order[index]])[chunk_item].first);
          execute(line);
        }

        execute("");
        execute("");

        // Validation contexts is at chunk level
        _custom_context = path + "@[" + _chunk_order[index] + " validation]";
        if (!validate(path, _chunk_order[index])) {
          if (debug_level() >= 3) {
            // Failure logs are printed on the fly in debug mode
            FAIL();
          }
          std::cerr << makeredbg(
                           "----------vvvv Failure Log Begin vvvv----------")
                    << std::endl;
          output_handler.flush_debug_log();
          std::cerr << makeredbg(
                           "----------^^^^ Failure Log End ^^^^------------")
                    << std::endl;
        } else {
          output_handler.whipe_debug_log();
        }
      }
    } else {
      _options->interactive = false;

      // Processes the script
      _interactive_shell->process_stream(stream, script, {}, true);

      // When path is empty it is processing a setup script
      // If an error is found it will be printed here
      if (path.empty()) {
        if (!output_handler.std_err.empty()) {
          SCOPED_TRACE(output_handler.std_err);
          std::string text("Setup Script: " + _setup_script);
          SCOPED_TRACE(text.c_str());
          ADD_FAILURE();
        }

        output_handler.wipe_all();
      } else {
        // If processing a tets script, performs the validations over it
        _options->interactive = true;
        if (!validate(script)) {
          if (debug_level() >= 3) {
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

void Shell_script_tester::validate_chunks(const std::string& path,
                                          const std::string& val_path,
                                          const std::string& pre_path) {
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

      load_validations(file_name, true);

      // Process the file
      _options->interactive = true;
      load_source_chunks(stream);
      for (size_t index = 0; index < _chunk_order.size(); index++) {
        // Prints debugging information
        std::string chunk_log = "CHUNK: " + _chunk_order[index];
        std::string splitter(chunk_log.length(), '-');
        output_handler.debug_print(makeyellow(splitter));
        output_handler.debug_print(makeyellow(chunk_log));
        output_handler.debug_print(makeyellow(splitter));

        // Executes the file line by line
        for (size_t chunk_item = 0;
             chunk_item < _chunks[_chunk_order[index]].size(); chunk_item++) {
          std::string line((_chunks[_chunk_order[index]])[chunk_item].second);

          // Execution context is at line level
          _custom_context =
              path + "@[" + _chunk_order[index] + "][" + line + "]";

          // There's chance to do preprocessing
          pre_process_line(path, line);

          if (testutil)
            testutil->set_test_execution_context(
                _filename, (_chunks[_chunk_order[index]])[chunk_item].first);
          execute(line);
        }

        execute("");
        execute("");

        // Validation contexts is at chunk level
        _custom_context = path + "@[" + _chunk_order[index] + " validation]";
        if (!validate(path, _chunk_order[index])) {
          if (debug_level() >= 3) {
            // Failure logs are printed on the fly in debug mode
            FAIL();
          }
          std::cerr << makeredbg(
                           "----------vvvv Failure Log Begin vvvv----------")
                    << std::endl;
          output_handler.flush_debug_log();
          std::cerr << makeredbg(
                           "----------^^^^ Failure Log End ^^^^------------")
                    << std::endl;
        } else {
          output_handler.whipe_debug_log();
        }
      }

      stream.close();
    } else {
      std::string text("Unable to open test script: " + script);
      SCOPED_TRACE(text.c_str());
      ADD_FAILURE();
    }
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}

// Searches for // Assumpsions: comment, if found, creates the __assumptions__
// array And processes the _assumption_script
void Shell_script_tester::process_setup(std::istream& stream) {
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

void Shell_script_tester::validate_batch(const std::string& script) {
  _filename = script;
  execute_script(script, false);
}

void Shell_js_script_tester::set_defaults() {
  Shell_script_tester::set_defaults();

  _interactive_shell->process_line("\\js");

  output_handler.wipe_all();
}

void Shell_py_script_tester::set_defaults() {
  Shell_script_tester::set_defaults();

  _interactive_shell->process_line("\\py");

  output_handler.wipe_all();
}

void Shell_script_tester::execute_setup() {
  const std::vector<std::string> argv;
  _interactive_shell->process_file(_setup_script, argv);
}

// Append option to the end of the given config file.
void Shell_script_tester::add_to_cfg_file(const std::string& cfgfile_path,
                                          const std::string& option) {
  std::ofstream cfgfile(cfgfile_path, std::ios_base::app);
  cfgfile << option << std::endl;
  cfgfile.close();
}

// Delete lines with the option from the given config file.
void Shell_script_tester::remove_from_cfg_file(const std::string& cfgfile_path,
                                               const std::string& option) {
  std::string new_cfgfile_path = cfgfile_path + ".new";
  std::ofstream new_cfgfile(new_cfgfile_path);
  std::ifstream cfgfile(cfgfile_path);
  std::string line;
  while (std::getline(cfgfile, line)) {
    if (line.find(option) != 0)
      new_cfgfile << line << std::endl;
  }
  cfgfile.close();
  new_cfgfile.close();
  std::remove(cfgfile_path.c_str());
  std::rename(new_cfgfile_path.c_str(), cfgfile_path.c_str());
}

// Check whether openssl executable is accessible via PATH
bool Shell_script_tester::has_openssl_binary() {
  const char* argv[] = {"openssl", "version", nullptr};
  shcore::Process_launcher p(argv);
  p.start();
  std::string s = p.read_line();
  if (p.wait() == 0) {
    return shcore::str_beginswith(s, "OpenSSL");
  }
  return false;
}
