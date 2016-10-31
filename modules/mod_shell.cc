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
#include "modules/mod_shell.h"
#include "modules/mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include "shellcore/shell_core_options.h"
#include "utils/utils_help.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/base_session.h"

using namespace std::placeholders;

REGISTER_HELP(SHELL_BRIEF, "Gives access to general purpose functions and objects.");

namespace mysqlsh {

  Shell::Shell(shcore::IShell_core* owner):
  _shell_core(owner) {
    init();
  }

  void Shell::init() {
    _custom_prompt[0] = shcore::Value::Null();
    _custom_prompt[1] = shcore::Value::Null();

    add_property("options");
    add_property("customPrompt"); // Note that this specific property uses this name in both PY/JS (for now)

    add_method("parseUri", std::bind(&Shell::parse_uri, this, _1), "uri", shcore::String, NULL);
    add_varargs_method("prompt", std::bind(&Shell::prompt, this, _1));
    add_varargs_method("connect", std::bind(&Shell::connect, this, _1));
  }

  Shell::~Shell() {}

  bool Shell::operator == (const Object_bridge &other) const {
    return class_name() == other.class_name() && this == &other;
  }


  void Shell::set_member(const std::string &prop, shcore::Value value) {
    if (prop == "customPrompt") {
      if (value.type == shcore::Function)
        _custom_prompt[naming_style] = value;
      else
        throw shcore::Exception::runtime_error("The custom prompt must be a function");
    } else {
      Cpp_object_bridge::set_member(prop, value);
    }
  }

  // Documentation of getTables function
  REGISTER_HELP(SHELL_OPTIONS_BRIEF, "Displays the active shell options.");
  REGISTER_HELP(SHELL_CUSTOMPROMPT_BRIEF, "Callback to modify the default shell prompt.");
  REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL, "This property can be used to customize the shell prompt, i.e. to make it include more information than the default implementation.");
  REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL1, "This property acts as a place holder for a function that would create the desired prompt.");
  REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL2, "To modify the default shell prompt, follow the next steps:");
  REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL3, "@li Define a function that receives no parameters and returns a string which is the desired prompt.");
  REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL4, "@li Assign the function to this property.");
  REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL5, "The new prompt function will be automatically used to create the shell propmt.");

  /**
   * $(SHELL_CUSTOMPROMPT_BRIEF)
   *
   * $(SHELL_CUSTOMPROMPT_DETAIL)
   *
   * $(SHELL_CUSTOMPROMPT_DETAIL1)
   *
   * $(SHELL_CUSTOMPROMPT_DETAIL2)
   * $(SHELL_CUSTOMPROMPT_DETAIL3)
   * $(SHELL_CUSTOMPROMPT_DETAIL4)
   *
   * $(SHELL_CUSTOMPROMPT_DETAIL5)
   */
  #if DOXYGEN_JS
  Shell::customPrompt;
  #elif DOXYGEN_PY
  Shell::custom_prompt;
  #endif


  shcore::Value Shell::get_member(const std::string &prop) const {
    shcore::Value ret_val;

    if (prop == "options") {
      ret_val = shcore::Value(std::static_pointer_cast<Object_bridge>(shcore::Shell_core_options::get_instance()));
    } else if (prop == "customPrompt") {
      ret_val = _custom_prompt[naming_style];
    } else {
      ret_val = Cpp_object_bridge::get_member(prop);
    }

    return ret_val;
  }

  REGISTER_HELP(SHELL_PARSEURI_BRIEF, "Utility function to parse a URI string.");
  REGISTER_HELP(SHELL_PARSEURI_PARAM, "@param uri a URI string.");
  REGISTER_HELP(SHELL_PARSEURI_RETURN, "@return A dictionary containing all the elements contained on the given URI string.");
  REGISTER_HELP(SHELL_PARSEURI_DETAIL, "Parses a URI string and returns a dictionary containing an item for each found element.");
  /**
   * $(SHELL_PARSEURI_BRIEF)
   *
   * $(SHELL_PARSEURI_PARAM)
   *
   * $(SHELL_PARSEURI_RETURN)
   *
   * $(SHELL_PARSEURI_DETAIL)
   */
  #if DOXYGEN_JS
  Dictionary Shell::parseUri(String uri) {}
  #elif DOXYGEN_PY
  dict Shell::parse_uri(str uri) {}
  #endif
  shcore::Value Shell::parse_uri(const shcore::Argument_list &args) {

    shcore::Value ret_val;
    args.ensure_count(1, get_function_name("parseUri").c_str());

    try {
      ret_val = shcore::Value(shcore::get_connection_data(args.string_at(0), false));
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("parseUri"));

    return ret_val;
  }

  REGISTER_HELP(SHELL_PROMPT_BRIEF, "Utility function to prompt data from the user.");
  REGISTER_HELP(SHELL_PROMPT_PARAM, "@param message a string with the message to be shown to the user.");
  REGISTER_HELP(SHELL_PROMPT_PARAM1, "@param options Optional dictionary with attributes that change the function behavior.");
  REGISTER_HELP(SHELL_PROMPT_RETURN, "@return A string value containing the input from the user.");
  REGISTER_HELP(SHELL_PROMPT_DETAIL, "This function allows creating scripts that require interaction with hte user to gather data.");
  REGISTER_HELP(SHELL_PROMPT_DETAIL1, "Calling this function with no options will show the given message to the user and wait for the input. "\
    "The information entered by the user will be the returned value");
  REGISTER_HELP(SHELL_PROMPT_DETAIL2, "The options dictionary may contain the following attributes:");
  REGISTER_HELP(SHELL_PROMPT_DETAIL3, "@li defaultValue: a string value to be returned if the provides no data.");
  REGISTER_HELP(SHELL_PROMPT_DETAIL4, "@li password: boolean value, if set to true, it will not display hte user input.");

  /**
   * $(SHELL_PROMPT_BRIEF)
   *
   * $(SHELL_PROMPT_PARAM)
   * $(SHELL_PROMPT_PARAM1)
   *
   * $(SHELL_PROMPT_RETURN)
   *
   * $(SHELL_PROMPT_DETAIL)
   *
   * $(SHELL_PROMPT_DETAIL1)
   *
   * $(SHELL_PROMPT_DETAIL2)
   * $(SHELL_PROMPT_DETAIL3)
   * $(SHELL_PROMPT_DETAIL4)
   */
  #if DOXYGEN_JS
  String prompt(String message, Dictionary options);
  #elif DOXYGEN_PY
  str prompt(str message, dict options);
  #endif
  shcore::Value Shell::prompt(const shcore::Argument_list &args) {

    std::string ret_val;

    args.ensure_count(1, 2, get_function_name("prompt").c_str());

    try {

      std::string prompt = args.string_at(0);

      shcore::Value::Map_type_ref options;

      if (args.size() == 2)
        options = args.map_at(1);

      // If there are options, reads them to determine how to proceed
      std::string default_value;
      bool password = false;

      if (options) {
        shcore::Argument_map opt_map (*options);
        opt_map.ensure_keys({}, {"defaultValue", "type"}, "prompt options");

        if (opt_map.has_key("defaultValue"))
          default_value = opt_map.string_at("defaultValue");

        if (opt_map.has_key("type")) {
          std::string type = opt_map.string_at("type");

          if (type != "password")
            throw shcore::Exception::runtime_error("Unsupported value for parameter 'type', allowed values: password");
          else
            password = true;
        }
      }

      // Performs the actual prompt
      auto delegate = _shell_core->get_delegate();

      bool succeeded;
      if (password)
        succeeded = delegate->password(delegate->user_data, prompt.c_str(), ret_val);
      else
        succeeded = delegate->prompt(delegate->user_data, prompt.c_str(), ret_val);

      // Uses the default value if needed
      if (!default_value.empty() && (!succeeded || ret_val.empty()))
        ret_val = default_value;
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("prompt"));

    return shcore::Value(ret_val);

  }

  REGISTER_HELP(SHELL_CONNECT_BRIEF, "Establishes the shell global session.");
  REGISTER_HELP(SHELL_CONNECT_PARAM, "@param connectionData the connection data to be used to establish the session.");
  REGISTER_HELP(SHELL_CONNECT_PARAM1, "@param password Optional the password to be used when establishing the session.");
  REGISTER_HELP(SHELL_CONNECT_DETAIL, "This function will establish the global session with the received connection data.");
  REGISTER_HELP(SHELL_CONNECT_DETAIL1, "The connectionData parameter may be any of");
  REGISTER_HELP(SHELL_CONNECT_DETAIL2, "@li A URI string");
  REGISTER_HELP(SHELL_CONNECT_DETAIL3, "@li A dictionary with the connection data");
  REGISTER_HELP(SHELL_CONNECT_DETAIL4, "The password may be included on the connectionData, the optional parameter should be used only "\
    "if the connectionData does not contain it already. If both are specified the password parameter will override the password defined on "\
    "the connectionData." );
  REGISTER_HELP(SHELL_CONNECT_DETAIL5, "The type of session will be determined by the given connectionData:");
  REGISTER_HELP(SHELL_CONNECT_DETAIL6, "@li When using URI with a mysqlx scheme, a NodeSession wil be created");
  REGISTER_HELP(SHELL_CONNECT_DETAIL7, "@li When using URI with a mysql scheme, a ClassicSession wil be created");
  REGISTER_HELP(SHELL_CONNECT_DETAIL8, "@li When using a dictionary containing the session type will be determined by the 'scheme' attribute");
  REGISTER_HELP(SHELL_CONNECT_DETAIL9, "@li If the 'scheme' component is not provided, a NodeSession will be created");
  /**
   * $(SHELL_CONNECT_BRIEF)
   *
   * $(SHELL_CONNECT_PARAM)
   * $(SHELL_CONNECT_PARAM1)
   *
   * $(SHELL_CONNECT_RETURN)
   *
   * $(SHELL_CONNECT_DETAIL)
   *
   * $(SHELL_CONNECT_DETAIL1)
   * $(SHELL_CONNECT_DETAIL2)
   * $(SHELL_CONNECT_DETAIL3)
   *
   * $(SHELL_CONNECT_DETAIL4)
   *
   * $(SHELL_CONNECT_DETAIL5)
   * $(SHELL_CONNECT_DETAIL6)
   * $(SHELL_CONNECT_DETAIL7)
   * $(SHELL_CONNECT_DETAIL8)
   * $(SHELL_CONNECT_DETAIL9)
   */
  #if DOXYGEN_JS
  Session connect(String message, Dictionary options);
  #elif DOXYGEN_PY
  Session connect(str message, dict options);
  #endif
  shcore::Value Shell::connect(const shcore::Argument_list &args) {

    args.ensure_count(1, 2, get_function_name("connect").c_str());

    try {
      auto options = mysqlsh::dba::get_instance_options_map(args);
      mysqlsh::dba::resolve_instance_credentials(options);

      SessionType type = SessionType::Auto;
      if (options->has_key("scheme")) {
        if (options->get_string("scheme") == "mysqlx")
          type = SessionType::Node;
        else if (options->get_string("scheme") == "mysql")
          type = SessionType::Classic;
      }

      shcore::Argument_list new_args;

      new_args.push_back(shcore::Value(options));

      _shell_core->connect_dev_session(new_args, type);
    }
    CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("connect"));

    return shcore::Value();
  }
}