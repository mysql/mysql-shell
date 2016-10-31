/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "modules/adminapi/mod_dba_common.h"
#include "utils/utils_general.h"
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_sql.h"
//#include "mod_dba_instance.h"

namespace mysqlsh {
namespace dba {

  void resolve_instance_credentials(const shcore::Value::Map_type_ref& options, shcore::Interpreter_delegate* delegate) {
    // Sets a default user if not specified
    if (!options->has_key("user") && !options->has_key("dbUser"))
      (*options)["dbUser"] = shcore::Value("root");

    if (!options->has_key("password") && !options->has_key("dbPassword")) {
      if (delegate) {
        std::string answer;

        std::string prompt = "Please provide a password for '" + build_connection_string(options, false) + "': ";
        if (delegate->password(delegate->user_data, prompt.c_str(), answer))
          (*options)["password"] = shcore::Value(answer);
      }
      else
        throw shcore::Exception::argument_error("Missing password for '" + build_connection_string(options, false) + "'");
    }
  }

  shcore::Value::Map_type_ref get_instance_options_map(const shcore::Argument_list &args, bool get_password_from_options) {
  shcore::Value::Map_type_ref options;

  // This validation will be added here but should not be needed if the function is used properly
  // callers are resposible for validating the number of arguments is correct
  args.ensure_at_least(1, "get_instance_options_map");

  // Attempts getting an instance object
  //auto instance = args.object_at<mysqlsh::dba::Instance>(0);
  //if (instance) {
  //  options = shcore::get_connection_data(instance->get_uri(), false);
  //  (*options)["password"] = shcore::Value(instance->get_password());
  //}

  // Not an instance, tries as URI string
  //else
  if (args[0].type == shcore::String)
    options = shcore::get_connection_data(args.string_at(0), false);

  // Finally as a dictionary
  else if (args[0].type == shcore::Map)
    options = args.map_at(0);
  else
    throw shcore::Exception::argument_error("Invalid connection options, expected either a URI or a Dictionary");

  if (options->size() == 0)
    throw shcore::Exception::argument_error("Connection definition is empty");

  // Overrides the options password if given appart
  if (args.size() == 2) {
    bool override_pwd = false;
    std::string new_password;

    if (get_password_from_options) {
      if(args[1].type == shcore::Map ) {
        auto other_options = args.map_at(1);
        shcore::Argument_map other_args(*other_options);

        if (other_args.has_key("password")) {
          new_password = other_args.string_at("password");
          override_pwd = true;
        }
        else if (other_args.has_key("dbPassword")) {
          new_password = other_args.string_at("dbPassword");
          override_pwd = true;
        }
      }
    }
    else if(args[1].type == shcore::String ) {
      override_pwd = true;
      new_password = args.string_at(1);
    }

    if (override_pwd) {
      if (options->has_key("dbPassword"))
        (*options)["dbPassword"] = shcore::Value(new_password);
      else
        (*options)["password"] = shcore::Value(new_password);
    }
  }

  return options;
}

std::string get_mysqlprovision_error_string(const shcore::Value::Array_type_ref& errors) {
  std::vector<std::string> str_errors;

  for (auto error: *errors) {
    auto data = error.as_map();
    auto error_type = data->get_string("type");
    auto error_text = data->get_string("msg");

    str_errors.push_back(error_type + ": " + error_text);
  }

  return shcore::join_strings(str_errors, "\n");
}

} // dba
} // mysqlsh
