/* Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.

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
#include "shellcore/ishell_core.h"
#include "utils/process_launcher.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

using namespace shcore;
extern "C" const char* g_test_home;
extern bool g_generate_validation_file;


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
                                   const std::string& chunk_id,
                                   bool optional) {
  std::string original_std_out = output_handler.std_out;
  std::string original_std_err = output_handler.std_err;
  size_t out_position = 0;
  size_t err_position = 0;

  if (_chunk_validations.find(chunk_id) != _chunk_validations.end()) {
    bool expect_failures = false;

    // Identifies the validations to be done based on the context
    std::vector<std::shared_ptr<Validation>> validations;
    for (const auto& val : _chunk_validations[chunk_id]) {
      bool enabled = false;
      try {
        enabled = context_enabled(val->def->context);
      } catch (const std::invalid_argument& e) {
        ADD_FAILURE_AT("validation file", val->def->linenum)
            << "ERROR EVALUATING VALIDATION CONTEXT: " << e.what() << "\n"
            << "\tCHUNK: " << val->def->line << "\n";
      }

      if (enabled) {
        validations.push_back(val);

        if (!val->expected_error.empty())
          expect_failures = true;
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
          ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id].code[0].first)
              << makered("\tUnexpected Error: " + original_std_err) << "\n";
          return false;
        }

        output_handler.wipe_all();
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
      }

      // Validates unexpected error
      if (!expect_failures && !original_std_err.empty()) {
        ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id].code[0].first)
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
              ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id].code[0].first)
                  << "while executing chunk: " + _chunks[chunk_id].def->line
                  << "\n"
                  << makeyellow("\tSTDOUT missing: ") << out << "\n"
                  << makeyellow("\tSTDOUT actual: ") +
                         original_std_out.substr(out_position)
                  << "\n"
                  << makeyellow("\tSTDOUT original: ") + original_std_out
                  << "\n";
              return false;
            } else {
              // Consumes the already found output
              out_position = pos + out.length();
            }
          } else {
            if (!validate_line_by_line(context, chunk_id, "STDOUT", out,
                                       original_std_out))
              return false;
          }
        }
      }

      // Validates unexpected output if any
      if (!validations[valindex]->unexpected_output.empty()) {
        std::string out = validations[valindex]->unexpected_output;

        out = resolve_string(out);

        if (original_std_out.find(out) != std::string::npos) {
          ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id].code[0].first)
              << "while executing chunk: " + _chunks[chunk_id].def->line << "\n"
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
              ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id].code[0].first)
                  << "while executing chunk: " + _chunks[chunk_id].def->line
                  << "\n"
                  << makeyellow("\tSTDERR missing: ") + error << "\n"
                  << makeyellow("\tSTDERR actual: ") +
                         original_std_err.substr(err_position)
                  << "\n"
                  << makeyellow("\tSTDERR original: ") + original_std_err
                  << "\n";
              return false;
            } else {
              // Consumes the already found error
              err_position = pos + error.length();
            }
          } else {
            if (!validate_line_by_line(context, chunk_id, "STDERR", error,
                                       original_std_err))
              return false;
          }
        }
      }
    }

    if (validations.empty()) {
      ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id].code[0].first)
          << makered("MISSING VALIDATIONS FOR CHUNK ")
          << _chunks[chunk_id].def->line << "\n"
          << makeyellow("\tSTDOUT: ") << original_std_out << "\n"
          << makeyellow("\tSTDERR: ") << original_std_err << "\n";
      return false;
    }
    output_handler.wipe_all();
  } else {
    // There were errors
    if (!original_std_err.empty()) {
      ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id].code[0].first)
          << "while executing chunk: " + _chunks[chunk_id].def->line << "\n"
          << makered("\tUnexpected Error: ") + original_std_err << "\n";
    } else if (!optional && _chunks.find(chunk_id) != _chunks.end()) {
    // The error is that there are no validations
      ADD_FAILURE_AT(_filename.c_str(), _chunks[chunk_id].code[0].first)
          << makered("MISSING VALIDATIONS FOR CHUNK ")
          << _chunks[chunk_id].def->line << "\n"
          << makeyellow("\tSTDOUT: ") << original_std_out << "\n"
          << makeyellow("\tSTDERR: ") << original_std_err << "\n";
    }
    output_handler.wipe_all();
  }

  return true;
}

void Shell_script_tester::validate_interactive(const std::string& script) {
  _filename = script;
  try {
    execute_script(script, true);
    if (testutil->test_skipped()) {
      SKIP_TEST(testutil->test_skip_reason());
    }
  } catch (std::exception& e) {
    std::string error = e.what();
    FAIL() << makered("Unexpected exception excuting test script: ") << e.what()
           << "\n";
  }
}

void Shell_script_tester::load_source_chunks(const std::string& path,
                                             std::istream& stream) {
  Chunk_t chunk;
  chunk.def->id = "__global__";
  chunk.def->validation = ValidationType::Optional;
  int linenum = 0;

  while (!stream.eof()) {
    std::string line;
    std::getline(stream, line);
    linenum++;

    line = str_rstrip(line, "\r\n");

    std::shared_ptr<Chunk_definition> chunk_def = load_chunk_definition(line);
    if (chunk_def) {
      chunk_def->linenum = linenum;
      add_source_chunk(path, chunk);

      // Starts the new chunk
      chunk = Chunk_t();
      chunk.def = chunk_def;
    } else {
      // Only adds the lines that are NO snippet specifier
      if (line.find("//! [") != 0)
        chunk.code.push_back({linenum, line});
    }
  }

  // Inserts the remaining code chunk
  add_source_chunk(path, chunk);
}

void Shell_script_tester::add_source_chunk(const std::string& path,
                                           const Chunk_t& chunk) {
  if (!chunk.code.empty()) {
    if (_chunks.find(chunk.def->id) == _chunks.end()) {
      _chunks[chunk.def->id] = chunk;
      _chunk_order.push_back(chunk.def->id);
    } else {
      ADD_FAILURE_AT(path.c_str(), chunk.code[0].first - 1)
          << makered("REDEFINITION OF CHUNK: \"") + chunk.def->line << "\"\n"
          << "\tInitially defined at line: "
          << (_chunks[chunk.def->id].code[0].first - 1) << "\n";
    }
  }
}

void Shell_script_tester::add_validation(
    const std::shared_ptr<Chunk_definition>& chunk_def,
    const std::vector<std::string>& source) {
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
    const std::string& line) {
  std::shared_ptr<Chunk_definition> ret_val;

  if (line.find(get_chunk_token()) == 0) {
    std::string chunk_id;
    std::string chunk_context;
    ValidationType val_type = ValidationType::Simple;
    std::string stream;

    chunk_id = line.substr(get_chunk_token().size());
    if (chunk_id[0] == '#')
      chunk_id = chunk_id.substr(1);

    // Identifies the version for the chunk expectations
    // If no version is specified assigns '*'
    auto start = chunk_id.find("{");
    auto end = chunk_id.find("}");

    if (start != std::string::npos && end != std::string::npos && start < end) {
      chunk_context = chunk_id.substr(start + 1, end - start - 1);
      chunk_id = chunk_id.substr(0, start);
    }

    chunk_id = str_strip(chunk_id);

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
    } else if (chunk_id.find("<>") == 0) {
      stream = "";
      chunk_id = chunk_id.substr(2);
      chunk_id = str_strip(chunk_id);
      val_type = ValidationType::Optional;
    }

    ret_val.reset(new Chunk_definition());

    ret_val->line = line;
    ret_val->id = chunk_id;
    ret_val->context = chunk_context;
    ret_val->validation = val_type;
    ret_val->stream = stream;
  }

  return ret_val;
}

void Shell_script_tester::load_validations(const std::string& path) {
  std::ifstream file(path.c_str());
  std::vector<std::string> lines;
  bool skip_chunk = false;

  _chunk_validations.clear();

  bool chunk_verification = !_chunk_order.empty();
  size_t chunk_index = 0;
  Chunk_t *current_chunk = &_chunks[_chunk_order[chunk_index]];

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

          value = str_strip(value);

          if (current_val_def->stream == "OUT")
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
          if (_chunks.find(current_val_def->id) == _chunks.end()) {
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

          while (optional && !match && chunk_index < _chunk_order.size()) {
            chunk_index++;
            current_chunk = &_chunks[_chunk_order[chunk_index]];
            optional = current_chunk->is_validation_optional();
            match = current_val_def->id == current_chunk->def->id;
          }

          if (!optional && !match) {
            ADD_FAILURE_AT(path.c_str(), line_no)
                << makered("EXPECTED VALIDATIONS FOR CHUNK ")
                << current_chunk->def->line << "\n"
                << "INSTEAD FOUND FOR CHUNK " << current_val_def->line << "\n"
                << "\tLINE: " << line_no << "\n";
            skip_chunk = true;
            continue;
          }

          if (chunk_index >=  _chunk_order.size()) {
            ADD_FAILURE_AT(path.c_str(), line_no)
                << makered("UNEXPECTED VALIDATIONS FOR CHUNK ") << current_val_def->line
                << "\n"
                << "\tLINE: " << line_no << "\n";
            skip_chunk = true;
          }
        }

        // If the new chunk is wrong, ignores it
        //if (!skip_chunk)
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

      value = str_strip(value);

      if (current_val_def->stream == "OUT")
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
    }

    // Process the file
    if (in_chunks) {
      _options->interactive = true;
      load_source_chunks(script, stream);

      // Loads the validations
      load_validations(_new_format ? VAL_SCRIPT(path)
                                   : VALIDATION_SCRIPT(path));

      // Abort the script processing if something went wrong on the validation
      // loading
      if (::testing::Test::HasFailure() || testutil->test_skipped())
        return;

      std::ofstream ofile;
      if (g_generate_validation_file) {
        std::string vfile_name = VALIDATION_SCRIPT(path);
        if (!shcore::file_exists(vfile_name)) {
          ofile.open(vfile_name, std::ofstream::out | std::ofstream::trunc);
        } else {
          vfile_name.append(".new");
          ofile.open(vfile_name, std::ofstream::out | std::ofstream::trunc);
        }
      }

      for (size_t index = 0; index < _chunk_order.size(); index++) {
        // Prints debugging information
        std::string chunk_log = "CHUNK: " + _chunk_order[index];
        std::string splitter(chunk_log.length(), '-');
        output_handler.debug_print(makeyellow(splitter));
        output_handler.debug_print(makeyellow(chunk_log));
        output_handler.debug_print(makeyellow(splitter));

        // Gets the chunks for the next id
        auto& chunk = _chunks[_chunk_order[index]];

        bool enabled;
        try {
          enabled = context_enabled(chunk.def->context);
        } catch (const std::invalid_argument& e) {
          ADD_FAILURE_AT(script.c_str(), chunk.code[0].first)
              << makered("ERROR EVALUATING CONTEXT: ") << e.what() << "\n"
              << "\tCHUNK: " << chunk.def->line << "\n";
          break;
        }

        // Executes the file line by line
        if (enabled) {
          auto& code = chunk.code;
          std::string full_statement;
          for (size_t chunk_item = 0; chunk_item < code.size(); chunk_item++) {
            std::string line(code[chunk_item].second);

            full_statement.append(line);
            // Execution context is at line (statement actually) level
            _custom_context = path + "@[" + _chunk_order[index] + "][" +
                              std::to_string(chunk.code[chunk_item].first) +
                              ":" + full_statement + "]";

            // There's chance to do preprocessing
            pre_process_line(path, line);

            if (testutil)
              testutil->set_test_execution_context(_filename,
                                                   code[chunk_item].first);
            execute(chunk.code[chunk_item].first, line);

            if (testutil->test_skipped())
              return;

            if (_interactive_shell->input_state() == shcore::Input_state::Ok)
              full_statement.clear();
            else
              full_statement.append("\n");
          }

          execute("");
          execute("");

          if (g_generate_validation_file) {
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

            output_handler.wipe_all();
          } else {
            // Validation contexts is at chunk level
            _custom_context =
                path + "@[" + _chunk_order[index] + " validation]";
            if (!validate(path, _chunk_order[index],
                chunk.is_validation_optional())) {
              if (g_test_trace_scripts > 1) {
                // Failure logs are printed on the fly in debug mode
                FAIL();
              }
              std::cerr
                  << makeredbg(
                         "----------vvvv Failure Log Begin vvvv----------")
                  << std::endl;
              output_handler.flush_debug_log();
              std::cerr
                  << makeredbg(
                         "----------^^^^ Failure Log End ^^^^------------")
                  << std::endl;
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
      if (::testing::Test::HasFailure() || testutil->test_skipped())
        return;

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
          if (g_test_trace_scripts > 1) {
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

      // Process the file
      _options->interactive = true;
      load_source_chunks(script, stream);
      load_validations(file_name);
      for (size_t index = 0; index < _chunk_order.size(); index++) {
        // Prints debugging information
        std::string chunk_log = "CHUNK: " + _chunk_order[index];
        std::string splitter(chunk_log.length(), '-');
        output_handler.debug_print(makeyellow(splitter));
        output_handler.debug_print(makeyellow(chunk_log));
        output_handler.debug_print(makeyellow(splitter));

        auto chunk = _chunks[_chunk_order[index]];

        bool enabled;
        try {
          enabled = context_enabled(chunk.def->context);
        } catch (const std::invalid_argument& e) {
          ADD_FAILURE_AT(script.c_str(), chunk.code[0].first)
              << makered("ERROR EVALUATING CONTEXT: ") << e.what() << "\n"
              << "\tCHUNK: " << chunk.def->line << "\n";
        }

        // Executes the file line by line
        if (enabled) {
          auto& code = chunk.code;
          for (size_t chunk_item = 0; chunk_item < code.size(); chunk_item++) {
            std::string line(code[chunk_item].second);

            // Execution context is at line level
            _custom_context =
                path + "@[" + _chunk_order[index] + "][" + line + "]";

            // There's chance to do preprocessing
            pre_process_line(path, line);

            if (testutil)
              testutil->set_test_execution_context(_filename,
                                                   (code[chunk_item].first));
            execute(chunk.code[chunk_item].first, line);

            if (testutil->test_skipped())
              return;
          }

          execute("");
          execute("");

          // Validation contexts is at chunk level
          _custom_context = path + "@[" + _chunk_order[index] + " validation]";
          if (!validate(path, _chunk_order[index],
            chunk.is_validation_optional())) {
            if (g_test_trace_scripts > 1) {
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

  std::string code = "var __version = '" + _target_server_version.get_base() + "'";
  exec_and_out_equals(code);
}

void Shell_py_script_tester::set_defaults() {
  Shell_script_tester::set_defaults();

  _interactive_shell->process_line("\\py");

  output_handler.wipe_all();

  std::string code = "__version = '" + _target_server_version.get_base() + "'";
  exec_and_out_equals(code);
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

bool Shell_script_tester::context_enabled(const std::string& context) {
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
      std::string fname = shcore::get_member_name("versionCheck",
                                                  get_naming_style());
      std::string new_func = "testutil." + fname +
                             "(__version, '" + op + "', '" + ver + "')";

      code = shcore::str_replace(code, old_func, new_func);
      function_pos = code.find("VER(");
    }

    output_handler.wipe_out();

    std::string value;
    execute(code);
    value = output_handler.std_out;
    value = str_strip(value);

    if (value == "true") {
      ret_val = true;
    } else if (value == "false") {
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

void Shell_script_tester::execute(int location, const std::string& code) {
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


void Shell_script_tester::execute(const std::string& code) {
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
