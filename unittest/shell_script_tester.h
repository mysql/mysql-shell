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

#include <map>
#include <memory>
#include <utility>
#include <vector>
#include "unittest/test_utils.h"

/**
 * Defines the type of validations available on the script testing engine.
 */
enum class ValidationType {
  Simple = 0,
  Multiline = 1,
  Optional = 2
};

// Note(rennox) the chunk lines in a test script and the ones on the validation
// file have different needs: the validation type and stream are only needed for
// validation, those things will start being ignored on the test scripts until
// they are cleaned up
struct Chunk_definition{
  std::string line;           // The line as read from the file
  std::string id;             // The ID of the chunk.
  std::string context;        // The context if defined.
  ValidationType validation;  // The validation type: single or multiline.
  std::string stream;         // The stream for multiline validation.
  int linenum;                // The line number
};

/**
 * Defines a validation to be done on the shell script testing engine.
 */
struct Validation {
  Validation(const std::vector<std::string>& source,
             const std::shared_ptr<Chunk_definition> &chunk_def) {
    code = source.size() >= 1 ? source[0] : "";
    expected_output = source.size() >= 2 ? source[1] : "";
    expected_error = source.size() >= 3 ? source[2] : "";


    if (expected_output.find("~") == 0) {
      unexpected_output = expected_output.substr(1);
      expected_output = "";
    }

    def = chunk_def;
  }

  std::string code;  //!< Defines code that must be executed before the validation takes place
  std::string expected_output;  //!< Defines the expected output
  std::string unexpected_output;  //!< Defines unexpected output
  std::string expected_error;  //!< Defines the expected error

  std::shared_ptr<Chunk_definition> def;
};

typedef std::vector<std::shared_ptr<Validation>> Chunk_validations;

#define NEW_TEST_SCRIPT(x) _shell_scripts_home+"/"+x+"."+_extension
#define PRE_SCRIPT(x) _shell_scripts_home+"/"+x+".pre"
#define VAL_SCRIPT(x) _shell_scripts_home+"/"+x+".val"

#define TEST_SCRIPT(x) _scripts_home+"/"+x
#define SETUP_SCRIPT(x) _shell_scripts_home+"/setup/"+x
#define VALIDATION_SCRIPT(x) _shell_scripts_home+"/validation/"+x

typedef std::pair<size_t, std::string> Chunk_line_t;

struct Chunk_t {
  Chunk_t() {
    def.reset(new Chunk_definition());
  }
  std::vector<Chunk_line_t> code;
  std::shared_ptr<Chunk_definition> def;
  bool is_validation_optional() const {
    return def->validation == ValidationType::Optional;
  }
};

/**
 * Base class for the Shell Script Testing engine.
 */
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

  void execute(int location, const std::string& code);
  void execute(const std::string& code);

 protected:
  std::string _setup_script; // Name of the active script
  std::string _scripts_home; // Path to the scripts to be tested
  std::string _shell_scripts_home; // Id of the folder containing the setup and validation scripts

  // The name of the folder containing the setup and validation scripts
  void set_config_folder(const std::string &name);

  // The name of the active setup script, should be set after the config folder
  void set_setup_script(const std::string &name);

  virtual std::string get_comment_token() = 0;
  virtual std::string get_chunk_token() = 0;
  virtual std::string get_chunk_by_line_token() = 0;
  virtual std::string get_assumptions_token() = 0;
  virtual std::string get_variable_prefix() = 0;
  virtual shcore::NamingStyle get_naming_style() = 0;

  std::string _extension;
  bool _new_format;

  void execute_setup();
  void add_to_cfg_file(const std::string &cfgfile_path, const std::string &option);
  void remove_from_cfg_file(const std::string &cfgfile_path, const std::string &option);

  bool has_openssl_binary();

private:
  // Chunks of code will be stored here
  std::string _filename;
  std::map<std::string, Chunk_t> _chunks;
  std::vector<std::string> _chunk_order;
  std::map<std::string, Chunk_validations> _chunk_validations;
  std::map<std::string, int> _chunk_to_line;

  void execute_script(const std::string& path = "", bool in_chunks = false, bool is_pre_script = false);
  void process_setup(std::istream & stream);
  bool validate(const std::string& context,
                const std::string &chunk_id = "__global__",
                bool optional = false);
  bool validate_line_by_line(const std::string& context, const std::string &chunk_id, const std::string &stream, const std::string& expected, const std::string &actual);
  std::string resolve_string(const std::string& source);
  virtual void pre_process_line(const std::string &path, std::string & line) {};

  std::shared_ptr<Chunk_definition> load_chunk_definition(const std::string &line);
  bool load_source_chunks(const std::string& path, std::istream & stream);
  void add_source_chunk(const std::string& path, const Chunk_t& chunk);
  void add_validation(const std::shared_ptr<Chunk_definition> &chunk,
                      const std::vector<std::string>& source);
  void load_validations(const std::string& path);
  bool context_enabled(const std::string& context);
};

/**
 * Base class for the JavaScript Script Testing Engine.
 */
class Shell_js_script_tester : public Shell_script_tester {
protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void set_defaults();

  virtual std::string get_comment_token() { return "//"; }
  virtual std::string get_chunk_token() { return "//@"; }
  virtual std::string get_chunk_by_line_token() { return "//@#"; }
  virtual std::string get_assumptions_token() { return "// Assumptions:"; }
  virtual std::string get_variable_prefix() { return "var "; }
  virtual shcore::NamingStyle get_naming_style() { return shcore::LowerCamelCase; }
};

/**
 * Base class for the Python Script Testing Engine.
 */
class Shell_py_script_tester : public Shell_script_tester {
protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void set_defaults();

  virtual std::string get_comment_token() { return "#"; };
  virtual std::string get_chunk_token() { return "#@"; }
  virtual std::string get_chunk_by_line_token() { return "#@#"; }
  virtual std::string get_assumptions_token() { return "# Assumptions:"; }
  virtual std::string get_variable_prefix() { return ""; }
  virtual shcore::NamingStyle get_naming_style() { return shcore::LowerCaseUnderscores; }
};
