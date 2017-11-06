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
#include "shellcore/utils_help.h"
#include "modules/adminapi/mod_dba_common.h"
#include "shellcore/base_session.h"
#include "modules/mod_mysql_session.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/devapi/base_database_object.h"
#include "shellcore/shell_notifications.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/libs/db/utils_connection.h"

using namespace std::placeholders;


namespace mysqlsh {

REGISTER_HELP(SHELL_BRIEF, "Gives access to general purpose functions and properties.");

Shell::Shell(Mysql_shell *owner)
    : _shell(owner),
      _shell_core(owner->shell_context().get()),
      _core_options(new shcore::Mod_shell_options(owner->get_options())) {
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
REGISTER_HELP(SHELL_OPTIONS_DETAIL15,
              "@li " SHCORE_DB_NAME_CACHE \
              ": true if auto-refresh of DB object name cache is "\
              "enabled. The \\rehash command can be used for manual refresh");
REGISTER_HELP(
    SHELL_OPTIONS_DETAIL16,
    "@li " SHCORE_DEVAPI_DB_OBJECT_HANDLES\
    ": true to enable schema collection and table name aliases in the db "\
    "object, for DevAPI operations.");

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
 * $(SHELL_OPTIONS_DETAIL13)
 * $(SHELL_OPTIONS_DETAIL14)
 * $(SHELL_OPTIONS_DETAIL15)
 * $(SHELL_OPTIONS_DETAIL16)
 */
#if DOXYGEN_JS
Dictionary Shell::options;
#elif DOXYGEN_PY
dict Shell::options;
#endif

shcore::Value Shell::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "options") {
    ret_val =
        shcore::Value(std::static_pointer_cast<Object_bridge>(_core_options));
  } else {
    ret_val = Cpp_object_bridge::get_member(prop);
  }

  return ret_val;
}

REGISTER_HELP(SHELL_PARSEURI_BRIEF, "Utility function to parse a URI string.");
REGISTER_HELP(SHELL_PARSEURI_PARAM, "@param uri a URI string.");
REGISTER_HELP(SHELL_PARSEURI_RETURNS, "@returns A dictionary containing all "\
                                      "the elements contained on the given "\
                                      "URI string.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL, "Parses a URI string and returns a "\
                                     "dictionary containing an item for each "\
                                     "found element.");

REGISTER_HELP(SHELL_PARSEURI_DETAIL1, "TOPIC_URI");

REGISTER_HELP(SHELL_PARSEURI_DETAIL2, "For more details about how a URI is "
  "created as well as the returned dictionary, use \\? connection");


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
 *
 * For more information about the URI format as well as the returned dictionary
 * please look at \ref connection_data
 *
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
    auto options = mysqlsh::get_connection_options(args, PasswordFormat::NONE);
    auto map = mysqlsh::get_connection_map(options);

    ret_val = shcore::Value(map);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("parseUri"));

  return ret_val;
}

// clang-format off
REGISTER_HELP(SHELL_PROMPT_BRIEF, "Utility function to prompt data from the user.");
REGISTER_HELP(SHELL_PROMPT_PARAM, "@param message a string with the message to be shown to the user.");
REGISTER_HELP(SHELL_PROMPT_PARAM1, "@param options Optional dictionary with "\
                                   "options that change the function behavior.");
REGISTER_HELP(SHELL_PROMPT_RETURNS, "@returns A string value containing the input from the user.");
REGISTER_HELP(SHELL_PROMPT_DETAIL, "This function allows creating scripts that "\
                                   "require interaction with the user to gather data.");
REGISTER_HELP(SHELL_PROMPT_DETAIL1, "Calling this function with no options will show the given "\
                                    "message to the user and wait for the input. "\
                                    "The information entered by the user will be the returned value");
REGISTER_HELP(SHELL_PROMPT_DETAIL2, "The options dictionary may contain the following options:");
REGISTER_HELP(SHELL_PROMPT_DETAIL3, "@li defaultValue: a string value to be returned if the provides no data.");
REGISTER_HELP(SHELL_PROMPT_DETAIL4, "@li type: a string value to define the prompt type.");
REGISTER_HELP(SHELL_PROMPT_DETAIL5, "The type option supports the following values:");
REGISTER_HELP(SHELL_PROMPT_DETAIL6, "@li password: the user input will not be echoed on the screen.");
// clang-format on

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

    shcore::Prompt_result r;
    if (password)
      r = delegate->password(delegate->user_data, prompt.c_str(), &ret_val);
    else
      r = delegate->prompt(delegate->user_data, prompt.c_str(), &ret_val);

    // Uses the default value if needed (but not if cancelled)
    if (!default_value.empty() &&
        (r == shcore::Prompt_result::Ok && ret_val.empty()))
      ret_val = default_value;
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("prompt"));

  return shcore::Value(ret_val);

}

// These two lines link the help to be shown on \? connection
REGISTER_HELP(TOPIC_CONNECTION, "TOPIC_CONNECTION_DATA");
REGISTER_HELP(TOPIC_CONNECTION1, "TOPIC_CONNECTION_DATA_ADDITIONAL");

// These lines link the help that will be shown on the help() for every
// function using connection data
REGISTER_HELP(TOPIC_CONNECTION_DATA, "The connection data may be specified in the following formats:");
REGISTER_HELP(TOPIC_CONNECTION_DATA1, "@li A URI string");
REGISTER_HELP(TOPIC_CONNECTION_DATA2, "@li A dictionary with the connection options");
REGISTER_HELP(TOPIC_CONNECTION_DATA3, "TOPIC_URI");
REGISTER_HELP(TOPIC_CONNECTION_DATA4, "TOPIC_CONNECTION_OPTIONS");
REGISTER_HELP(TOPIC_CONNECTION_DATA5, "TOPIC_CONNECTION_DATA_DETAILS");
REGISTER_HELP(TOPIC_CONNECTION_DATA6, "TOPIC_CONNECTION_MORE_INFO");

REGISTER_HELP(TOPIC_CONNECTION_MORE_INFO, "For additional information on connection data use \\? connection.");

REGISTER_HELP(TOPIC_URI, "A basic URI string has the following format:");
REGISTER_HELP(TOPIC_URI1, "[scheme://][user[:password]@]host[:port][/schema][?option=value&option=value...]");

// These lines group the description of ALL the available connection options
REGISTER_HELP(TOPIC_CONNECTION_OPTIONS, "The following options are valid for use either in a URI or in a dictionary:");
REGISTER_HELP(TOPIC_CONNECTION_OPTIONS1, "TOPIC_URI_CONNECTION_OPTIONS");
REGISTER_HELP(TOPIC_CONNECTION_OPTIONS2, "The following options are also valid when a dictionary is used:");
REGISTER_HELP(TOPIC_CONNECTION_OPTIONS3, "TOPIC_DICT_CONNECTION_OPTIONS");

// These lines group the connection options available for URI
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS, "@li ssl-mode: the SSL mode to be used in the connection.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS1, "@li ssl-ca: the path to the X509 certificate authority in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS2, "@li ssl-capath: the path to the directory that contains the X509 certificates authorities in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS3, "@li ssl-cert: The path to the X509 certificate in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS4, "@li ssl-key: The path to the X509 key in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS5, "@li ssl-crl: The path to file that contains certificate revocation lists.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS6, "@li ssl-crlpath: The path of directory that contains certificate revocation list files.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS7, "@li ssl-ciphers: List of permitted ciphers to use for connection encryption.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS8, "@li tls-version: List of protocols permitted for secure connections");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS9, "@li auth-method: Authentication method");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS10, "When these options are defined in a URI, their values must be URL encoded.");

// These lines group the connection options available for dictionary
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS, "@li scheme: the protocol to be used on the connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS1, "@li user: the MySQL user name to be used on the connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS2, "@li dbUser: alias for user.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS3, "@li password: the password to be used on the connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS4, "@li dbPassword: same as password.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS5, "@li host: the hostname or IP address to be used on a TCP connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS6, "@li port: the port to be used in a TCP connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS7, "@li socket: the socket file name to be used on a connection through unix sockets.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS8, "@li schema: the schema to be selected once the connection is done.");

// The rest of the lines provide additional details related to the connection option definition
REGISTER_HELP(TOPIC_CONNECTION_DATA_DETAILS, "The connection options are case insensitive and can only be defined once.");
REGISTER_HELP(TOPIC_CONNECTION_DATA_DETAILS1, "If an option is defined more than once, an error will be generated.");

REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL, "TOPIC_CONNECTION_OPTION_SCHEME");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL1, "TOPIC_CONNECTION_OPTION_SOCKET");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL2, "TOPIC_CONNECTION_OPTION_SSL_MODE");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL3, "TOPIC_CONNECTION_OPTION_TLS_VERSION");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL4, "TOPIC_URI_ENCODED_VALUE");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL5, "TOPIC_URI_ENCODED_ATTRIBUTE");

REGISTER_HELP(TOPIC_CONNECTION_OPTION_SCHEME, "<b>Protocol Selection</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SCHEME1, "The scheme option defines the protocol to be used on the connection, the following are the accepted values:");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SCHEME2, "@li mysql: for connections using the Classic protocol.");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SCHEME3, "@li mysqlx: for connections using the X protocol.");

REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET, "<b>Socket Connections</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET1, "To define a socket connection, replace the host and port by the socket path.");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET2, "When using a connection dictionary, the path is set as the value for the socket option.");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET3, "When using a URI, the socket path must be URL encoded.");

REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE, "<b>SSL Mode</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE1, "The ssl-mode option accepts the following values:");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE2, "@li DISABLED");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE3, "@li PREFERRED");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE4, "@li REQUIRED");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE5, "@li VERIFY_CA");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE6, "@li VERIFY_IDENTITY");

REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION, "<b>TLS Version</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION1, "The tls-version option accepts the following values:");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION2, "@li TLSv1");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION3, "@li TLSv1.1");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION4, "@li TLSv1.2 (Supported only on commercial packages)");

REGISTER_HELP(TOPIC_URI_ENCODED_VALUE, "<b>URL Encoding</b>");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE1, "URL encoded values only accept alphanumeric characters and the next symbols: -._~!$'()*+;");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE2, "Any other character must be URL encoded.");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE3, "URL encoding is done by replacing the character being encoded by the sequence: %XX");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE4, "Where XX is the hexadecimal ASCII value of the character being encoded.");



REGISTER_HELP(SHELL_CONNECT_BRIEF, "Establishes the shell global session.");
REGISTER_HELP(SHELL_CONNECT_PARAM, "@param connectionData the connection data to be used to establish the session.");
REGISTER_HELP(SHELL_CONNECT_PARAM1, "@param password Optional the password to be used when establishing the session.");
REGISTER_HELP(SHELL_CONNECT_DETAIL, "This function will establish the global session with the received connection data.");
REGISTER_HELP(SHELL_CONNECT_DETAIL1, "TOPIC_CONNECTION_DATA");

REGISTER_HELP(SHELL_CONNECT_DETAIL2,
              "If no scheme is provided, a first attempt will be made to "
              "establish a NodeSession, and if it detects the used port is for "
              "the mysql protocol, it will attempt a ClassicSession");

REGISTER_HELP(SHELL_CONNECT_DETAIL3,
              "The password may be included on the connectionData, the "
              "optional parameter should be used only "
              "if the connectionData does not contain it already. If both are "
              "specified the password parameter will override the password "
              "defined on "
              "the connectionData.");
/**
 * $(SHELL_CONNECT_BRIEF)
 *
 * $(SHELL_CONNECT_PARAM)
 * $(SHELL_CONNECT_PARAM1)
 *
 * $(SHELL_CONNECT_DETAIL)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref connection_data
 *
 * $(SHELL_CONNECT_DETAIL2)
 *
 * $(SHELL_CONNECT_DETAIL3)
 *
 * $(SHELL_CONNECT_DETAIL4)
 * $(SHELL_CONNECT_DETAIL5)
 * $(SHELL_CONNECT_DETAIL6)
 * $(SHELL_CONNECT_DETAIL7)
 */
#if DOXYGEN_JS
Session Shell::connect(ConnectionData connectionData, String password) {}
#elif DOXYGEN_PY
Session Shell::connect(ConnectionData connectionData, str password) {}
#endif
shcore::Value Shell::connect(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("connect").c_str());

  try {
    auto connection_options =
        mysqlsh::get_connection_options(args, PasswordFormat::STRING);

    mysqlsh::resolve_connection_credentials(&connection_options);

    _shell->connect(connection_options);
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

  _shell_core->set_global("db", new_schema,
                          shcore::IShell_core::all_scripting_modes());
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

/*
 * Configures the received session as the global development session.
 * \param session: The session to be set as global.
 *
 * If there's a selected schema on the received session, it will be made
 * available to the scripting interfaces on the global *db* variable
 */
std::shared_ptr<mysqlsh::ShellBaseSession> Shell::set_session_global(
    const std::shared_ptr<mysqlsh::ShellBaseSession> &session) {
  if (session) {
    _shell_core->set_global(
        "session",
        shcore::Value(std::static_pointer_cast<Object_bridge>(session)));

    std::string currentSchema = session->get_default_schema();
    shcore::Value schema;
    if (!currentSchema.empty())
      schema = shcore::Value(session->get_schema(currentSchema));

    _shell_core->set_global("db", schema);
  } else {
    _shell_core->set_global("session", shcore::Value::Null());
    _shell_core->set_global("db", shcore::Value::Null());
  }

  return session;
}

/*
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

    if (object) {
      _shell_core->set_dev_session(object);
      set_session_global(object);
    } else {
      throw shcore::Exception::runtime_error("Invalid session object");
    }
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

}  // namespace mysqlsh
