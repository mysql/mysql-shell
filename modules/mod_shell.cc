/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/mod_shell.h"

#include <memory>

#include "modules/adminapi/mod_dba_common.h"
#include "modules/devapi/base_database_object.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/shellcore/credential_manager.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_notifications.h"
#include "shellcore/utils_help.h"
#include "utils/utils_general.h"

using namespace std::placeholders;

namespace mysqlsh {
REGISTER_HELP_OBJECT(shell, shellapi);
REGISTER_HELP(SHELL_BRIEF,
              "Gives access to general purpose functions and properties.");
REGISTER_HELP(SHELL_GLOBAL_BRIEF,
              "Gives access to general purpose functions and properties.");

Shell::Shell(Mysql_shell *owner)
    : _shell(owner),
      _shell_core(owner->shell_context().get()),
      _core_options(new Options(owner->get_options())) {
  init();
}

void Shell::init() {
  add_property("options");

  add_method("parseUri", std::bind(&Shell::parse_uri, this, _1), "uri",
             shcore::String);
  add_varargs_method("prompt", std::bind(&Shell::prompt, this, _1));
  add_varargs_method("connect", std::bind(&Shell::connect, this, _1));
  add_method("setCurrentSchema",
             std::bind(&Shell::_set_current_schema, this, _1), "name",
             shcore::String);
  add_method("setSession", std::bind(&Shell::set_session, this, _1), "session",
             shcore::Object);
  add_method("getSession", std::bind(&Shell::get_session, this, _1));
  add_method("reconnect", std::bind(&Shell::reconnect, this, _1));
  add_method("log", std::bind(&Shell::log, this, _1));
  add_method("status", std::bind(&Shell::status, this, _1));
  add_method("listCredentialHelpers",
             std::bind(&Shell::list_credential_helpers, this, _1));
  add_method("storeCredential", std::bind(&Shell::store_credential, this, _1),
             "url", shcore::String, "password", shcore::String);
  add_method("deleteCredential", std::bind(&Shell::delete_credential, this, _1),
             "url", shcore::String);
  add_method("deleteAllCredentials",
             std::bind(&Shell::delete_all_credentials, this, _1));
  add_method("listCredentials", std::bind(&Shell::list_credentials, this, _1));
}

Shell::~Shell() {}

bool Shell::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

void Shell::set_member(const std::string &prop, shcore::Value value) {
  Cpp_object_bridge::set_member(prop, value);
}

// Documentation of shell.options
REGISTER_HELP(SHELL_OPTIONS_BRIEF,
              "Gives access to the options that modify the shell behavior.");

/**
 * $(SHELL_OPTIONS_BRIEF)
 */
#if DOXYGEN_JS
Options Shell::options;
#elif DOXYGEN_PY
Options Shell::options;
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

REGISTER_HELP_FUNCTION(parseUri, shell);
REGISTER_HELP(SHELL_PARSEURI_BRIEF, "Utility function to parse a URI string.");
REGISTER_HELP(SHELL_PARSEURI_PARAM, "@param uri a URI string.");
REGISTER_HELP(SHELL_PARSEURI_RETURNS,
              "@returns A dictionary containing all "
              "the elements contained on the given "
              "URI string.");
REGISTER_HELP(SHELL_PARSEURI_DETAIL,
              "Parses a URI string and returns a "
              "dictionary containing an item for each "
              "found element.");

REGISTER_HELP(SHELL_PARSEURI_DETAIL1, "${TOPIC_URI}");

REGISTER_HELP(SHELL_PARSEURI_DETAIL2,
              "For more details about how a URI is "
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
 * $(TOPIC_URI)
 *
 * $(TOPIC_URI1)
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
REGISTER_HELP_FUNCTION(prompt, shell);
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
String Shell::prompt(String message, Dictionary options) {}
#elif DOXYGEN_PY
str Shell::prompt(str message, dict options) {}
#endif
shcore::Value Shell::prompt(const shcore::Argument_list &args) {
  std::string ret_val;

  args.ensure_count(1, 2, get_function_name("prompt").c_str());

  try {
    std::string prompt = args.string_at(0);

    shcore::Value::Map_type_ref options;

    if (args.size() == 2) options = args.map_at(1);

    // If there are options, reads them to determine how to proceed
    std::string default_value;
    bool password = false;

    if (options) {
      shcore::Argument_map opt_map(*options);
      opt_map.ensure_keys({}, {"defaultValue", "type"}, "prompt options");

      if (opt_map.has_key("defaultValue"))
        default_value = opt_map.string_at("defaultValue");

      if (opt_map.has_key("type")) {
        std::string type = opt_map.string_at("type");

        if (type != "password")
          throw shcore::Exception::runtime_error(
              "Unsupported value for parameter 'type', allowed values: "
              "password");
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
REGISTER_HELP_TOPIC(Connection, TOPIC, TOPIC_CONNECTION, Contents, ALL);
REGISTER_HELP(TOPIC_CONNECTION_BRIEF,
              "Information about the data used to create sessions.");
REGISTER_HELP(TOPIC_CONNECTION, "${TOPIC_CONNECTION_DATA_BASIC}");
REGISTER_HELP(TOPIC_CONNECTION1, "${TOPIC_CONNECTION_DATA_ADDITIONAL}");

// These lines link the help that will be shown on the help() for every
// function using connection data

REGISTER_HELP(TOPIC_CONNECTION_DATA_BASIC,
              "The connection data may be specified in the following formats:");
REGISTER_HELP(TOPIC_CONNECTION_DATA_BASIC1, "@li A URI string");
REGISTER_HELP(TOPIC_CONNECTION_DATA_BASIC2,
              "@li A dictionary with the connection options");
REGISTER_HELP(TOPIC_CONNECTION_DATA_BASIC3, "${TOPIC_URI}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_BASIC4, "${TOPIC_CONNECTION_OPTIONS}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_BASIC5, "${TOPIC_CONNECTION_DATA_DETAILS}");

REGISTER_HELP(TOPIC_CONNECTION_DATA, "${TOPIC_CONNECTION_DATA_BASIC}");
REGISTER_HELP(TOPIC_CONNECTION_DATA1, "${TOPIC_CONNECTION_MORE_INFO}");

REGISTER_HELP(TOPIC_CONNECTION_MORE_INFO,
              "For additional information on connection data use "
              "\\? connection.");

REGISTER_HELP(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY,
              "${TOPIC_CONNECTION_MORE_INFO}");
REGISTER_HELP(TOPIC_CONNECTION_MORE_INFO_TCP_ONLY1,
              "Only TCP/IP connections are allowed for this function.");

REGISTER_HELP(TOPIC_URI, "A basic URI string has the following format:");
REGISTER_HELP(TOPIC_URI1,
              "[scheme://][user[:password]@]<host[:port]|socket>[/schema]"
              "[?option=value&option=value...]");

// These lines group the description of ALL the available connection options
REGISTER_HELP(TOPIC_CONNECTION_OPTIONS, "<b>SSL Connection Options</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTIONS1,
              "The following options are valid for use either in a URI or in a "
              "dictionary:");
REGISTER_HELP(TOPIC_CONNECTION_OPTIONS2, "${TOPIC_URI_CONNECTION_OPTIONS}");
REGISTER_HELP(
    TOPIC_CONNECTION_OPTIONS3,
    "The following options are also valid when a dictionary is used:");
REGISTER_HELP(TOPIC_CONNECTION_OPTIONS4, "${TOPIC_DICT_CONNECTION_OPTIONS}");

// These lines group the connection options available for URI
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS,
              "@li ssl-mode: the SSL mode to be used in the connection.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS1,
    "@li ssl-ca: the path to the X509 certificate authority in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS2,
              "@li ssl-capath: the path to the directory that contains the "
              "X509 certificates authorities in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS3,
              "@li ssl-cert: The path to the X509 certificate in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS4,
              "@li ssl-key: The path to the X509 key in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS5,
              "@li ssl-crl: The path to file that contains certificate "
              "revocation lists.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS6,
              "@li ssl-crlpath: The path of directory that contains "
              "certificate revocation list files.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS7,
              "@li ssl-cipher: SSL Cipher to use.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS8,
    "@li tls-version: List of protocols permitted for secure connections");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS9,
              "@li auth-method: Authentication method");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS10,
    "@li get-server-public-key: Request public key from the server required "
    "for RSA key pair-based password exchange. Use when connecting to MySQL "
    "8.0 servers with classic MySQL sessions with SSL mode DISABLED.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS11,
    "@li server-public-key-path: The path name to a file containing a "
    "client-side copy of the public key required by the server for RSA key "
    "pair-based password exchange. Use when connecting to MySQL 8.0 servers "
    "with classic MySQL sessions with SSL mode DISABLED.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS12,
              "When these options are defined in a URI, their values must be "
              "URL encoded.");

// These lines group the connection options available for dictionary
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS, "<b>Base Connection Options</b>");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS1,
              "@li scheme: the protocol to be used on the connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS2,
              "@li user: the MySQL user name to be used on the connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS3, "@li dbUser: alias for user.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS4,
              "@li password: the password to be used on the connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS5,
              "@li dbPassword: same as password.");
REGISTER_HELP(
    TOPIC_DICT_CONNECTION_OPTIONS6,
    "@li host: the hostname or IP address to be used on a TCP connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS7,
              "@li port: the port to be used in a TCP connection.");
REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS8,
              "@li socket: the socket file name to be used on a connection "
              "through unix sockets.");
REGISTER_HELP(
    TOPIC_DICT_CONNECTION_OPTIONS9,
    "@li schema: the schema to be selected once the connection is done.");

REGISTER_HELP(TOPIC_DICT_CONNECTION_OPTIONS10,
              "@attention The dbUser and dbPassword options are will be "
              "removed in a future release.");
// The rest of the lines provide additional details related to the connection
// option definition
REGISTER_HELP(TOPIC_CONNECTION_DATA_DETAILS,
              "The connection options are case insensitive and can only be "
              "defined once.");
REGISTER_HELP(
    TOPIC_CONNECTION_DATA_DETAILS1,
    "If an option is defined more than once, an error will be generated.");

REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL,
              "${TOPIC_CONNECTION_OPTION_SCHEME}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL1,
              "${TOPIC_CONNECTION_OPTION_SOCKET}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL2,
              "${TOPIC_CONNECTION_OPTION_SSL_MODE}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL3,
              "${TOPIC_CONNECTION_OPTION_TLS_VERSION}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL4, "${TOPIC_URI_ENCODED_VALUE}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL5,
              "${TOPIC_URI_ENCODED_ATTRIBUTE}");

REGISTER_HELP(TOPIC_CONNECTION_OPTION_SCHEME, "<b>Protocol Selection</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SCHEME1,
              "The scheme option defines the protocol to be used on the "
              "connection, the following are the accepted values:");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SCHEME2,
              "@li mysql: for connections using the Classic protocol.");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SCHEME3,
              "@li mysqlx: for connections using the X protocol.");

REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET, "<b>Socket Connections</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET1,
              "To define a socket connection, replace the host and port by the "
              "socket path.");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET2,
              "When using a connection dictionary, the path is set as the "
              "value for the socket option.");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET3,
              "When using a URI, the socket path must be URL encoded. A socket "
              "path may be specified in a URI in one of the following ways:");

#ifdef _WIN32
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET4,
              "@li \\.d%3A%5Cpath%5Cto%5Csocket.sock");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET5,
              "@li \\.(d:%5Cpath%5Cto%5Csocket.sock)");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET6,
              "@li \\.(d:/path/to/socket.sock)");
#else
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET4, "@li /path%2Fto%2Fsocket.sock");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET5, "@li (/path/to/socket.sock)");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SOCKET6, "");
#endif

REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE, "<b>SSL Mode</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE1,
              "The ssl-mode option accepts the following values:");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE2, "@li DISABLED");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE3, "@li PREFERRED");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE4, "@li REQUIRED");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE5, "@li VERIFY_CA");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_SSL_MODE6, "@li VERIFY_IDENTITY");

REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION, "<b>TLS Version</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION1,
              "The tls-version option accepts the following values:");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION2, "@li TLSv1");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION3, "@li TLSv1.1");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_TLS_VERSION4,
              "@li TLSv1.2 (Supported only on commercial packages)");

REGISTER_HELP(TOPIC_URI_ENCODED_VALUE, "<b>URL Encoding</b>");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE1,
              "URL encoded values only accept alphanumeric characters and the "
              "next symbols: -._~!$'()*+;");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE2,
              "Any other character must be URL encoded.");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE3,
              "URL encoding is done by replacing the character being encoded "
              "by the sequence: %XX");
REGISTER_HELP(
    TOPIC_URI_ENCODED_VALUE4,
    "Where XX is the hexadecimal ASCII value of the character being encoded.");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE5,
              "If host is a literal IPv6 address it should be enclosed in "
              "\"[\" and \"]\" characters.");

REGISTER_HELP_FUNCTION(connect, shell);
REGISTER_HELP(SHELL_CONNECT_BRIEF, "Establishes the shell global session.");
REGISTER_HELP(SHELL_CONNECT_PARAM,
              "@param connectionData the connection data to be used to "
              "establish the session.");
REGISTER_HELP(SHELL_CONNECT_PARAM1,
              "@param password Optional the password to be used when "
              "establishing the session.");
REGISTER_HELP(SHELL_CONNECT_DETAIL,
              "This function will establish the global session with the "
              "received connection data.");
REGISTER_HELP(SHELL_CONNECT_DETAIL1, "${TOPIC_CONNECTION_DATA}");

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
 * Detailed description of the connection data format is available at \ref
 * connection_data
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

    _shell->connect(connection_options);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("connect"));

  return shcore::Value(
      std::static_pointer_cast<Object_bridge>(get_dev_session()));
}

void Shell::set_current_schema(const std::string &name) {
  auto session = _shell_core->get_dev_session();
  shcore::Value new_schema = shcore::Value::Null();

  if (!name.empty()) {
    session->set_current_schema(name);

    auto x_session =
        std::dynamic_pointer_cast<mysqlsh::mysqlx::Session>(session);
    if (x_session) new_schema = shcore::Value(x_session->get_schema(name));
  }

  _shell_core->set_global("db", new_schema,
                          shcore::IShell_core::all_scripting_modes());
}

REGISTER_HELP_FUNCTION(setCurrentSchema, shell);
REGISTER_HELP(SHELL_SETCURRENTSCHEMA_BRIEF,
              "Sets the active schema on the global session.");
REGISTER_HELP(SHELL_SETCURRENTSCHEMA_PARAM,
              "@param name The name of the schema to be set as active.");
REGISTER_HELP(SHELL_SETCURRENTSCHEMA_DETAIL,
              "Once the schema is set as active, it will be available through "
              "the <b>db</b> global object.");
/**
 * $(SHELL_SETCURRENTSCHEMA_BRIEF)
 *
 * $(SHELL_SETCURRENTSCHEMA_PARAM)
 *
 * $(SHELL_SETCURRENTSCHEMA_DETAIL)
 */
#if DOXYGEN_JS
Undefined Shell::setCurrentSchema(String name) {}
#elif DOXYGEN_PY
None Shell::set_current_schema(str name) {}
#endif

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
    shcore::Value schema = shcore::Value::Null();

    auto x_session =
        std::dynamic_pointer_cast<mysqlsh::mysqlx::Session>(session);
    if (x_session && !currentSchema.empty())
      schema = shcore::Value(x_session->get_schema(currentSchema));

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

REGISTER_HELP_FUNCTION(setSession, shell);
REGISTER_HELP(SHELL_SETSESSION_BRIEF, "Sets the global session.");
REGISTER_HELP(
    SHELL_SETSESSION_PARAM,
    "@param session The session object to be used as global session.");
REGISTER_HELP(SHELL_SETSESSION_DETAIL,
              "Sets the global session using a session from a variable.");
/**
 * $(SHELL_SETSESSION_BRIEF)
 *
 * $(SHELL_SETSESSION_PARAM)
 *
 * $(SHELL_SETSESSION_DETAIL)
 */
#if DOXYGEN_JS
Undefined Shell::setSession(Session session) {}
#elif DOXYGEN_PY
None Shell::set_session(Session session) {}
#endif
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

REGISTER_HELP_FUNCTION(getSession, shell);
REGISTER_HELP(SHELL_GETSESSION_BRIEF, "Returns the global session.");
REGISTER_HELP(SHELL_GETSESSION_DETAIL,
              "The returned object will be either a <b>ClassicSession</b> or a "
              "<b>Session</b> object, depending on how the global session was "
              "created.");
/**
 * $(SHELL_GETSESSION_BRIEF)
 */
#if DOXYGEN_JS
Session Shell::getSession() {}
#elif DOXYGEN_PY
Session Shell::get_session() {}
#endif
shcore::Value Shell::get_session(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("getSession").c_str());

  return shcore::Value(std::dynamic_pointer_cast<shcore::Object_bridge>(
      _shell_core->get_dev_session()));
}

REGISTER_HELP_FUNCTION(reconnect, shell);
REGISTER_HELP(SHELL_RECONNECT_BRIEF, "Reconnect the global session.");
/**
 * $(SHELL_RECONNECT_BRIEF)
 */
#if DOXYGEN_JS
Undefined Shell::reconnect() {}
#elif DOXYGEN_PY
None Shell::reconnect() {}
#endif
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

REGISTER_HELP_FUNCTION(log, shell);
REGISTER_HELP(SHELL_LOG_BRIEF, "Logs an entry to the shell's log file.");
REGISTER_HELP(SHELL_LOG_PARAM,
              "@param level one of ERROR, WARNING, INFO, "
              "DEBUG, DEBUG2 as a string");
REGISTER_HELP(SHELL_LOG_PARAM1, "@param message the text to be logged");
REGISTER_HELP(
    SHELL_LOG_DETAIL,
    "Only messages that have a level value equal "
    "to or lower than the active one (set via --log-level) are logged.");

/**
 * $(SHELL_LOG_BRIEF)
 *
 * $(SHELL_LOG_PARAM)
 * $(SHELL_LOG_PARAM1)
 *
 * $(SHELL_LOG_DETAIL)
 */
#if DOXYGEN_JS
Undefined Shell::log(String level, String message) {}
#elif DOXYGEN_PY
None Shell::log(str level, str message) {}
#endif
shcore::Value Shell::log(const shcore::Argument_list &args) {
  args.ensure_count(2, get_function_name("log").c_str());

  try {
    ngcommon::Logger::LOG_LEVEL level = ngcommon::Logger::LOG_INFO;
    level = ngcommon::Logger::get_log_level(args.string_at(0));
    if (level == ngcommon::Logger::LOG_NONE)
      throw shcore::Exception::argument_error("Invalid log level '" +
                                              args[0].descr() + "'");
    ngcommon::Logger::log(level, nullptr, "%s", args.string_at(1).c_str());
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("log"));

  return shcore::Value();
}

REGISTER_HELP_FUNCTION(status, shell);
REGISTER_HELP(SHELL_STATUS_BRIEF,
              "Shows connection status info for the shell.");
REGISTER_HELP(SHELL_STATUS_DETAIL,
              "This shows the same information shown by the \\status command.");

/**
 * $(SHELL_STATUS_BRIEF)
 *
 * $(SHELL_STATUS_DETAIL)
 */
#if DOXYGEN_JS
Undefined Shell::status() {}
#elif DOXYGEN_PY
None Shell::status() {}
#endif
shcore::Value Shell::status(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("status").c_str());

  try {
    _shell->cmd_status({});
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("status"));

  return shcore::Value();
}

REGISTER_HELP_FUNCTION(listCredentialHelpers, shell);
REGISTER_HELP(SHELL_LISTCREDENTIALHELPERS_BRIEF,
              "Returns a list of strings, where each string is a name of a "
              "helper available on the current platform.");
REGISTER_HELP(
    SHELL_LISTCREDENTIALHELPERS_RETURNS,
    "@returns A list of string with names of available credential helpers.");
REGISTER_HELP(SHELL_LISTCREDENTIALHELPERS_DETAIL,
              "The special values \"default\" and \"@<disabled>\" "
              "are not on the list.");
REGISTER_HELP(SHELL_LISTCREDENTIALHELPERS_DETAIL1,
              "Only values on this list (plus \"default\" and "
              "\"@<disabled>\") can be used to set the "
              "\"credentialStore.helper\" option.");

/**
 * $(SHELL_LISTCREDENTIALHELPERS_BRIEF)
 *
 * $(SHELL_LISTCREDENTIALHELPERS_RETURNS)
 *
 * $(SHELL_LISTCREDENTIALHELPERS_DETAIL)
 * $(SHELL_LISTCREDENTIALHELPERS_DETAIL1)
 */
#if DOXYGEN_JS
List Shell::listCredentialHelpers() {}
#elif DOXYGEN_PY
list Shell::list_credential_helpers() {}
#endif
shcore::Value Shell::list_credential_helpers(
    const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("listCredentialHelpers").c_str());

  shcore::Value ret_val;

  try {
    shcore::Value::Array_type_ref values =
        std::make_shared<shcore::Value::Array_type>();

    for (const auto &helper :
         shcore::Credential_manager::get().list_credential_helpers()) {
      values->emplace_back(helper);
    }

    ret_val = shcore::Value(values);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("listCredentialHelpers"));

  return ret_val;
}

REGISTER_HELP_FUNCTION(storeCredential, shell);
REGISTER_HELP(SHELL_STORECREDENTIAL_BRIEF,
              "Stores given credential using the configured helper.");
REGISTER_HELP(SHELL_STORECREDENTIAL_PARAM,
              "@param url URL of the server for the password to be stored.");
REGISTER_HELP(SHELL_STORECREDENTIAL_PARAM1,
              "@param password Optional Password for the given URL.");
REGISTER_HELP(SHELL_STORECREDENTIAL_THROWS,
              "Throws ArgumentError if URL has invalid form.");
REGISTER_HELP(SHELL_STORECREDENTIAL_THROWS1,
              "Throws RuntimeError in the following scenarios:");
REGISTER_HELP(SHELL_STORECREDENTIAL_THROWS2,
              "@li if configured credential helper is invalid.");
REGISTER_HELP(SHELL_STORECREDENTIAL_THROWS3,
              "@li if storing the credential fails.");
REGISTER_HELP(SHELL_STORECREDENTIAL_DETAIL,
              "If password is not provided, displays password prompt.");
REGISTER_HELP(SHELL_STORECREDENTIAL_DETAIL1,
              "If URL is already in storage, it's value is overwritten.");
REGISTER_HELP(SHELL_STORECREDENTIAL_DETAIL2,
              "URL needs to be in the following form: "
              "<b>user@(host[:port]|socket)</b>.");

/**
 * $(SHELL_STORECREDENTIAL_BRIEF)
 *
 * $(SHELL_STORECREDENTIAL_PARAM)
 * $(SHELL_STORECREDENTIAL_PARAM1)
 *
 * $(SHELL_STORECREDENTIAL_THROWS)
 *
 * $(SHELL_STORECREDENTIAL_THROWS1)
 * $(SHELL_STORECREDENTIAL_THROWS2)
 * $(SHELL_STORECREDENTIAL_THROWS3)
 *
 * $(SHELL_STORECREDENTIAL_DETAIL)
 * $(SHELL_STORECREDENTIAL_DETAIL1)
 * $(SHELL_STORECREDENTIAL_DETAIL2)
 */
#if DOXYGEN_JS
Undefined Shell::storeCredential(String url, String password) {}
#elif DOXYGEN_PY
None Shell::store_credential(str url, str password) {}
#endif
shcore::Value Shell::store_credential(const shcore::Argument_list &args) {
  args.ensure_count(1, 2, get_function_name("storeCredential").c_str());

  try {
    auto url = args.string_at(0);
    std::string password;

    if (args.size() == 1) {
      const std::string prompt =
          "Please provide the password for '" + url + "': ";
      const auto result = current_console()->prompt_password(prompt, &password);
      if (result != shcore::Prompt_result::Ok) {
        throw shcore::cancelled("Cancelled");
      }
    } else {
      password = args.string_at(1);
    }

    shcore::Credential_manager::get().store_credential(url, password);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("storeCredential"));

  return shcore::Value();
}

REGISTER_HELP_FUNCTION(deleteCredential, shell);
REGISTER_HELP(
    SHELL_DELETECREDENTIAL_BRIEF,
    "Deletes credential for the given URL using the configured helper.");
REGISTER_HELP(SHELL_DELETECREDENTIAL_PARAM,
              "@param url URL of the server to delete.");
REGISTER_HELP(SHELL_DELETECREDENTIAL_THROWS,
              "Throws ArgumentError if URL has invalid form.");
REGISTER_HELP(SHELL_DELETECREDENTIAL_THROWS1,
              "Throws RuntimeError in the following scenarios:");
REGISTER_HELP(SHELL_DELETECREDENTIAL_THROWS2,
              "@li if configured credential helper is invalid.");
REGISTER_HELP(SHELL_DELETECREDENTIAL_THROWS3,
              "@li if deleting the credential fails.");
REGISTER_HELP(SHELL_DELETECREDENTIAL_DETAIL,
              "URL needs to be in the following form: "
              "<b>user@(host[:port]|socket)</b>.");

/**
 * $(SHELL_DELETECREDENTIAL_BRIEF)
 *
 * $(SHELL_DELETECREDENTIAL_PARAM)
 *
 * $(SHELL_DELETECREDENTIAL_THROWS)
 *
 * $(SHELL_DELETECREDENTIAL_THROWS1)
 * $(SHELL_DELETECREDENTIAL_THROWS2)
 * $(SHELL_DELETECREDENTIAL_THROWS3)
 *
 * $(SHELL_DELETECREDENTIAL_DETAIL)
 */
#if DOXYGEN_JS
Undefined Shell::deleteCredential(String url) {}
#elif DOXYGEN_PY
None Shell::delete_credential(str url) {}
#endif
shcore::Value Shell::delete_credential(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("deleteCredential").c_str());

  try {
    shcore::Credential_manager::get().delete_credential(args.string_at(0));
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("deleteCredential"));

  return shcore::Value();
}

REGISTER_HELP_FUNCTION(deleteAllCredentials, shell);
REGISTER_HELP(SHELL_DELETEALLCREDENTIALS_BRIEF,
              "Deletes all credentials managed by the configured helper.");
REGISTER_HELP(SHELL_DELETEALLCREDENTIALS_THROWS,
              "Throws RuntimeError in the following scenarios:");
REGISTER_HELP(SHELL_DELETEALLCREDENTIALS_THROWS1,
              "@li if configured credential helper is invalid.");
REGISTER_HELP(SHELL_DELETEALLCREDENTIALS_THROWS2,
              "@li if deleting the credentials fails.");

/**
 * $(SHELL_DELETEALLCREDENTIALS_BRIEF)
 *
 * $(SHELL_DELETEALLCREDENTIALS_THROWS)
 * $(SHELL_DELETEALLCREDENTIALS_THROWS1)
 * $(SHELL_DELETEALLCREDENTIALS_THROWS2)
 */
#if DOXYGEN_JS
Undefined Shell::deleteAllCredentials() {}
#elif DOXYGEN_PY
None Shell::delete_all_credentials() {}
#endif
shcore::Value Shell::delete_all_credentials(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("deleteAllCredentials").c_str());

  try {
    shcore::Credential_manager::get().delete_all_credentials();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("deleteAllCredentials"));

  return shcore::Value();
}

REGISTER_HELP_FUNCTION(listCredentials, shell);
REGISTER_HELP(SHELL_LISTCREDENTIALS_BRIEF,
              "Retrieves a list of all URLs stored by the configured helper.");
REGISTER_HELP(SHELL_LISTCREDENTIALS_THROWS,
              "Throws RuntimeError in the following scenarios:");
REGISTER_HELP(SHELL_LISTCREDENTIALS_THROWS1,
              "@li if configured credential helper is invalid.");
REGISTER_HELP(SHELL_LISTCREDENTIALS_THROWS2, "@li if listing the URLs fails.");
REGISTER_HELP(
    SHELL_LISTCREDENTIALS_RETURNS,
    "@returns A list of URLs stored by the configured credential helper.");

/**
 * $(SHELL_LISTCREDENTIALS_BRIEF)
 *
 * $(SHELL_LISTCREDENTIALS_THROWS)
 * $(SHELL_LISTCREDENTIALS_THROWS1)
 * $(SHELL_LISTCREDENTIALS_THROWS2)
 *
 * $(SHELL_LISTCREDENTIALS_RETURNS)
 */
#if DOXYGEN_JS
List Shell::listCredentials() {}
#elif DOXYGEN_PY
list Shell::list_credentials() {}
#endif
shcore::Value Shell::list_credentials(const shcore::Argument_list &args) {
  args.ensure_count(0, get_function_name("listCredentials").c_str());

  shcore::Value ret_val;

  try {
    shcore::Value::Array_type_ref values =
        std::make_shared<shcore::Value::Array_type>();

    for (const auto &credential :
         shcore::Credential_manager::get().list_credentials()) {
      values->emplace_back(credential);
    }

    ret_val = shcore::Value(values);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("listCredentials"));

  return ret_val;
}

}  // namespace mysqlsh
