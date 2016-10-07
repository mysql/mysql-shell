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
#include "utils/utils_general.h"
#include "mod_dba_instance.h"

namespace mysh {
namespace dba {

shcore::Value::Map_type_ref get_instance_options_map(const shcore::Argument_list &args) {
  shcore::Value::Map_type_ref options;

  // Attempts getting an instance object
  auto instance = args.object_at<mysh::dba::Instance>(0);
  if (instance) {
    options = shcore::get_connection_data(instance->get_uri(), false);
    (*options)["password"] = shcore::Value(instance->get_password());
  }

  // Not an instance, tries as URI string
  else if (args[0].type == shcore::String)
    options = shcore::get_connection_data(args.string_at(0), false);

  // Finally as a dictionary
  else if (args[0].type == shcore::Map)
    options = args.map_at(0);
  else
    throw shcore::Exception::argument_error("Invalid connection options, expected either a URI, a Dictionary or an Instance object.");

  if (options->size() == 0)
    throw shcore::Exception::argument_error("Instance definition is empty.");
  
  // Overrides the options password if given appart
  if (args.size() == 2 && args[1].type == shcore::String ) {
    if (options->has_key("dbPassword"))
      (*options)["dbPassword"] = args[1];
    else
      (*options)["password"] = args[1];
  }
  
  return options;
}

} // dba
} // mysh
