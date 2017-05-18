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
#include "modules/mod_utils.h"

using namespace std::placeholders;


namespace mysqlsh {

REGISTER_HELP(SHELL_BRIEF, "Gives access to general purpose functions and properties.");

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
REGISTER_HELP(SHELL_OPTIONS_BRIEF, "Dictionary of active shell options.");
REGISTER_HELP(SHELL_OPTIONS_DETAIL, "The options dictionary may contain "\
                                    "the next attributes:");
REGISTER_HELP(SHELL_OPTIONS_DETAIL1, "@li batchContinueOnError: read-only, "\
                                     "boolean value to indicate if the "\
                                     "execution of an SQL script in batch "\
                                     "mode shall continue if errors occur");
REGISTER_HELP(SHELL_OPTIONS_DETAIL2, "@li interactive: read-only, boolean "\
                                     "value that indicates if the shell is "\
                                     "running in interactive mode");
REGISTER_HELP(SHELL_OPTIONS_DETAIL3, "@li outputFormat: controls the type of "\
                                     "output produced for SQL results.");
REGISTER_HELP(SHELL_OPTIONS_DETAIL4, "@li sandboxDir: default path where the "\
                                     "new sandbox instances for InnoDB "\
                                     "cluster will be deployed");
REGISTER_HELP(SHELL_OPTIONS_DETAIL5, "@li showWarnings: boolean value to "\
                                     "indicate whether warnings shall be "\
                                     "included when printing an SQL result");
REGISTER_HELP(SHELL_OPTIONS_DETAIL6, "@li useWizards: read-only, boolean value "\
                                     "to indicate if the Shell is using the "\
                                     "interactive wrappers (wizard mode)");

REGISTER_HELP(SHELL_OPTIONS_DETAIL7, "The outputFormat option supports the following values:");
REGISTER_HELP(SHELL_OPTIONS_DETAIL8, "@li table: displays the output in table format (default)");
REGISTER_HELP(SHELL_OPTIONS_DETAIL9, "@li json: displays the output in JSON format");
REGISTER_HELP(SHELL_OPTIONS_DETAIL10, "@li json/raw: displays the output in a JSON format but in a single line");
REGISTER_HELP(SHELL_OPTIONS_DETAIL11, "@li vertical: displays the outputs vertically, one line per column value");

REGISTER_HELP(SHELL_CUSTOMPROMPT_BRIEF, "Callback to modify the default shell prompt.");
REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL, "This property can be used to "\
                                         "customize the shell prompt, i.e. to "\
                                         "make it include more information "\
                                         "than the default implementation.");
REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL1, "This property acts as a place "\
                                          "holder for a function that would "\
                                          "create the desired prompt.");
REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL2, "To modify the default shell prompt, follow the next steps:");
REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL3, "@li Define a function that receives "\
                                          "no parameters and returns a string "\
                                          "which is the desired prompt.");
REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL4, "@li Assign the function to this property.");
REGISTER_HELP(SHELL_CUSTOMPROMPT_DETAIL5, "The new prompt function will be "\
                                          "automatically used to create the "\
                                          "shell prompt.");

/**
 * $(SHELL_OPTIONS_BRIEF)
 *
 * $(SHELL_OPTIONS_DETAIL)
 * $(SHELL_OPTIONS_DETAIL1)
 * $(SHELL_OPTIONS_DETAIL2)
 * $(SHELL_OPTIONS_DETAIL3)
 * $(SHELL_OPTIONS_DETAIL4)
 * $(SHELL_OPTIONS_DETAIL5)
 * $(SHELL_OPTIONS_DETAIL6)
 * $(SHELL_OPTIONS_DETAIL7)
 * $(SHELL_OPTIONS_DETAIL8)
 * $(SHELL_OPTIONS_DETAIL9)
 * $(SHELL_OPTIONS_DETAIL10)
 * $(SHELL_OPTIONS_DETAIL11)
 */
#if DOXYGEN_JS
Dictionary Shell::options;
#elif DOXYGEN_PY
dict Shell::options;
#endif

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
Callback Shell::customPrompt;
#elif DOXYGEN_PY
Callback Shell::custom_prompt;
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
REGISTER_HELP(SHELL_PARSEURI_RETURNS, "@returns A dictionary containing all "\
                                      "the elements contained on the given URI "\
                                      "string.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL, "Parses a URI string and returns a "\
                                     "dictionary containing an item for each "\
                                     "found element.");

REGISTER_HELP(SHELL_PARSEURI_DETAIL1, "A basic URI string has the following format:");
REGISTER_HELP(SHELL_PARSEURI_DETAIL2, "[scheme://][user[:password]@]host[:port][?key=value&key=value...]");

REGISTER_HELP(SHELL_PARSEURI_DETAIL3, "The scheme can be any of the following:");
REGISTER_HELP(SHELL_PARSEURI_DETAIL4, "@li mysql: For TCP connections using the Classic protocol.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL5, "@li mysqlx: For TCP connections using the X protocol.");

REGISTER_HELP(SHELL_PARSEURI_DETAIL6, "When using a URI the supported keys include:");
REGISTER_HELP(SHELL_PARSEURI_DETAIL7, "@li sslCa: the path to the X509 certificate authority in PEM format.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL8, "@li sslCaPath: the path to the directory that contains the X509 certificates authorities in PEM format.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL9, "@li sslCert: The path to the X509 certificate in PEM format.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL10, "@li sslKey: The path to the X509 key in PEM format.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL11, "@li sslCrl: The path to file that contains certificate revocation lists.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL12, "@li sslCrlPath: The path of directory that contains certificate revocation list files.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL13, "@li sslCiphers: List of permitted ciphers to use for connection encryption.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL14, "@li sslTlsVersion: List of protocols permitted for secure connections");

/**
 * $(SHELL_PARSEURI_BRIEF)
 *
 * $(SHELL_PARSEURI_PARAM)
 *
 * $(SHELL_PARSEURI_RETURNS)
 *
 * $(SHELL_PARSEURI_DETAIL)
 *
 * $(SHELL_PARSEURI_DETAIL1)
 * $(SHELL_PARSEURI_DETAIL2)
 *
 * $(SHELL_PARSEURI_DETAIL3)
 * $(SHELL_PARSEURI_DETAIL4)
 * $(SHELL_PARSEURI_DETAIL5)
 *
 * $(SHELL_PARSEURI_DETAIL6)
 * $(SHELL_PARSEURI_DETAIL7)
 * $(SHELL_PARSEURI_DETAIL8)
 * $(SHELL_PARSEURI_DETAIL9)
 * $(SHELL_PARSEURI_DETAIL10)
 * $(SHELL_PARSEURI_DETAIL11)
 * $(SHELL_PARSEURI_DETAIL12)
 * $(SHELL_PARSEURI_DETAIL13)
 * $(SHELL_PARSEURI_DETAIL14)
 */
#if DOXYGEN_JS
Dictionary Shell::parseUri(String uri){}
#elif DOXYGEN_PY
dict Shell::parse_uri(str uri){}
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
REGISTER_HELP(SHELL_PROMPT_PARAM1, "@param options Optional dictionary with "\
                                   "options that change the function "\
                                   "behavior.");
REGISTER_HELP(SHELL_PROMPT_RETURNS, "@returns A string value containing the input from the user.");
REGISTER_HELP(SHELL_PROMPT_DETAIL, "This function allows creating scripts that "\
                                   "require interaction with the user to "\
                                   "gather data.");
REGISTER_HELP(SHELL_PROMPT_DETAIL1, "Calling this function with no options will show the given "\
                                    "message to the user and wait for the input. "\
                                    "The information entered by the user will be the returned value");
REGISTER_HELP(SHELL_PROMPT_DETAIL2, "The options dictionary may contain the following options:");
REGISTER_HELP(SHELL_PROMPT_DETAIL3, "@li defaultValue: a string value to be returned if the provides no data.");
REGISTER_HELP(SHELL_PROMPT_DETAIL4, "@li type: a string value to define the prompt type.");
REGISTER_HELP(SHELL_PROMPT_DETAIL5, "The type option supports the following values:");
REGISTER_HELP(SHELL_PROMPT_DETAIL6, "@li password: the user input will not be echoed on the screen.");

/**
 * $(SHELL_PROMPT_BRIEF)
 *
 * $(SHELL_PROMPT_PARAM)
 * $(SHELL_PROMPT_PARAM1)
 *
 * $(SHELL_PROMPT_RETURNS)
 *
 * $(SHELL_PROMPT_DETAIL)
 *
 * $(SHELL_PROMPT_DETAIL1)
 *
 * $(SHELL_PROMPT_DETAIL2)
 * $(SHELL_PROMPT_DETAIL3)
 * $(SHELL_PROMPT_DETAIL4)
 * $(SHELL_PROMPT_DETAIL5)
 * $(SHELL_PROMPT_DETAIL6)
 */
#if DOXYGEN_JS
String Shell::prompt(String message, Dictionary options){}
#elif DOXYGEN_PY
str Shell::prompt(str message, dict options){}
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
REGISTER_HELP(SHELL_CONNECT_DETAIL1, "The connectionData parameter may be any of:");
REGISTER_HELP(SHELL_CONNECT_DETAIL2, "@li A URI string");
REGISTER_HELP(SHELL_CONNECT_DETAIL3, "@li A dictionary with the connection data");

REGISTER_HELP(SHELL_CONNECT_DETAIL4, "A basic URI string has the following format:");
REGISTER_HELP(SHELL_CONNECT_DETAIL5, "[scheme://][user[:password]@]host[:port][?key=value&key=value...]");

REGISTER_HELP(SHELL_CONNECT_DETAIL6, "The scheme can be any of the following:");
REGISTER_HELP(SHELL_CONNECT_DETAIL7, "@li mysql: For TCP connections using the Classic protocol.");
REGISTER_HELP(SHELL_CONNECT_DETAIL8, "@li mysqlx: For TCP connections using the X protocol.");

REGISTER_HELP(SHELL_CONNECT_DETAIL9, "When using a URI the supported keys include:");
REGISTER_HELP(SHELL_CONNECT_DETAIL10, "@li sslCa: the path to the X509 certificate authority in PEM format.");
REGISTER_HELP(SHELL_CONNECT_DETAIL11, "@li sslCaPath: the path to the directory that contains the X509 certificates authorities in PEM format.");
REGISTER_HELP(SHELL_CONNECT_DETAIL12, "@li sslCert: The path to the X509 certificate in PEM format.");
REGISTER_HELP(SHELL_CONNECT_DETAIL13, "@li sslKey: The path to the X509 key in PEM format.");
REGISTER_HELP(SHELL_CONNECT_DETAIL14, "@li sslCrl: The path to file that contains certificate revocation lists.");
REGISTER_HELP(SHELL_CONNECT_DETAIL15, "@li sslCrlPath: The path of directory that contains certificate revocation list files.");
REGISTER_HELP(SHELL_CONNECT_DETAIL16, "@li sslCiphers: List of permitted ciphers to use for connection encryption.");
REGISTER_HELP(SHELL_CONNECT_DETAIL17, "@li sslTlsVersion: List of protocols permitted for secure connections");

REGISTER_HELP(SHELL_CONNECT_DETAIL18, "The connection data dictionary may contain the following attributes:");
REGISTER_HELP(SHELL_CONNECT_DETAIL19, "@li user/dbUser: Username");
REGISTER_HELP(SHELL_CONNECT_DETAIL20, "@li password/dbPassword: Username password");
REGISTER_HELP(SHELL_CONNECT_DETAIL21, "@li host: Hostname or IP address");
REGISTER_HELP(SHELL_CONNECT_DETAIL22, "@li port: Port number");
REGISTER_HELP(SHELL_CONNECT_DETAIL23, "@li The ssl options described above");

REGISTER_HELP(SHELL_CONNECT_DETAIL24, "The password may be included on the connectionData, the optional parameter should be used only "\
"if the connectionData does not contain it already. If both are specified the password parameter will override the password defined on "\
"the connectionData.");
REGISTER_HELP(SHELL_CONNECT_DETAIL25, "The type of session will be determined by the given scheme:");
REGISTER_HELP(SHELL_CONNECT_DETAIL26, "@li If mysqlx scheme, a NodeSession will be created");
REGISTER_HELP(SHELL_CONNECT_DETAIL27, "@li If mysql scheme, a ClassicSession will be created");
REGISTER_HELP(SHELL_CONNECT_DETAIL28, "@li If 'scheme' is not provided, the shell will first attempt establish a NodeSession and if it detects "\
                                      "the used port is for the mysql protocol, it will attempt a ClassicSession");
/**
 * $(SHELL_CONNECT_BRIEF)
 *
 * $(SHELL_CONNECT_PARAM)
 * $(SHELL_CONNECT_PARAM1)
 *
 * $(SHELL_CONNECT_RETURNS)
 *
 * $(SHELL_CONNECT_DETAIL)
 *
 * $(SHELL_CONNECT_DETAIL1)
 * $(SHELL_CONNECT_DETAIL2)
 * $(SHELL_CONNECT_DETAIL3)
 *
 * $(SHELL_CONNECT_DETAIL4)
 * $(SHELL_CONNECT_DETAIL5)
 *
 * $(SHELL_CONNECT_DETAIL6)
 * $(SHELL_CONNECT_DETAIL7)
 * $(SHELL_CONNECT_DETAIL8)
 *
 * $(SHELL_CONNECT_DETAIL9)
 * $(SHELL_CONNECT_DETAIL10)
 * $(SHELL_CONNECT_DETAIL11)
 * $(SHELL_CONNECT_DETAIL12)
 * $(SHELL_CONNECT_DETAIL13)
 * $(SHELL_CONNECT_DETAIL14)
 * $(SHELL_CONNECT_DETAIL15)
 * $(SHELL_CONNECT_DETAIL16)
 * $(SHELL_CONNECT_DETAIL17)
 *
 * $(SHELL_CONNECT_DETAIL18)
 * $(SHELL_CONNECT_DETAIL19)
 * $(SHELL_CONNECT_DETAIL20)
 * $(SHELL_CONNECT_DETAIL21)
 * $(SHELL_CONNECT_DETAIL22)
 * $(SHELL_CONNECT_DETAIL23)
 *
 * $(SHELL_CONNECT_DETAIL24)
 *
 * $(SHELL_CONNECT_DETAIL25)
 * $(SHELL_CONNECT_DETAIL26)
 * $(SHELL_CONNECT_DETAIL27)
 * $(SHELL_CONNECT_DETAIL28)
 */
#if DOXYGEN_JS
Session Shell::connect(ConnectionData connectionData, String password){}
#elif DOXYGEN_PY
Session Shell::connect(ConnectionData connectionData, str password){}
#endif
shcore::Value Shell::connect(const shcore::Argument_list &args) {

  args.ensure_count(1, 2, get_function_name("connect").c_str());

  try {
    auto options = mysqlsh::get_connection_data(args, PasswordFormat::STRING);

    mysqlsh::resolve_connection_credentials(options);

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
