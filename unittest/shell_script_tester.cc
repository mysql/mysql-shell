/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include "shell_script_tester.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"
#include "shellcore/ishell_core.h"

using namespace shcore;

Shell_script_tester::Shell_script_tester() {
  _shell_scripts_home = MYSQLX_SOURCE_HOME;
  _shell_scripts_home += "/unittest/scripts";
  _new_format = false;
}

void Shell_script_tester::SetUp() {
  Crud_test_wrapper::SetUp();
}

void Shell_script_tester::TearDown() {
  Crud_test_wrapper::TearDown();
}

void Shell_script_tester::set_config_folder(const std::string &name) {
  _shell_scripts_home += "/" + name;

  // Currently hardcoded since scripts are on the shell repo
  // but can easily be updated to be setup on an ENV VAR so
  // the scripts can by dynamically imported from the dev-api docs
  // with the sctract tool Jan is working on
  _scripts_home = _shell_scripts_home + "/scripts";
}

void Shell_script_tester::set_setup_script(const std::string &name) {
  _setup_script = _shell_scripts_home + "/setup/" + name;
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

    execute(token);

    std::string value = output_handler.std_out;

    value = str_strip(value);

    updated.replace(start, end - start + 3, value);

    start = updated.find("<<<");
  }

  output_handler.wipe_out();

  return updated;
}

/**
 * Multiple value validation
 * To be used on a single line
 * Line may have an entry that may have one of several values, i.e. on an expected line like:
 *
 * My text line is {{empty|full}}
 *
 * Ths function would return true whenever the actual line is any of
 *
 * My text line is empty
 * My text line is full
 */
bool Shell_script_tester::multi_value_compare(const std::string& expected, const std::string &actual) {
  bool ret_val = false;

  size_t start;
  size_t end;

  start = expected.find("{{");

  if (start != std::string::npos) {
    end = expected.find("}}");

    std::string pre = expected.substr(0, start);
    std::string post = expected.substr(end + 2);
    std::string opts = expected.substr(start + 2, end - (start + 2));
    auto options = shcore::split_string(opts, "|");

    for (auto item : options) {
      std::string exp = pre + item + post;
      if ((ret_val = (exp == actual)))
        break;
    }
  } else
    ret_val = (expected == actual);

  return ret_val;
}

bool Shell_script_tester::validate_line_by_line(const std::string& context, const std::string &chunk_id, const std::string &stream, const std::string& expected, const std::string &actual) {
  bool ret_val = true;
  auto expected_lines = shcore::split_string(expected, "\n");
  auto actual_lines = shcore::split_string(actual, "\n");

  // Identifies the index of the actual line containing the first expected line
  size_t actual_index = 0;
  while (actual_index < actual_lines.size()) {
    if (actual_lines[actual_index].find(expected_lines[0]) != std::string::npos)
      break;
    else
      actual_index++;
  }

  if (actual_index < actual_lines.size()) {
    size_t expected_index;
    for (expected_index = 0; expected_index < expected_lines.size(); expected_index++) {
      // if there are less actual lines than the ones expected
      if ((actual_index + expected_index) >= actual_lines.size()){
        SCOPED_TRACE(stream + " actual: " + actual);
        expected_lines[expected_index] += "<------ MISSING";
        SCOPED_TRACE(stream + " expected lines missing: " +
                     shcore::str_join(expected_lines, "\n"));
        ADD_FAILURE();
        ret_val = false;
        break;
      }

      auto act_str = str_rstrip(actual_lines[actual_index + expected_index]);
      auto exp_str = str_rstrip(expected_lines[expected_index]);
      if (!multi_value_compare(exp_str, act_str)) {
        SCOPED_TRACE("File: " + context);
        SCOPED_TRACE("Executing: " + chunk_id);
        SCOPED_TRACE(stream + " actual: " + actual);

        expected_lines[expected_index] += "<------ INCONSISTENCY";

        SCOPED_TRACE(stream + " inconsistent: " +
                     shcore::str_join(expected_lines, "\n"));
        ADD_FAILURE();
        ret_val = false;
        break;
      }
    }
  }

  return ret_val;
}

bool Shell_script_tester::validate(const std::string& context, const std::string &chunk_id) {
  bool ret_val = true;
  if (_chunk_validations.count(chunk_id)) {
    Validation_t validations = _chunk_validations[chunk_id];

    std::string original_std_out = output_handler.std_out;
    std::string original_std_err = output_handler.std_err;

    for (size_t valindex = 0; valindex < validations.size(); valindex++) {
      // Validation goes against validation code
      if (!validations[valindex].code.empty()) {
        // Before cleaning up, prints any error found on the script execution
        if (valindex == 0 && !original_std_err.empty()) {
          ret_val = false;
          SCOPED_TRACE("File: " + context);
          SCOPED_TRACE("Unexpected Error: " + original_std_err);
          ADD_FAILURE();
        }

        output_handler.wipe_all();
        std::string backup = _custom_context;
        _custom_context += "[" + validations[valindex].code + "]";
        execute(validations[valindex].code);
        _custom_context = backup;

        original_std_err = output_handler.std_err;
        original_std_out = output_handler.std_out;

        output_handler.wipe_all();
      }

      // Validates unexpected error
      if (validations[valindex].expected_error.empty() &&
          !original_std_err.empty()) {
        ret_val = false;
        SCOPED_TRACE("File: " + context);
        SCOPED_TRACE("Executing: " + chunk_id);
        SCOPED_TRACE("Unexpected Error: " + original_std_err);
        ADD_FAILURE();
      }

      // Validates expected output if any
      if (!validations[valindex].expected_output.empty()) {
        std::string out = validations[valindex].expected_output;

        out = resolve_string(out);

        if (out != "*") {
          if (validations[valindex].type == ValidationType::Simple) {
            SCOPED_TRACE("File: " + context);
            SCOPED_TRACE("Executing: " + chunk_id);
            SCOPED_TRACE("STDOUT missing: " + out);
            SCOPED_TRACE("STDOUT actual: " + original_std_out);
            if (original_std_out.find(out) == std::string::npos) {
              ret_val = false;
              ADD_FAILURE();
            }
          } else {
            ret_val = validate_line_by_line(context, chunk_id, "STDOUT", out, original_std_out);
          }
        }
      }

      // Validates unexpected output if any
      if (!validations[valindex].unexpected_output.empty()) {
        std::string out = validations[valindex].unexpected_output;

        out = resolve_string(out);

        SCOPED_TRACE("File: " + context);
        SCOPED_TRACE("Executing: " + chunk_id);
        SCOPED_TRACE("STDOUT unexpected: " + out);
        SCOPED_TRACE("STDOUT actual: " + original_std_out);
        if (original_std_out.find(out) != std::string::npos) {
          ret_val = false;
          ADD_FAILURE();
        }
      }

      // Validates expected error if any
      if (!validations[valindex].expected_error.empty()) {
        std::string error = validations[valindex].expected_error;

        error = resolve_string(error);

        if (error != "*") {
          if (validations[valindex].type == ValidationType::Simple) {
            SCOPED_TRACE("File: " + context);
            SCOPED_TRACE("Executing: " + chunk_id);
            SCOPED_TRACE("STDERR missing: " + error);
            SCOPED_TRACE("STDERR actual: " + original_std_err);
            if (original_std_err.find(error) == std::string::npos) {
              ret_val = false;
              ADD_FAILURE();
            }
          } else {
            ret_val = validate_line_by_line(context, chunk_id, "STDERR", error, original_std_out);
          }
        }
      }
    }
    output_handler.wipe_all();
  } else {
    // Every defined validation chunk MUST have validations
    // or should not be defined as a validation chunk
    if (chunk_id != "__global__") {
      SCOPED_TRACE("File: " + context);
      SCOPED_TRACE("Executing: " + chunk_id);
      SCOPED_TRACE("MISSING VALIDATIONS!!!");
      ADD_FAILURE();
      ret_val = false;
    } else
      output_handler.wipe_all();
  }

  return ret_val;
}

void Shell_script_tester::validate_interactive(const std::string& script) {
  try {
    execute_script(script, true);
  } catch (std::exception& e) {
    std::string error = e.what();
  }
}

void Shell_script_tester::load_source_chunks(std::istream & stream) {
  std::string chunk_id = "__global__";
  std::vector<std::string> current_chunk;

  while (!stream.eof()) {
    std::string line;
    std::getline(stream, line);

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
        current_chunk = std::vector<std::string>();

      // Only adds the lines that are NO snippet specifier
      if (line.find("//! [") != 0)
        current_chunk.push_back(line);
    }
  }

  // Inserts the remaining code chunk
  if (!current_chunk.empty()) {
    _chunks[chunk_id] = current_chunk;
    _chunk_order.push_back(chunk_id);
  }
}

void Shell_script_tester::add_validation(const std::string &chunk_id, const std::vector<std::string>& source, ValidationType type) {
  if (source.size() == 3) {
    if (!_chunk_validations.count(chunk_id))
      _chunk_validations[chunk_id] = Validation_t();

    // Line by line validation may be complement of an existing validation
    if (type == ValidationType::LineByLine) {
      if (_chunk_validations[chunk_id].size() == 0)
        _chunk_validations[chunk_id].push_back(Validation(source, type));
      else if (_chunk_validations[chunk_id].size() == 1) {
        if (_chunk_validations[chunk_id].at(0).type == ValidationType::LineByLine) {
          if (source[1].size() > 0)
            _chunk_validations[chunk_id].at(0).expected_output = source[1];
          if (source[2].size() > 0)
            _chunk_validations[chunk_id].at(0).expected_error = source[2];
        } else
          throw std::runtime_error("Unable to mix Single and Line by Line validations");
      } else
        throw std::runtime_error("Unable to mix Sinngle and Line by Line validations");
    } else
      _chunk_validations[chunk_id].push_back(Validation(source, type));
  }
}

void Shell_script_tester::load_validations(const std::string& path, bool in_chunks) {
  std::ifstream file(path.c_str());
  std::string chunk_id = "__global__";
  std::string format;
  std::vector<std::string> format_lines;

  _chunk_validations.clear();

  if (!file.fail()) {
    while (!file.eof()) {
      std::string line;
      std::getline(file, line);

      if (line.find(get_chunk_token()) == 0) {
        if (format_lines.size()) {
          std::string value = shcore::str_join(format_lines, "\n");

          value = str_strip(value);

          if (format == "OUT")
            add_validation(chunk_id, {"", value, ""}, ValidationType::LineByLine);
          else if (format == "ERR")
            add_validation(chunk_id, {"", "", value}, ValidationType::LineByLine);

          format_lines.clear();
        }
        if (in_chunks) {
          chunk_id = line.substr(get_chunk_token().size());
          if (chunk_id[0] == '#')
            chunk_id = chunk_id.substr(1);

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
          line = str_strip(line);
          if (!line.empty()) {
            std::vector<std::string> tokens;
            tokens = split_string(line, "|", false);
            add_validation(chunk_id, tokens);
          }
        } else {
          format_lines.push_back(line);
        }
      }
    }

    // Adds final formatted value if any
    if (format_lines.size()) {
      std::string value = shcore::str_join(format_lines, "\n");

      value = str_strip(value);

      if (format == "OUT")
        add_validation(chunk_id, {"", value, ""}, ValidationType::LineByLine);
      else if (format == "ERR")
        add_validation(chunk_id, {"", "", value}, ValidationType::LineByLine);
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

void Shell_script_tester::execute_script(const std::string& path, bool in_chunks, bool is_pre_script) {
  // If no path is provided then executes the setup script
  std::string script(path.empty() ? _setup_script : is_pre_script ? PRE_SCRIPT(path) : _new_format ? NEW_TEST_SCRIPT(path) : TEST_SCRIPT(path));
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
        load_validations(_new_format ? VAL_SCRIPT(path) : VALIDATION_SCRIPT(path), in_chunks);
    }

    // Process the file
    if (in_chunks) {
      (*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE] = shcore::Value::True();
      load_source_chunks(stream);
      for (size_t index = 0; index < _chunk_order.size(); index++) {

        // Prints debugging information
        std::string chunk_log = "CHUNK: " + _chunk_order[index];
        std::string splitter(chunk_log.length(), '-');
        output_handler.debug_print(splitter);
        output_handler.debug_print(chunk_log);
        output_handler.debug_print(splitter);

        // Executes the file line by line
        for (size_t chunk_item = 0; chunk_item < _chunks[_chunk_order[index]].size(); chunk_item++) {
          std::string line((_chunks[_chunk_order[index]])[chunk_item]);

          // Execution context is at line level
          _custom_context = path + "@[" + _chunk_order[index] + "][" + line + "]";

          // There's chance to do preprocessing
          pre_process_line(path, line);

          execute(line);
        }

        execute("");
        execute("");

        // Validation contexts is at chunk level
        _custom_context = path + "@[" + _chunk_order[index] + " validation]";
        if (!validate(path, _chunk_order[index])) {
          std::cerr << "---------- Failure Log Begin ----------" << std::endl;
          output_handler.flush_debug_log();
          std::cerr << "---------- Failure Log End ------------" << std::endl;
        }
        else
          output_handler.whipe_debug_log();
      }
    } else {
      (*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE] = shcore::Value::False();

      // Processes the script
      _interactive_shell->process_stream(stream, script, {});

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
        (*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE] = shcore::Value::True();
        if (!validate(script)) {
          std::cerr << "---------- Failure Log ----------" << std::endl;
          output_handler.flush_debug_log();
          std::cerr << "---------------------------------" << std::endl;
        }
        else
          output_handler.whipe_debug_log();
      }
    }

    stream.close();
  } else {
    std::string text("Unable to open test script: " + script);
    SCOPED_TRACE(text.c_str());
    ADD_FAILURE();
  }
}

// Searches for // Assumpsions: comment, if found, creates the __assumptions__ array
// And processes the _assumption_script
void Shell_script_tester::process_setup(std::istream & stream) {
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
        execute_script(); // Executes the active setup script
    } else
      done = true;
  }

  // Once the assumptions are processed, rewinds the read position
  // To the beggining of the script
  stream.clear(); // To clean up the eof flag in case it was set
  stream.seekg(0, stream.beg);
}

void Shell_script_tester::validate_batch(const std::string& script) {
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

void Shell_script_tester::execute_setup(){
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
  while(std::getline(cfgfile, line)) {
    if (line.find(option) != 0)
      new_cfgfile << line << std::endl;
  }
  cfgfile.close();
  new_cfgfile.close();
  std::remove(cfgfile_path.c_str());
  std::rename(new_cfgfile_path.c_str(), cfgfile_path.c_str());
}
