/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_sys.h"
#include <algorithm>
#include "modules/mysqlxtest_utils.h"
#include "shellcore/utils_help.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"

namespace mysqlsh {

REGISTER_HELP_GLOBAL_OBJECT_MODE(sys, shellapi, JAVASCRIPT);
REGISTER_HELP(SYS_BRIEF, "Gives access to system specific parameters.");
REGISTER_HELP(SYS_GLOBAL_BRIEF, "Gives access to system specific parameters.");

Sys::Sys(shcore::IShell_core *owner) { init(); }

void Sys::init() {
  _argv = shcore::make_array();
  _path = shcore::make_array();

  {
    // Searches for MYSQLX_HOME
    auto path = shcore::get_mysqlx_home_path();

    if (!path.empty()) {
      path = shcore::path::join_path(path, "share", "mysqlsh", "modules", "js");
    } else {
      // If MYSQLX_HOME not found, sets the current directory as a module path
      path = shcore::get_binary_folder();

      if (!path.empty())
        path = shcore::path::join_path(path, "modules", "js");
      else
        path = shcore::path::join_path(".", "modules", "js");
    }

    _path->emplace_back(path);
  }

  // Finally sees if there are additional configured paths
  if (const auto custom_paths = getenv("MYSQLSH_JS_MODULE_PATH")) {
    for (const auto &path : shcore::split_string(custom_paths, ";")) {
      if (!path.empty()) {
        _path->emplace_back(path);
      }
    }
  }

  add_property("argv");
  add_property("path");
}

Sys::~Sys() {}

bool Sys::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Sys::set_member(const std::string &prop, shcore::Value value) {
  std::string error;

  if (prop == "argv") {
    error = "The sys.argv property must be an array";
    if (value.type == shcore::Array) {
      _argv = value.as_array();
      error.clear();
    }
  } else if (prop == "path") {
    error = "The sys.path property must be an array of strings";
    if (value.type == shcore::Array) {
      auto tmp = value.as_array();

      if (std::find_if(tmp->begin(), tmp->end(), [](const shcore::Value &data) {
            return data.type != shcore::String;
          }) != tmp->end())
        error = "The sys.path property must be an array of strings";
      else {
        _path = tmp;
        error.clear();
      }
    }
  } else
    Cpp_object_bridge::set_member(prop, value);

  if (!error.empty()) throw shcore::Exception::argument_error(error);
}

REGISTER_HELP_PROPERTY_MODE(argv, sys, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(SYS_ARGV, R"*(
Contains the arguments for the script execution.

When the shell is used to execute a script, the arguments passed to the shell
after the script name will be considered script arguments and will be available
during the script execution at sys.argv which will contain:

@li The first element is the path to the script being executed.
@li Each script argument will be added as an element of the array.

For example, given the following call:
@code
$ mysqlsh root@localhost/sakila -f test.js first second 3
@endcode

The sys.argv array will contain:

@li sys.argv[0]: "test.js"
@li sys.argv[1]: "first"
@li sys.argv[2]: "second"
@li sys.argv[3]: "3"
)*");

REGISTER_HELP(
    SYS_ARGV_BRIEF,
    "Contains the list of arguments available during a script processing.");

REGISTER_HELP_PROPERTY_MODE(path, sys, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(SYS_PATH, R"*(
Lists the search paths to load the JavaScript modules.

When the shell is launched, this variable is initialized to
@li the home folder of shell + "share/mysqlsh/modules/js",
@li the folder specified in the <b>MYSQLSH_JS_MODULE_PATH</b> environment
variable (multiple folders are allowed, separated with semicolon).

Users may change the sys.path variable at run-time.

If sys.path contains a relative path, its absolute path is resolved as a
relative to the current working directory.
)*");

/**
 * $(SYS_ARGV_BRIEF)
 *
 * $(SYS_ARGV)
 */
#if DOXYGEN_JS
Array Sys::argv;
#endif

/**
 * $(SYS_PATH_BRIEF)
 *
 * $(SYS_PATH)
 */
#if DOXYGEN_JS
Array Sys::path;
#endif
shcore::Value Sys::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "argv") {
    ret_val = shcore::Value(_argv);
  } else if (prop == "path") {
    ret_val = shcore::Value(_path);
  } else {
    ret_val = Cpp_object_bridge::get_member(prop);
  }

  return ret_val;
}
}  // namespace mysqlsh
