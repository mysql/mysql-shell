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

Shell_script_tester::Shell_script_tester()
{
  _shell_scripts_home = shcore::get_binary_folder();
}

void Shell_script_tester::SetUp()
{}

void Shell_script_tester::TearDown()
{}

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

void Shell_script_tester::load_source(const std::string& path)
{
  std::ifstream file(path.c_str());

  if (!file.fail())
  {
    while (!file.eof())
    {
      std::string line;

      std::getline(file, line);

      _src_lines.push_back(line);
    }
  }
}

void Shell_script_tester::load_validation(const std::string& path, bool interactive)
{
  std::ifstream file(path.c_str());

  if (!file.fail())
  {
    while (!file.eof())
    {
      std::string line;

      std::getline(file, line);

      boost::trim(line);

      if (!line.empty())
      {
        std::vector<std::string> tokens;

        boost::split(tokens, line, boost::is_any_of("|"), boost::token_compress_off);

        int lineno = 0;
        if (interactive)
          lineno = boost::lexical_cast<int>(tokens[0]);

        tokens.erase(tokens.begin());

        if (!_validations.count(lineno))
          _validations[lineno] = new Validation_t();

        _validations[lineno]->push_back(Validation(tokens));
      }
    }
  }
}

void Shell_script_tester::validate(const std::string& context, size_t index, bool interactive)
{
  Validation_t* validations = _validations[index];
  for (size_t valindex = 0; valindex < validations->size(); valindex++)
  {
    // Validation goes against validation code
    if (!(*validations)[valindex].code.empty())
    {
      output_handler.wipe_all();
      execute((*validations)[valindex].code);
    }

    // Validates unexpected error
    if ((*validations)[valindex].expected_error.empty() &&
       !output_handler.std_err.empty())
    {
      if (interactive)
        SCOPED_TRACE("Executing: " + _src_lines[index]);
      SCOPED_TRACE("File: " + context);
      SCOPED_TRACE("Unexpexted Error: " + output_handler.std_err);
      ADD_FAILURE();
    }

    // Validates expected output if any
    if (!(*validations)[valindex].expected_output.empty())
    {
      std::string out = (*validations)[valindex].expected_output;

      if (interactive)
        SCOPED_TRACE("Executing: " + _src_lines[index]);
      SCOPED_TRACE("File: " + context);
      SCOPED_TRACE("STDOUT missing: " + out);
      SCOPED_TRACE("STDOUT actual: " + output_handler.std_out);
      EXPECT_NE(-1, int(output_handler.std_out.find(out)));
    }

    // Validates expected error if any
    if (!(*validations)[valindex].expected_error.empty())
    {
      std::string error = (*validations)[valindex].expected_error;

      if (interactive)
        SCOPED_TRACE("Executing: " + _src_lines[index]);
      SCOPED_TRACE("File: " + context);
      SCOPED_TRACE("STDOUT missing: " + error);
      SCOPED_TRACE("STDOUT actual: " + output_handler.std_err);
      EXPECT_NE(-1, int(output_handler.std_err.find(error)));
    }
  }
  output_handler.wipe_all();
}

void Shell_script_tester::validate_interactive(const std::string& script)
{
  std::ifstream pre(SETUP_SCRIPT(script).c_str());
  if (!pre.fail())
  {
    _shell_core->process_stream(pre);
    output_handler.wipe_all();
  }

  load_source(TEST_SCRIPT(script).c_str());
  load_validation(VALIDATION_SCRIPT(script), true);

  for (size_t index = 0; index < _src_lines.size(); index++)
  {
    execute(_src_lines[index]);

    if (_validations.count(index))
      validate(script, index, true);
    else
      output_handler.wipe_all();
  }
}

void Shell_script_tester::execute_script(const std::string& path)
{
  // If no path is provided then executes the setup script
  std::ifstream stream(path.empty() ? _setup_script : TEST_SCRIPT(path).c_str());

  if (!stream.fail())
  {
    // When it is a test script, preprocesses it so the
    // right execution scenario is in place
    if (!path.empty())
      process_setup(stream);

    // Process the file
    _shell_core->process_stream(stream);

    // In case it is the setup and there were errors
    // reports them here
    if (path.empty())
    {
      if (!output_handler.std_err.empty())
      {
        SCOPED_TRACE(output_handler.std_err);

        std::string text("Setup Script: " + path);
        SCOPED_TRACE(text.c_str());
        ADD_FAILURE();
      }

      output_handler.wipe_all();
    }
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

    if (line.find("// Assumptions:") == 0)
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
      std::string code = "var __assumptions__ = [" + boost::join(tokens, ",") + "];";
      execute(code);

      if (_setup_script.empty())
        throw std::logic_error("An assumptions script must be specified when there are assumptions on the tested scripts.");
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
  (*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE] = shcore::Value::False();

  execute_script(script);

  if (!output_handler.std_err.empty())
  {
    SCOPED_TRACE(output_handler.std_err);

    std::string text("Script File: ");
    text += script;
    SCOPED_TRACE(text.c_str());
    ADD_FAILURE();
  }

  // Executes the validations
  if (_validations.count(0))
  {
    delete _validations[0];
    _validations.clear();
  }

  // The validation process must be don in interactive way
  (*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE] = shcore::Value::True();
  load_validation(VALIDATION_SCRIPT(script), false);
  if (_validations.count(0))
    validate(script, 0, false);
}