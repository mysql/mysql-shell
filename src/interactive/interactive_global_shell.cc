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

#include "interactive_global_shell.h"
#include "utils/utils_general.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/mysqlxtest_utils.h"
#include "shellcore/base_session.h"
#include "modules/mod_utils.h"

using namespace std::placeholders;

namespace shcore {

void Global_shell::init() {
  add_varargs_method("connect", std::bind(&Global_shell::connect, this, _1));
}

shcore::Value Global_shell::connect(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("connect").c_str());

  mysqlshdk::db::Connection_options instance_def;
  try {
    instance_def = mysqlsh::get_connection_options(args,
                                               mysqlsh::PasswordFormat::STRING);

    mysqlsh::resolve_connection_credentials(&instance_def, _delegate);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("connect"));

  std::string stype;

  if (instance_def.has_scheme()) {
    if (instance_def.get_scheme() == "mysqlx")
      stype = "a Node";
    else
      stype = "a Classic";
  }
  else
    stype = "a";


  // Messages prior to the connection
  std::string message;
  message += "Creating " + stype + " Session to '" +
    instance_def.as_uri(mysqlshdk::db::uri::formats::full_no_password()) + "'";

  println(message);

  auto instance_map = mysqlsh::get_connection_map(instance_def);
  shcore::Argument_list new_args;
  new_args.push_back(shcore::Value(instance_map));

  shcore::Value ret_val = call_target("connect", new_args);

  auto new_session = _shell_core.get_dev_session();

  // Messages after the connection
  std::string session_type = new_session->class_name();

  message.clear();

  message = "Your MySQL connection id is " +
            std::to_string(new_session->get_connection_id());
  if (new_session->class_name() == "NodeSession")
    message += " (X protocol)";
  message += "\n";

  shcore::Value default_schema = new_session->get_member("currentSchema");

  if (default_schema) {
    if (session_type == "ClassicSession")
      message += "Default schema set to `" + default_schema.as_object()->get_member("name").as_string() + "`.";
    else
      message += "Default schema `" + default_schema.as_object()->get_member("name").as_string() + "` accessible through db.";
  } else
    message += "No default schema selected; type \\use <schema> to set one.";

  println(message);

  return shcore::Value();
}

}
