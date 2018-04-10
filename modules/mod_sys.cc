/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

namespace mysqlsh {

REGISTER_HELP(SYS_BRIEF, "Gives access to system specific parameters.");

Sys::Sys(shcore::IShell_core *owner) { init(); }

void Sys::init() {
  _argv.reset(new shcore::Value::Array_type());
  _path.reset(new shcore::Value::Array_type());

  // Searches for MYSQLX_HOME
  std::string path = shcore::get_mysqlx_home_path();
  if (!path.empty()) path.append("/share/mysqlsh/modules/js");
  // If MYSQLX_HOME not found, sets the current directory as a valid module path
  else {
    path = shcore::get_binary_folder();
    if (!path.empty())
      path.append("/modules/js");
    else
      path = "./modules/js";
  }

  _path->push_back(shcore::Value(path));

  // Finally sees if there are additional configured paths
  char *custom_paths = getenv("MYSQLSH_JS_MODULE_PATH");
  if (custom_paths) {
    auto paths = shcore::split_string(custom_paths, ";");
    for (auto custom_path : paths) _path->push_back(shcore::Value(custom_path));
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

REGISTER_HELP(
    SYS_ARGV_BRIEF,
    "Contains the list of arguments available during a script processing.");
REGISTER_HELP(SYS_PATH_BRIEF,
              "Lists the search paths to load JavaScript modules.");

/**
 * $(SHELL_ARGV_BRIEF)
 */
#if DOXYGEN_JS
Array Sys::argv;
#endif

/**
 * $(SHELL_PATH_BRIEF)
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
