/* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.

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
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "utils/utils_file.h"
#include "shellcore/ishell_core.h"

Shell_script_tester::Shell_script_tester()
{
  _shell_scripts_home = MYSQLX_SOURCE_HOME;
  _shell_scripts_home += "/unittest/scripts";
}

void Shell_script_tester::SetUp()
{
  Crud_test_wrapper::SetUp();
}

void Shell_script_tester::TearDown()
{
  Crud_test_wrapper::TearDown();
}

void Shell_script_tester::set_config_folder(const std::string &name)
{
  _shell_scripts_home += "/" + name;

  // Currently hardcoded since scripts are on the shell repo
  // but can easily be updated to be setup on an ENV VAR so
  // the scripts can by dynamically imported from the dev-api docs
  // with the sctract tool Jan is working on
  _scripts_home = _shell_scripts_home + "/scripts";
}

void Shell_script_tester::set_setup_script(const std::string &name)
{
  _setup_script = _shell_scripts_home + "/setup/" + name;
}

void Shell_script_tester::validate(const std::string& context, const std::string &chunk_id)
{
  if (_chunk_validations.count(chunk_id))
  {
    Validation_t* validations = _chunk_validations[chunk_id];

    for (size_t valindex = 0; valindex < validations->size(); valindex++)
    {
      // Validation goes against validation code
      if (!(*validations)[valindex].code.empty())
      {
        // Before cleaning up, prints any error found on the script execution
        if (valindex == 0 && !output_handler.std_err.empty())
        {
          SCOPED_TRACE("File: " + context);
          SCOPED_TRACE("Unexpexted Error: " + output_handler.std_err);
          ADD_FAILURE();
        }

        output_handler.wipe_all();
        execute((*validations)[valindex].code);
      }

      // Validates unexpected error
      if ((*validations)[valindex].expected_error.empty() &&
         !output_handler.std_err.empty())
      {
        SCOPED_TRACE("File: " + context);
        SCOPED_TRACE("Executing: " + chunk_id);
        SCOPED_TRACE("Unexpexted Error: " + output_handler.std_err);
        ADD_FAILURE();
      }

      // Validates expected output if any
      if (!(*validations)[valindex].expected_output.empty())
      {
        std::string out = (*validations)[valindex].expected_output;

        SCOPED_TRACE("File: " + context);
        SCOPED_TRACE("Executing: " + chunk_id);
        SCOPED_TRACE("STDOUT missing: " + out);
        SCOPED_TRACE("STDOUT actual: " + output_handler.std_out);
        EXPECT_NE(-1, int(output_handler.std_out.find(out)));
      }

      // Validates unexpected output if any
      if (!(*validations)[valindex].unexpected_output.empty())
      {
        std::string out = (*validations)[valindex].unexpected_output;

        SCOPED_TRACE("File: " + context);
        SCOPED_TRACE("Executing: " + chunk_id);
        SCOPED_TRACE("STDOUT unexpected: " + out);
        SCOPED_TRACE("STDOUT actual: " + output_handler.std_out);
        EXPECT_EQ(-1, int(output_handler.std_out.find(out)));
      }

      // Validates expected error if any
      if (!(*validations)[valindex].expected_error.empty())
      {
        std::string error = (*validations)[valindex].expected_error;

        SCOPED_TRACE("File: " + context);
        SCOPED_TRACE("Executing: " + chunk_id);
        SCOPED_TRACE("STDOUT missing: " + error);
        SCOPED_TRACE("STDOUT actual: " + output_handler.std_err);
        EXPECT_NE(-1, int(output_handler.std_err.find(error)));
      }
    }
    output_handler.wipe_all();
  }
  else
  {
    // Every defined validation chunk MUST have validations
    // or should not be defined as a validation chunk
    if (chunk_id != "__global__")
    {
      SCOPED_TRACE("File: " + context);
      SCOPED_TRACE("Executing: " + chunk_id);
      SCOPED_TRACE("MISSING VALIDATIONS!!!");
      ADD_FAILURE();
    }
  }
}

void Shell_script_tester::validate_interactive(const std::string& script)
{
  execute_script(script, true);
}

void Shell_script_tester::load_source_chunks(std::istream & stream)
{
  std::string chunk_id = "__global__";
  std::vector<std::string> * current_chunk = NULL;

  while (!stream.eof())
  {
    std::string line;
    std::getline(stream, line);

    if (line.find(get_chunk_token()) == 0)
    {
      if (current_chunk)
      {
        _chunks[chunk_id] = current_chunk;
        _chunk_order.push_back(chunk_id);
      }

      chunk_id = line;
      boost::trim(chunk_id);
      current_chunk = NULL;
    }
    else
    {
      if (!current_chunk)
        current_chunk = new std::vector<std::string>();

      current_chunk->push_back(line);
    }
  }

  // Inserts the remaining code chunk
  if (current_chunk)
  {
    _chunks[chunk_id] = current_chunk;
    _chunk_order.push_back(chunk_id);
  }
}

void Shell_script_tester::load_validations(const std::string& path, bool in_chunks)
{
  std::ifstream file(path.c_str());
  std::string chunk_id = "__global__";

  _chunk_validations.clear();

  if (!file.fail())
  {
    while (!file.eof())
    {
      std::string line;
      std::getline(file, line);

      boost::trim(line);
      if (!line.empty())
      {
        if (line.find(get_chunk_token()) == 0)
        {
          if (in_chunks)
          {
            chunk_id = line;
            boost::trim(chunk_id);
          }
        }
        else
        {
          std::vector<std::string> tokens;
          boost::split(tokens, line, boost::is_any_of("|"), boost::token_compress_off);

          if (!_chunk_validations.count(chunk_id))
            _chunk_validations[chunk_id] = new Validation_t();

          _chunk_validations[chunk_id]->push_back(Validation(tokens));
        }
      }
    }

    file.close();
  }
}

void Shell_script_tester::execute_script(const std::string& path, bool in_chunks)
{
  // If no path is provided then executes the setup script
  std::string script(path.empty() ? _setup_script : TEST_SCRIPT(path));
  std::ifstream stream(script.c_str());

  if (!stream.fail())
  {
    // When it is a test script, preprocesses it so the
    // right execution scenario is in place
    if (!path.empty())
    {
      process_setup(stream);

      // Loads the validations
      load_validations(VALIDATION_SCRIPT(path), in_chunks);
    }

    // Process the file
    if (in_chunks)
    {
      (*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE] = shcore::Value::True();
      load_source_chunks(stream);
      for (size_t index = 0; index < _chunk_order.size(); index++)
      {
        // Executes the file line by line
        for (size_t chunk_item = 0; chunk_item < _chunks[_chunk_order[index]]->size(); chunk_item++)
          execute((*_chunks[_chunk_order[index]])[chunk_item]);

        execute("");
        execute("");

        validate(path, _chunk_order[index]);
      }
    }
    else
    {
      (*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE] = shcore::Value::False();

      // Processes the script
      _interactive_shell->process_stream(stream, script);

      // When path is empty it is processing a setup script
      // If an error is found it will be printed here
      if (path.empty())
      {
        if (!output_handler.std_err.empty())
        {
          SCOPED_TRACE(output_handler.std_err);
          std::string text("Setup Script: " + _setup_script);
          SCOPED_TRACE(text.c_str());
          ADD_FAILURE();
        }

        output_handler.wipe_all();
      }
      else
      {
        // If processing a tets script, performs the validations over it
        (*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE] = shcore::Value::True();
        validate(script);
      }
    }

    stream.close();
  }
  else
  {
    std::string text("Unable to open test script: " + script);
    SCOPED_TRACE(text.c_str());
    ADD_FAILURE();
  }
}

// Searches for // Assumpsions: comment, if found, creates the __assumptions__ array
// And processes the _assumption_script
void Shell_script_tester::process_setup(std::istream & stream)
{
  bool done = false;
  while (!done && !stream.eof())
  {
    std::string line;
    std::getline(stream, line);

    if (line.find(get_assumptions_token()) == 0)
    {
      // Removes the assumptions header and parses the rest
      std::vector<std::string> tokens;
      boost::split(tokens, line, boost::is_any_of(":"), boost::token_compress_on);

      // Now parses the real assumptions
      std::string assumptions = tokens[1];
      boost::split(tokens, assumptions, boost::is_any_of(","), boost::token_compress_on);

      // Now quotes the assumptions
      for (size_t index = 0; index < tokens.size(); index++)
      {
        boost::trim(tokens[index]);
        tokens[index] = "'" + tokens[index] + "'";
      }

      // Creates an assumptions array to be processed on the setup script
      std::string code = get_variable_prefix() + "__assumptions__ = [" + boost::join(tokens, ",") + "];";
      execute(code);

      if (_setup_script.empty())
        throw std::logic_error("A setup script must be specified when there are assumptions on the tested scripts.");
      else
        execute_script(); // Executes the active setup script
    }
    else
      done = true;
  }

  // Once the assumptions are processed, rewinds the read position
  // To the beggining of the script
  stream.seekg(0, stream.beg);
}

void Shell_script_tester::validate_batch(const std::string& script)
{
  execute_script(script, false);
}

void Shell_js_script_tester::SetUp()
{
  Shell_script_tester::SetUp();

  _interactive_shell->process_line("\\js");

  std::string js_modules_path = MYSQLX_SOURCE_HOME;
  js_modules_path += "/scripting/modules/js";
  execute("shell.js.module_paths[shell.js.module_paths.length] = '" + js_modules_path + "';");
}

void Shell_py_script_tester::SetUp()
{
  Shell_script_tester::SetUp();

  _interactive_shell->process_line("\\py");

  output_handler.wipe_all();
}