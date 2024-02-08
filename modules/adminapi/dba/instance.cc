/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

// #include "mod_dba_instance.h"
#include "modules/adminapi/dba/instance.h"
#include "shellcore/utils_help.h"

namespace mysqlsh {
namespace dba {

REGISTER_HELP(INSTANCE_BRIEF, "Represents an Instance.");
REGISTER_HELP(
    INSTANCE_DETAIL,
    "This object represents an Instance and can be used to perform instance "
    "operations.");

Instance::Instance(const std::string &name, const std::string &uri,
                   const shcore::Value::Map_type_ref options)
    : _name(name), _uri(uri), _options(options) {
  init();
}

bool Instance::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Instance::init() {
  add_property("name", "getName");
  add_property("uri", "getUri");
  add_property("options", "getOptions");

  // Caches the password and removes it from the options
  if (_options) {
    if (_options->has_key("password")) {
      _password = _options->get_string("password");
      _options->erase("password");
    }
  }

  REGISTER_HELP(INSTANCE_NAME_BRIEF, "The instance name.");
  REGISTER_HELP(INSTANCE_URI_BRIEF, "The instance connection string.");
  REGISTER_HELP(INSTANCE_OPTIONS_BRIEF,
                "Dictionary with additional instance options.");
  REGISTER_HELP(INSTANCE_GETNAME_BRIEF, "Returns the instance name.");
  REGISTER_HELP(INSTANCE_GETURI_BRIEF, "Returns the instance URI.");
  REGISTER_HELP(INSTANCE_GETOPTIONS_BRIEF,
                "Returns a dictionary with additional instance options.");
}

shcore::Value Instance::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "name")
    ret_val = shcore::Value(_name);
  else if (prop == "uri")
    ret_val = shcore::Value(_uri);
  else if (prop == "options")
    ret_val = shcore::Value(_options);
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

}  // namespace dba
}  // namespace mysqlsh
