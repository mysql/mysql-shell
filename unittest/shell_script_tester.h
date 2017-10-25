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

#include <map>
#include "test_utils.h"

enum class ValidationType {
  Simple = 0,
  LineByLine = 1
};

struct Validation {
  Validation(const std::vector<std::string>& source, ValidationType vtype = ValidationType::Simple) {
    if (source.size() < 3) {
      std::string a;
      a = "asads";
      std::cout << a;
    }
    code = source.size() >= 1 ? source[0] : "";
    expected_output = source.size() >= 2 ? source[1] : "";
    expected_error = source.size() >= 3 ? source[2] : "";


    if (expected_output.find("~") == 0) {
      unexpected_output = expected_output.substr(1);
      expected_output = "";
    }

    type = vtype;
  }

  ValidationType type;
  std::string code;
  std::string expected_output;
  std::string unexpected_output;
  std::string expected_error;
};

typedef std::vector<Validation> Validation_t;

#define NEW_TEST_SCRIPT(x) _shell_scripts_home+"/"+x+"."+_extension
#define PRE_SCRIPT(x) _shell_scripts_home+"/"+x+".pre"
#define VAL_SCRIPT(x) _shell_scripts_home+"/"+x+".val"

#define TEST_SCRIPT(x) _scripts_home+"/"+x
#define SETUP_SCRIPT(x) _shell_scripts_home+"/setup/"+x
#define VALIDATION_SCRIPT(x) _shell_scripts_home+"/validation/"+x

class Shell_script_tester : public Crud_test_wrapper {
public:
  // You can define per-test set-up and tear-down logic as usual.
  Shell_script_tester();

  virtual void SetUp();

  void validate_batch(const std::string& name);
  void validate_interactive(const std::string& name);
  void validate_chunks(const std::string& path,
                       const std::string& val_path = "",
                       const std::string& pre_path = "");

protected:
  std::string _setup_script; // Name of the active script
  std::string _scripts_home; // Path to the scripts to be tested
  std::string _shell_scripts_home; // Id of the folder containing the setup and validation scripts

  // The name of the folder containing the setup and validation scripts
  void set_config_folder(const std::string &name);

  // The name of the active setup script, should be set after the config folder
  void set_setup_script(const std::string &name);

  virtual std::string get_chunk_token() = 0;
  virtual std::string get_chunk_by_line_token() = 0;
  virtual std::string get_assumptions_token() = 0;
  virtual std::string get_variable_prefix() = 0;

  std::string _extension;
  bool _new_format;

  void execute_setup();
  void add_to_cfg_file(const std::string &cfgfile_path, const std::string &option);
  void remove_from_cfg_file(const std::string &cfgfile_path, const std::string &option);

  bool has_openssl_binary();

private:
  // Chunks of code will be stored here
  std::map<std::string, std::vector<std::string>> _chunks;
  std::vector<std::string> _chunk_order;
  std::map<std::string, Validation_t> _chunk_validations;
  std::map<std::string, int> _chunk_to_line;

  void execute_script(const std::string& path = "", bool in_chunks = false, bool is_pre_script = false);
  void process_setup(std::istream & stream);
  bool validate(const std::string& context, const std::string &chunk_id = "__global__");
  bool validate_line_by_line(const std::string& context, const std::string &chunk_id, const std::string &stream, const std::string& expected, const std::string &actual);
  std::string resolve_string(const std::string& source);
  virtual void pre_process_line(const std::string &path, std::string & line) {};

  void load_source_chunks(std::istream & stream);
  void add_validation(const std::string &chunk, const std::vector<std::string>& source, ValidationType type = ValidationType::Simple);
  void load_validations(const std::string& path, bool in_chunks = false);
};

class Shell_js_script_tester : public Shell_script_tester {
protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void set_defaults();

  virtual std::string get_chunk_token() { return "//@"; }
  virtual std::string get_chunk_by_line_token() { return "//@#"; }
  virtual std::string get_assumptions_token() { return "// Assumptions:"; }
  virtual std::string get_variable_prefix() { return "var "; };
};

class Shell_py_script_tester : public Shell_script_tester {
protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void set_defaults();

  virtual std::string get_chunk_token() { return "#@"; }
  virtual std::string get_chunk_by_line_token() { return "#@#"; }
  virtual std::string get_assumptions_token() { return "# Assumptions:"; }
  virtual std::string get_variable_prefix() { return ""; }
};
