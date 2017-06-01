/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>

#include "modules/mysqlxtest_utils.h"
#include "utils/utils_general.h"
#include "shellcore/shell_core_options.h"
#include "shellcore/utils_help.h"
#include "modules/adminapi/mod_dba_common.h"
#include "shellcore/base_session.h"
#include "modules/mod_mysql_session.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/interactive_object_wrapper.h"
#include "modules/devapi/base_database_object.h"
#include "shellcore/shell_notifications.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/libs/db/utils_connection.h"

using namespace std::placeholders;


namespace mysqlsh {

REGISTER_HELP(SHELL_BRIEF, "Gives access to general purpose functions and properties.");

Shell::Shell(shcore::IShell_core* owner):
_shell_core(owner) {
  init();
}

void Shell::init() {
  add_property("options");

  add_method("parseUri", std::bind(&Shell::parse_uri, this, _1), "uri", shcore::String, NULL);
  add_varargs_method("prompt", std::bind(&Shell::prompt, this, _1));
  add_varargs_method("connect", std::bind(&Shell::connect, this, _1));
  add_method("setCurrentSchema", std::bind(&Shell::_set_current_schema, this, _1), "name", shcore::String, NULL);
  add_method("setSession", std::bind(&Shell::set_session, this, _1), "session", shcore::Object, NULL);
  add_method("getSession", std::bind(&Shell::get_session, this, _1), NULL);
  add_method("reconnect", std::bind(&Shell::reconnect, this, _1), NULL);
}

Shell::~Shell() {}

bool Shell::operator == (const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Shell::set_member(const std::string &prop, shcore::Value value) {
  Cpp_object_bridge::set_member(prop, value);
}

// Documentation of shell.options
REGISTER_HELP(SHELL_OPTIONS_BRIEF, "Dictionary of active shell options.");
REGISTER_HELP(SHELL_OPTIONS_DETAIL, "The options dictionary may contain "\
                                    "the following attributes:");
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
REGISTER_HELP(SHELL_OPTIONS_DETAIL7, "@li " SHCORE_HISTORY_MAX_SIZE ": number "\
                                     "of entries to keep in command history");
REGISTER_HELP(SHELL_OPTIONS_DETAIL8, "@li " SHCORE_HISTORY_AUTOSAVE ": true "\
                              "to save command history when exiting the shell");
REGISTER_HELP(SHELL_OPTIONS_DETAIL9, \
    "@li " SHCORE_HISTIGNORE ": colon separated list of glob patterns to filter"
    " out of the command history in SQL mode");

REGISTER_HELP(SHELL_OPTIONS_DETAIL10, \
              "The outputFormat option supports the following values:");
REGISTER_HELP(SHELL_OPTIONS_DETAIL11, \
              "@li table: displays the output in table format (default)");
REGISTER_HELP(SHELL_OPTIONS_DETAIL12, \
              "@li json: displays the output in JSON format");
REGISTER_HELP(\
    SHELL_OPTIONS_DETAIL13, \
    "@li json/raw: displays the output in a JSON format but in a single line");
REGISTER_HELP(\
    SHELL_OPTIONS_DETAIL14, \
    "@li vertical: displays the outputs vertically, one line per column value");

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
 * $(SHELL_OPTIONS_DETAIL12)
 */
#if DOXYGEN_JS
Dictionary Shell::options;
#elif DOXYGEN_PY
dict Shell::options;
#endif

shcore::Value Shell::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "options") {
    ret_val = shcore::Value(std::static_pointer_cast<Object_bridge>(shcore::Shell_core_options::get_instance()));
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
    auto options = mysqlsh::get_connection_options(args, PasswordFormat::NONE);
    auto map = mysqlsh::get_connection_map(options);

    ret_val = shcore::Value(map);
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
    auto connection_options =
        mysqlsh::get_connection_options(args, PasswordFormat::STRING);

    mysqlsh::resolve_connection_credentials(&connection_options);

    SessionType type = SessionType::Auto;
    if (connection_options.has_scheme()) {
      if (connection_options.get_scheme() == "mysqlx")
        type = SessionType::Node;
      else if (connection_options.get_scheme() == "mysql")
        type = SessionType::Classic;
    }

    set_dev_session(connect_session(connection_options, type));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("connect"));

  return shcore::Value();
}

void Shell::set_current_schema(const std::string& name) {

  auto session = _shell_core->get_dev_session();
  shcore::Value new_schema;

  if (!name.empty()) {
    session->set_current_schema(name);

    new_schema = shcore::Value(session->get_schema(name));
  }

  if(_shell_core->get_global("db")) {
    auto db = _shell_core->get_global("db").as_object<shcore::Interactive_object_wrapper>();

    if (db) {
      if (new_schema)
        db->set_target(new_schema.as_object<Cpp_object_bridge>());
      else
        db->set_target(std::shared_ptr<Cpp_object_bridge>());
    }
    else
      _shell_core->set_global("db", new_schema,
                              shcore::IShell_core::all_scripting_modes());
  }
}

shcore::Value Shell::_set_current_schema(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("setCurrentSchema").c_str());


  shcore::Value new_schema;

  try {
    set_current_schema(args.string_at(0));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("connect"));

  return new_schema;
}


/**
* Configures the received session as the global development session.
* \param session: The session to be set as global.
*
* If there's a selected schema on the received session, it will be made available to the scripting interfaces on the global *db* variable
*/
std::shared_ptr<mysqlsh::ShellBaseSession> Shell::set_dev_session(const std::shared_ptr<mysqlsh::ShellBaseSession>& session) {
  _shell_core->set_dev_session(session);

  // When using the interactive wrappers instead of setting the global variables
  // The target Objects on the wrappers are set
  std::shared_ptr<shcore::Interactive_object_wrapper> wrapper;
  auto raw_session = _shell_core->get_global("session");
  if (raw_session && raw_session.type == shcore::Object) {
    wrapper = raw_session.as_object<shcore::Interactive_object_wrapper>();
    if (wrapper)
      wrapper->set_target(session);
  }

  if (!wrapper)
    _shell_core->set_global("session", shcore::Value(std::static_pointer_cast<Object_bridge>(session)));

  wrapper.reset();


  std::string currentSchema = session->get_default_schema();
  shcore::Value schema;
  if (!currentSchema.empty())
    schema = shcore::Value(session->get_schema(currentSchema));

  auto raw_db = _shell_core->get_global("db");
  if (raw_db && raw_db.type == shcore::Object) {
    wrapper = raw_db.as_object<shcore::Interactive_object_wrapper>();
    if (wrapper) {
      if (currentSchema.empty())
        wrapper->set_target(std::shared_ptr<shcore::Cpp_object_bridge>());
      else
        wrapper->set_target(schema.as_object<shcore::Cpp_object_bridge>());
    }
  }

  if (!wrapper)
    _shell_core->set_global("db", schema);

  wrapper.reset();

  return session;
}

/**
* Returns the global development session.
*/
std::shared_ptr<mysqlsh::ShellBaseSession> Shell::get_dev_session() {
  return _shell_core->get_dev_session();
}

shcore::Value Shell::set_session(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("setSession").c_str());
  shcore::Value ret_val;

  try {
    auto object = args.object_at<mysqlsh::ShellBaseSession>(0);

    if (object)
      set_dev_session(object);
    else
      throw shcore::Exception::runtime_error("Invalid session object provided");
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("setSession"));

  return args[0];
}

shcore::Value Shell::get_session(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("getSession").c_str());

  return shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(_shell_core->get_dev_session()));
}

shcore::Value Shell::reconnect(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("reconnect").c_str());

  bool ret_val = false;

  try {
    _shell_core->get_dev_session()->reconnect();
    ret_val = true;
  } catch (shcore::Exception &e) {
    ret_val = false;
  }

  return shcore::Value(ret_val);

}

/*std::shared_ptr<mysqlsh::ShellBaseSession> Shell::connect_session(
    const std::string &uri, const std::string &password, SessionType session_type) {
  shcore::Argument_list args;

  args.push_back(shcore::Value(shcore::get_connection_options(uri, true)));
  (*args.map_at(0))["password"] = shcore::Value(password);

  return connect_session(args, session_type);
}*/

std::shared_ptr<mysqlsh::ShellBaseSession> Shell::connect_session(
    const mysqlshdk::db::Connection_options &connection_options,
    SessionType session_type) {
  std::shared_ptr<ShellBaseSession> ret_val;
  std::string connection_error;

  mysqlsh::SessionType type(session_type);

  // Automatic protocol detection is ON
  // Attempts X Protocol first, then Classic
  if (type == mysqlsh::SessionType::Auto) {
    ret_val.reset(new mysqlsh::mysqlx::Session());
    try {
      ret_val->connect(connection_options);

#ifdef DEBUG_SESSION_CREATE_CLOSE
      shcore::ShellNotifications::get()->notify("SN_SESSION_CONNECTED",
                                                ret_val);
#endif

      return ret_val;
    } catch (shcore::Exception &e) {
      // Unknown message received from server indicates an attempt to create
      // And X Protocol session through the MySQL protocol
      int code = 0;
      if (e.error()->has_key("code"))
        code = e.error()->get_int("code");

      if (code == 2027 ||  // Unknown message received from server 10
          code == 2002 ||  // No connection could be made because the target
                           // machine actively refused it connecting to
                           // host:port
          code == 2006) {  // MySQL server has gone away (randomly sent by
                           // libmysqlx)
        type = mysqlsh::SessionType::Classic;

        // Since this is an unexpected error, we store the message to be logged
        // in case the classic session connection fails too
        if (code == 2006)
          connection_error = "X protocol error: " + e.format();
      } else
        throw;
    }
  }

  switch (type) {
    case mysqlsh::SessionType::X:
      ret_val.reset(new mysqlsh::mysqlx::Session());
      break;
    case mysqlsh::SessionType::Classic:
      ret_val.reset(new mysql::ClassicSession());
      break;
    default:
      throw shcore::Exception::argument_error(
          "Invalid session type specified for MySQL connection.");
      break;
  }

  try {
    ret_val->connect(connection_options);
  } catch (shcore::Exception &e) {
    if (connection_error.empty())
      throw;
    else {
      // If an error was cached for the X protocol connection
      // it is included on a new exception
      connection_error.append("\nClassic protocol error: ");
      connection_error.append(e.format());
      throw shcore::Exception::argument_error(connection_error);
    }
  }

#ifdef DEBUG_SESSION_CREATE_CLOSE
  shcore::ShellNotifications::get()->notify("SN_SESSION_CONNECTED", ret_val);
#endif

  return ret_val;
}
}
