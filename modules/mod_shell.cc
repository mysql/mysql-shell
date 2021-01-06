/*
 * Copyright (c) 2016, 2021, Oracle and/or its affiliates.
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
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/devapi/base_database_object.h"
#include "modules/devapi/mod_mysqlx_schema.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/shell_notifications.h"
#include "mysqlshdk/include/shellcore/shell_resultset_dumper.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/shellcore/credential_manager.h"

using namespace std::placeholders;

namespace mysqlsh {
REGISTER_HELP_FUNCTION_MODE(dir, shellapi, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(SHELLAPI_DIR, R"*(
Returns a list of enumerable properties on the target object.

@param object The object whose properties will be listed.
@returns The list of enumerable properties on the object.

Traverses the object retrieving its enumerable properties. The content of the
returned list will depend on the object:

@li For a dictionary, returns the list of keys.
@li For an API object, returns the list of members.

Behavior of the function passing other types of objects is undefined and also
unsupported.
)*");
#if DOXYGEN_JS
/**
 * \ingroup ShellAPI
 * $(SHELLAPI_DIR_BRIEF)
 *
 * $(SHELLAPI_DIR)
 */
Array dir(Object object){};
#endif

REGISTER_HELP_FUNCTION_MODE(require, shellapi, JAVASCRIPT);
REGISTER_HELP_FUNCTION_TEXT(SHELLAPI_REQUIRE, R"*(
Loads the specified JavaScript module.

@param module_name_or_path The name or a path to the module to be loaded.
@returns The exported content of the loaded module.

The module_name_or_path parameter can be either a name of the built-in module
(i.e. mysql or mysqlx) or a path to the JavaScript module on the local file
system. The local module is searched for in the following folders:
@li if module_name_or_path begins with either './' or '../' characters, use
the folder which contains the JavaScipt file or module which is currently
being executed or the current working directory if there is no such file or
module (i.e. shell is running in interactive mode),
@li folders listed in the sys.path variable.

The file containing the module to be loaded is located by iterating through
these folders, for each folder path:
@li append module_name_or_path, if such file exists, use it,
@li append module_name_or_path, append '.js' extension, if such file exists,
use it,
@li append module_name_or_path, if such folder exists, append 'init.js' file
name, if such file exists, use it.

The loaded module has access to the following variables:
@li exports - an empty object, module should use it to export its functionalities;
this is the value returned from the require() function,
@li module - a module object, contains the exports object described above; can be
used to i.e. change the type of exports or store module-specific data,
@li __filename - absolute path to the module file,
@li __dirname - absolute path to the directory containing the module file.

Each module is loaded only once, any subsequent call to require() which would use
the same module will return a cached value instead.

If two modules form a cycle (try to load each other using the require() function),
one of them is going to receive an unfinished copy of the other ones exports object.

Here is a sample module called <b>test.js</b> which stores some data in the module
object and exports a function <b>callme()</b>:
@code
  module.counter = 0;

  exports.callme = function() {
    const plural = ++module.counter > 1 ? 's' : '';
    println(`I was called ${module.counter} time${plural}.`);
  };
@endcode

If placed in the current working directory, it can be used in shell as follows:
@code
  mysql-js> var test = require('./test');
  mysql-js> test.callme();
  I was called 1 time.
  mysql-js> test.callme();
  I was called 2 times.
  mysql-js> test.callme();
  I was called 3 times.
@endcode

@throw TypeError in the following scenarios:
@li if module_name_or_path is not a string.

@throw Error in the following scenarios:
@li if module_name_or_path is empty,
@li if module_name_or_path contains a backslash character,
@li if module_name_or_path is an absolute path,
@li if local module could not be found,
@li if local module could not be loaded.
)*");
#if DOXYGEN_JS
/**
 * \ingroup ShellAPI
 * $(SHELLAPI_REQUIRE_BRIEF)
 *
 * $(SHELLAPI_REQUIRE)
 */
Any require(String module_name_or_path) {}
#endif

REGISTER_HELP_GLOBAL_OBJECT(shell, shellapi);
REGISTER_HELP(SHELL_BRIEF,
              "Gives access to general purpose functions and properties.");
REGISTER_HELP(SHELL_GLOBAL_BRIEF,
              "Gives access to general purpose functions and properties.");

Shell::Shell(Mysql_shell *owner)
    : _shell(owner),
      _shell_core(owner->shell_context().get()),
      _core_options(std::make_shared<Options>(owner->get_options())),
      m_reports(std::make_shared<Shell_reports>("reports", "shell.reports")) {
  init();
}

void Shell::init() {
  add_property("options");
  add_property("reports");
  add_property("version");

  expose("parseUri", &Shell::parse_uri, "uri")->cli(false);
  expose("unparseUri", &Shell::unparse_uri, "options")->cli(false);
  expose("prompt", &Shell::prompt, "message", "?options")->cli(false);
  expose("setCurrentSchema", &Shell::set_current_schema, "name")->cli(false);
  expose("setSession", &Shell::set_session, "session")->cli(false);
  expose("getSession", &Shell::get_session)->cli(false);
  expose("log", &Shell::log, "level", "message")->cli(false);
  expose("status", &Shell::status)->cli();
  expose("listCredentialHelpers", &Shell::list_credential_helpers)->cli();
  expose("storeCredential", &Shell::store_credential, "url", "?password")
      ->cli();
  expose("deleteCredential", &Shell::delete_credential, "url")->cli();
  expose("deleteAllCredentials", &Shell::delete_all_credentials)->cli();
  expose("listCredentials", &Shell::list_credentials)->cli();
  expose("enablePager", &Shell::enable_pager)->cli(false);
  expose("disablePager", &Shell::disable_pager)->cli(false);
  expose("registerReport", &Shell::register_report, "name", "type", "report",
         "?description")
      ->cli(false);
  expose("createExtensionObject", &Shell::create_extension_object)->cli(false);
  expose("addExtensionObjectMember", &Shell::add_extension_object_member,
         "object", "name", "function", "?definition")
      ->cli(false);
  expose("registerGlobal", &Shell::register_global, "name", "object",
         "?definition")
      ->cli(false);
  expose("dumpRows", &Shell::dump_rows, "resultset", "?format", "table")
      ->cli(false);
  expose("connect", &Shell::connect, "connectionData", "?password")->cli(false);
  expose("disconnect", &Shell::disconnect)->cli(false);
  expose("reconnect", &Shell::reconnect)->cli(false);
  expose("connectToPrimary", &Shell::connect_to_primary, "?connectionData",
         "?password")
      ->cli(false);
  expose("openSession", &Shell::open_session, "connectionData", "?password")
      ->cli(false);
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

// Documentation of shell.reports
REGISTER_HELP(SHELL_REPORTS_BRIEF,
              "Gives access to built-in and user-defined reports.");

/**
 * $(SHELL_REPORTS_BRIEF)
 */
#if DOXYGEN_JS
Reports Shell::reports;
#elif DOXYGEN_PY
Reports Shell::reports;
#endif

// Documentation of shell.version
REGISTER_HELP(SHELL_VERSION_BRIEF, "MySQL Shell version information.");

/**
 * $(SHELL_VERSION_BRIEF)
 */
#if DOXYGEN_JS
String Shell::version;
#elif DOXYGEN_PY
str Shell::version;
#endif

shcore::Value Shell::get_member(const std::string &prop) const {
  shcore::Value ret_val;

  if (prop == "options") {
    ret_val =
        shcore::Value(std::static_pointer_cast<Object_bridge>(_core_options));
  } else if (prop == "reports") {
    ret_val = shcore::Value(m_reports);
  } else if (prop == "version") {
    ret_val = shcore::Value(shcore::get_long_version());
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
shcore::Dictionary_t Shell::parse_uri(const std::string &uri) {
  return mysqlsh::get_connection_map(
      mysqlsh::get_connection_options(shcore::Value(uri)));
}

REGISTER_HELP_FUNCTION(unparseUri, shell);
REGISTER_HELP_FUNCTION_TEXT(SHELL_UNPARSEURI, R"*(
Formats the given connection options to a URI string suitable for mysqlsh.

@param options a dictionary with the connection options.
@returns A URI string

This function assembles a MySQL connection string which can be used in the
shell or X DevAPI connectors.
)*");

/**
 * $(SHELL_UNPARSEURI_BRIEF)
 *
 * $(SHELL_UNPARSEURI)
 */
#if DOXYGEN_JS
String Shell::unparseUri(Dictionary options) {}
#elif DOXYGEN_PY
str Shell::unparse_uri(dict options) {}
#endif
std::string Shell::unparse_uri(const shcore::Dictionary_t &options) {
  return mysqlsh::get_connection_options(shcore::Value(options))
      .as_uri(mysqlshdk::db::uri::formats::full());
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
std::string Shell::prompt(const std::string &message,
                          const shcore::Dictionary_t &options) {
  std::string ret_val;
  std::string default_value;
  bool password = false;

  if (options) {
    // If there are options, reads them to determine how to proceed
    mysqlshdk::db::nullable<std::string> type;

    Unpack_options(options)
        .optional("defaultValue", &default_value)
        .optional("type", &type)
        .end();

    if (type) {
      if (*type != "password")
        throw shcore::Exception::runtime_error(
            "Unsupported value for parameter 'type', allowed values: "
            "password");
      else
        password = true;
    }
  }

  // Performs the actual prompt
  const auto console = mysqlsh::current_console();

  shcore::Prompt_result result;
  if (password)
    result = console->prompt_password(message, &ret_val);
  else
    result = console->prompt(message, &ret_val);
  if (result == shcore::Prompt_result::Cancel)
    throw shcore::cancelled("Cancelled");

  bool r = (result == shcore::Prompt_result::Ok);

  // Uses the default value if needed (but not if canceled)
  if (!default_value.empty() && (r && ret_val.empty())) ret_val = default_value;

  return ret_val;
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

#ifdef DOXYGEN

REGISTER_HELP_DETAIL_TEXT(TOPIC_CONNECTION_OPTIONS_DOXYGEN, R"*(
$(TOPIC_CONNECTION_DATA_BASIC)
$(TOPIC_CONNECTION_DATA_BASIC1)
$(TOPIC_CONNECTION_DATA_BASIC2)

$(TOPIC_URI)

$(TOPIC_URI1)

$(TOPIC_CONNECTION_OPTIONS)

$(TOPIC_CONNECTION_OPTIONS1)

$(TOPIC_URI_CONNECTION_OPTIONS)
$(TOPIC_URI_CONNECTION_OPTIONS1)
$(TOPIC_URI_CONNECTION_OPTIONS2)
$(TOPIC_URI_CONNECTION_OPTIONS3)
$(TOPIC_URI_CONNECTION_OPTIONS4)
$(TOPIC_URI_CONNECTION_OPTIONS5)
$(TOPIC_URI_CONNECTION_OPTIONS6)
$(TOPIC_URI_CONNECTION_OPTIONS7)
$(TOPIC_URI_CONNECTION_OPTIONS8)
$(TOPIC_URI_CONNECTION_OPTIONS9)
$(TOPIC_URI_CONNECTION_OPTIONS10)
$(TOPIC_URI_CONNECTION_OPTIONS11)
$(TOPIC_URI_CONNECTION_OPTIONS12)
$(TOPIC_URI_CONNECTION_OPTIONS13)
$(TOPIC_URI_CONNECTION_OPTIONS14)
$(TOPIC_URI_CONNECTION_OPTIONS15)
$(TOPIC_URI_CONNECTION_OPTIONS16)
$(TOPIC_URI_CONNECTION_OPTIONS17)

$(TOPIC_URI_CONNECTION_OPTIONS18)


$(TOPIC_DICT_CONNECTION_OPTIONS)

$(TOPIC_DICT_CONNECTION_OPTIONS1)
$(TOPIC_DICT_CONNECTION_OPTIONS2)
$(TOPIC_DICT_CONNECTION_OPTIONS3)
$(TOPIC_DICT_CONNECTION_OPTIONS4)
$(TOPIC_DICT_CONNECTION_OPTIONS5)
$(TOPIC_DICT_CONNECTION_OPTIONS6)
$(TOPIC_DICT_CONNECTION_OPTIONS7)
$(TOPIC_DICT_CONNECTION_OPTIONS8)
$(TOPIC_DICT_CONNECTION_OPTIONS9)

$(TOPIC_DICT_CONNECTION_OPTIONS10)

$(TOPIC_CONNECTION_DATA_DETAILS)

$(TOPIC_CONNECTION_DATA_DETAILS1)
)*");

REGISTER_HELP(TOPIC_CONNECTION_MORE_INFO,
              "For additional information about MySQL connection data, see "
              "@ref connection_data.");
#else
REGISTER_HELP(TOPIC_CONNECTION_MORE_INFO,
              "For additional information on connection data use "
              "\\? connection.");
#endif

REGISTER_HELP(TOPIC_URI, "A basic URI string has the following format:");
REGISTER_HELP(TOPIC_URI1,
              "[scheme://][user[:password]@]<host[:port]|socket>[/schema]"
              "[?option=value&option=value...]");

// These lines group the description of ALL the available connection options
REGISTER_HELP(TOPIC_CONNECTION_OPTIONS, "<b>Connection Options</b>");
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
              "@li ssl-mode: The SSL mode to be used in the connection.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS1,
              "@li ssl-ca: The path to the X509 certificate authority file in "
              "PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS2,
              "@li ssl-capath: The path to the directory that contains the "
              "X509 certificate authority files in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS3,
              "@li ssl-cert: The path to the SSL public key certificate file "
              "in PEM format.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS4,
    "@li ssl-key: The path to the SSL private key file in PEM format.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS5,
              "@li ssl-crl: The path to file that contains certificate "
              "revocation lists.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS6,
              "@li ssl-crlpath: The path of directory that contains "
              "certificate revocation list files.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS7,
              "@li ssl-cipher: The list of permissible encryption ciphers for "
              "connections that use TLS protocols up through TLSv1.2.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS8,
    "@li tls-version: List of protocols permitted for secure connections.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS9,
              "@li tls-ciphers: List of TLS v1.3 ciphers to use.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS10,
              "@li auth-method: Authentication method.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS11,
    "@li get-server-public-key: Request public key from the server required "
    "for RSA key pair-based password exchange. Use when connecting to MySQL "
    "8.0 servers with classic MySQL sessions with SSL mode DISABLED.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS12,
    "@li server-public-key-path: The path name to a file containing a "
    "client-side copy of the public key required by the server for RSA key "
    "pair-based password exchange. Use when connecting to MySQL 8.0 servers "
    "with classic MySQL sessions with SSL mode DISABLED.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS13,
    "@li connect-timeout: The connection timeout in milliseconds. If not "
    "provided a default timeout of 10 seconds will be used. Specifying a value "
    "of 0 disables the connection timeout.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS14,
              "@li compression: Enable compression in client/server protocol.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS15,
    "@li compression-algorithms: Use compression algorithm in server/client "
    "protocol.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS16,
              "@li compression-level: Use this compression level in the "
              "client/server protocol.");
REGISTER_HELP(
    TOPIC_URI_CONNECTION_OPTIONS17,
    "@li connection-attributes: List of connection attributes to be "
    "registered at the PERFORMANCE_SCHEMA connection attributes tables.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS18,
              "@li local-infile: Enable/disable LOAD DATA LOCAL INFILE.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS19,
              "@li net-buffer-length: The buffer size for TCP/IP and socket "
              "communication.");
REGISTER_HELP(TOPIC_URI_CONNECTION_OPTIONS20,
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
    "@li host: the hostname or IP address to be used on the connection.");
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

REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL, "${TOPIC_CONNECTION_TYPES}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL1,
              "${TOPIC_CONNECTION_OPTION_SSL_MODE}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL2,
              "${TOPIC_CONNECTION_OPTION_TLS_VERSION}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL3,
              "${TOPIC_CONNECTION_OPTION_AUTH_METHOD}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL4,
              "${TOPIC_CONNECTION_COMPRESSION}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL5,
              "${TOPIC_CONNECTION_ATTRIBUTES}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL6, "${TOPIC_URI_ENCODED_VALUE}");
REGISTER_HELP(TOPIC_CONNECTION_DATA_ADDITIONAL7,
              "${TOPIC_URI_ENCODED_ATTRIBUTE}");

REGISTER_HELP_TOPIC(Connection Types, TOPIC, TOPIC_CONNECTION_TYPES, Contents,
                    ALL);
REGISTER_HELP_TOPIC_TEXT(TOPIC_CONNECTION_TYPES, R"*(
The options specified in the connection data determine the type of connection
to be used.

The scheme option defines the protocol to be used on the connection, the
following are the accepted values:

@li mysql: for connections using the MySQL protocol.
@li mysqlx: for connections using the X protocol.

If no protocol is specified in the connection data, the shell will first
attempt connecting using the X protocol, if the connection fails it will then
try to connect using the MySQL protocol.

In general, the Shell connects to the server using TCP connections, unless the
connection data contains the options required to create any of the connections
described below.

<b>Unix-domain Socket Connections</b>

To connect to a local MySQL server using a Unix-domain socket, the host must
be set to 'localhost', no port number should be provided and the socket path
should be provided.

When using the MySQL protocol, the socket path might not be provided, in such
case a default path for the socket file will be used.

When using a connection dictionary, the socket path is set as the value for the
socket option.

When using a URI, the socket path must be URL encoded as follows:

@li user@@/path%2Fto%2Fsocket.sock
@li user@@./path%2Fto%2Fsocket.sock
@li user@@../path%2Fto%2Fsocket.sock

It is possible to skip the URL encoding by enclosing the socket path in
parenthesis:

@li user@@(/path/to/socket.sock)
@li user@@(./path/to/socket.sock)
@li user@@(../path/to/socket.sock)

<b>Windows Named Pipe Connections</b>

To connect to MySQL server using a named pipe the host must be set to '.', no
port number should be provided.

If the pipe name is not provided the default pipe name will be used: MySQL.

When using a connection dictionary, the named pipe name is set as the value for
the socket option.

When using a URI, if the named pipe has invalid characters for a URL, they must
be URL encoded. URL encoding can be skipped by enclosing the pipe name in
parenthesis:

@li user@@\\\\.\\named.pipe
@li user@@(\\\\.\\named.pipe)

Named pipe connections are only supported on the MySQL protocol.

<b>Windows Shared Memory Connections</b>

Shared memory connections are only allowed if the server has shared memory
connections enabled using the default shared memory base name: MySQL.

To connect to a local MySQL server using shared memory the host should be set to
'localhost' and no port should be provided.

If the server does not have shared memory connections enabled using the default
base name, the connection will be done using TCP.

Shared memory connections are only supported on the MySQL protocol.
)*");

REGISTER_HELP_TOPIC(Connection Attributes, TOPIC, TOPIC_CONNECTION_ATTRIBUTES,
                    Contents, ALL);
REGISTER_HELP_TOPIC_TEXT(TOPIC_CONNECTION_ATTRIBUTES, R"*(
<b>Connection Attributes</b>

Connection attributes are key-value pairs to be sent to the server at connect
time. They are stored at the following PERFORMANCE_SCHEMA tables:

@li session_account_connect_attrs: attributes for the current session, and
other sessions associated with the session account.
@li session_connect_attrs: attributes for all sessions.

These attributes should be defined when creating a session and are immutable
during the life-time of the session.

To define connection attributes on a URI, the connection-attributes should be
defined as part of the URI as follows:

root@@localhost:port/schema?connection-attributes=[att1=value1,att2=val2,...]

Note that the characters used for the attribute name and value must follow the
URI standard, this is, if the character is not allowed it must be percent
encoded.

To define connection attributes when creating a session using a dictionary the
connection-attributes option should be defined, its value can be set in the
following formats:

@li Array of "key=value" pairs.
@li Dictionary containing the key-value pairs.

Note that the connection-attribute values are expected to be strings, if other
data type is used in the dictionary, the string representation of the used
data will be stored on the database.
)*");

REGISTER_HELP_TOPIC(Connection Compression, TOPIC, TOPIC_CONNECTION_COMPRESSION,
                    Contents, ALL);
REGISTER_HELP_TOPIC_TEXT(TOPIC_CONNECTION_COMPRESSION, R"*(
<b>Connection Compression</b>

Connection compression is governed by following connection options:
"compression", "compression-algorithms", and "compression-level".

"compression" accepts following values:

@li REQUIRED: connection will only be made when compression negotiation is
succesful.
@li PREFFERED: (default for X protocol connections) shell will attempt to
establish connection with compression enabled, but if compression negotiation
fails, connection will be established without compression.
@li DISABLED: (defalut for classic protocol connections) connection will be
established without compression.

For convenience "compression" also accepts Boolean: 'True', 'False', '1',
and '0' values which map to REQUIRED and DISABLED respectively.

"compression-algorithms" expects comma separated list of algorithms.
Supported algorithms include:

@li zstd
@li zlib
@li lz4 (X protocol only)
@li uncompressed - special value, which if it appears in the list, causes
connection to succeed even if compression negotiation fails.

If "compression" connection option is not defined, its value will be deduced
from "compression-algorithms" value when it is provided.

"compression-level" expects an integer value. Valid range depends on the
compression algorithm and server configuration, but generally following is
expected:

@li zstd: 1-22 (default 3)
@li zlib: 1-9 (default 3), supported only by X protocol
@li lz4: 0-16 (default 2), supported only by X protocol.
)*");

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

REGISTER_HELP(TOPIC_CONNECTION_OPTION_AUTH_METHOD,
              "<b>Authentication method</b>");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_AUTH_METHOD1,
              "In case of classic session, this is the name of the "
              "authentication plugin to use, i.e. caching_sha2_password.");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_AUTH_METHOD2,
              "In case of X protocol session, it should be one of:");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_AUTH_METHOD3, "@li AUTO,");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_AUTH_METHOD4, "@li FROM_CAPABILITIES,");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_AUTH_METHOD5, "@li FALLBACK,");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_AUTH_METHOD6, "@li MYSQL41,");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_AUTH_METHOD7, "@li PLAIN,");
REGISTER_HELP(TOPIC_CONNECTION_OPTION_AUTH_METHOD8, "@li SHA256_MEMORY.");

REGISTER_HELP(TOPIC_URI_ENCODED_VALUE, "<b>URL Encoding</b>");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE1,
              "URL encoded values only accept alphanumeric characters and the "
              "next symbols: -._~!$'()*+;");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE2,
              "Any other character must be URL encoded.");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE3,
              "URL encoding is done by replacing the character being encoded "
              "by the sequence: @%XX");
REGISTER_HELP(
    TOPIC_URI_ENCODED_VALUE4,
    "Where XX is the hexadecimal ASCII value of the character being encoded.");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE5,
              "If host is a literal IPv6 address it should be enclosed in "
              "\"[\" and \"]\" characters.");
REGISTER_HELP(TOPIC_URI_ENCODED_VALUE6,
              "If host is a literal IPv6 address with zone ID, the '@%' "
              "character separating address from the zone ID needs to be URL "
              "encoded.");

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
REGISTER_HELP(SHELL_CONNECT_DETAIL1,
              "The password may be included on the connectionData, the "
              "optional parameter should be used only "
              "if the connectionData does not contain it already. If both are "
              "specified the password parameter will override the password "
              "defined on the connectionData.");
REGISTER_HELP(SHELL_CONNECT_DETAIL2, "${TOPIC_CONNECTION_DATA}");
/**
 * $(SHELL_CONNECT_BRIEF)
 *
 * $(SHELL_CONNECT_PARAM)
 * $(SHELL_CONNECT_PARAM1)
 *
 * $(SHELL_CONNECT_DETAIL)
 *
 * $(SHELL_CONNECT_DETAIL1)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref
 * connection_data
 */
#if DOXYGEN_JS
Session Shell::connect(ConnectionData connectionData, String password) {}
#elif DOXYGEN_PY
Session Shell::connect(ConnectionData connectionData, str password) {}
#endif
std::shared_ptr<ShellBaseSession> Shell::connect(
    const mysqlshdk::db::Connection_options &connection_options_,
    const char *password) {
  auto connection_options = connection_options_;
  mysqlsh::set_password_from_string(&connection_options, password);
  return _shell->connect(connection_options);
}

REGISTER_HELP_FUNCTION(connectToPrimary, shell);
REGISTER_HELP_FUNCTION_TEXT(SHELL_CONNECTTOPRIMARY, R"*(
Establishes the shell global session, connecting to a primary of an InnoDB
cluster or ReplicaSet.

@param connectionData Optional The connection data to be used to establish the
session.
@param password Optional The password to be used when establishing the session.

@returns The established session.

Ensures that the target server is a member of an InnoDB cluster or ReplicaSet
and if it is not a PRIMARY, finds the PRIMARY and connects to it. Sets the
global session object to the established session and returns that object.

If connectionData is not given, this function uses the global shell session, if
there is none, an exception is raised.

${SHELL_CONNECT_DETAIL1}

${TOPIC_CONNECTION_DATA}

@throws RuntimeError in the following scenarios:
@li If connectionData was not given and there is no global shell session.
@li If there is no primary member of an InnoDB cluster or ReplicaSet.
@li If the target server is not a member of an InnoDB cluster or ReplicaSet.
)*");

/**
 * $(SHELL_CONNECTTOPRIMARY_BRIEF)
 *
 * $(SHELL_CONNECTTOPRIMARY)
 */
#if DOXYGEN_JS
Session Shell::connectToPrimary(ConnectionData connectionData,
                                String password) {}
#elif DOXYGEN_PY
Session Shell::connect_to_primary(ConnectionData connectionData, str password) {
}
#endif
std::shared_ptr<ShellBaseSession> Shell::connect_to_primary(
    const mysqlshdk::db::Connection_options &co_, const char *password) {
  auto co = co_;

  if (co.has_data()) {
    mysqlsh::set_password_from_string(&co, password);
  }

  if (!_shell->redirect_session_if_needed(false, co)) {
    current_console()->print_note("Already connected to a PRIMARY.");
  }

  return get_dev_session();
}

REGISTER_HELP_FUNCTION(openSession, shell);
REGISTER_HELP_FUNCTION_TEXT(SHELL_OPENSESSION, R"*(
Establishes and returns session.

@param connectionData the connection data to be used to establish the session.
@param password Optional the password to be used when establishing the session.

@returns The session object.

This function will establish the session with the received connection data.

The password may be included on the connectionData, the optional parameter
should be used only if the connectionData does not contain it already.
If both are specified the password parameter will override the password
defined on the connectionData.

${TOPIC_CONNECTION_DATA}
)*");

/**
 * $(SHELL_OPENSESSION_BRIEF)
 *
 * $(SHELL_OPENSESSION_DETAIL)
 *
 * Detailed description of the connection data format is available at
 * \ref connection_data
 */
#if DOXYGEN_JS
Session Shell::openSession(ConnectionData connectionData, String password) {}
#elif DOXYGEN_PY
Session Shell::open_session(ConnectionData connectionData, str password) {}
#endif
std::shared_ptr<ShellBaseSession> Shell::open_session(
    const mysqlshdk::db::Connection_options &connection_options_,
    const char *password) {
  auto connection_options = connection_options_;
  mysqlsh::set_password_from_string(&connection_options, password);
  return _shell->connect(connection_options, false, false);
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
void Shell::set_current_schema(const std::string &name) {
  auto session = _shell_core->get_dev_session();

  if (!(session && session->is_open())) {
    throw shcore::Exception::runtime_error(
        "An open session is required to perform this operation.");
  }

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
void Shell::set_session(
    const std::shared_ptr<mysqlsh::ShellBaseSession> &session) {
  _shell_core->set_dev_session(session);
  set_session_global(session);
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
std::shared_ptr<ShellBaseSession> Shell::get_session() {
  return _shell_core->get_dev_session();
}

REGISTER_HELP_FUNCTION(disconnect, shell);
REGISTER_HELP(SHELL_DISCONNECT_BRIEF, "Disconnects the global session.");

/**
 * $(SHELL_DISCONNECT_BRIEF)
 */
#if DOXYGEN_JS
Undefined Shell::disconnect() {}
#elif DOXYGEN_PY
None Shell::disconnect() {}
#endif
void Shell::disconnect() { _shell->cmd_disconnect({}); }

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
bool Shell::reconnect() {
  bool ret_val = false;

  try {
    _shell_core->get_dev_session()->reconnect();
    ret_val = true;
  } catch (const shcore::Exception &e) {
    ret_val = false;
  }

  return ret_val;
}

REGISTER_HELP_FUNCTION(log, shell);
REGISTER_HELP(SHELL_LOG_BRIEF, "Logs an entry to the shell's log file.");
REGISTER_HELP(SHELL_LOG_PARAM,
              "@param level one of ERROR, WARNING, INFO, "
              "DEBUG, DEBUG2, DEBUG3 as a string");
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
void Shell::log(const std::string &str_level, const std::string &message) {
  shcore::Logger::LOG_LEVEL level;
  try {
    level = shcore::Logger::parse_log_level(str_level);
  } catch (...) {
    throw shcore::Exception::argument_error("Invalid log level '" + str_level +
                                            "'");
  }
  shcore::Logger::log(level, "%s", message.c_str());
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
void Shell::status() { _shell->cmd_status({}); }

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
shcore::Array_t Shell::list_credential_helpers() {
  shcore::Array_t ret_val = shcore::make_array();

  for (const auto &helper :
       shcore::Credential_manager::get().list_credential_helpers()) {
    ret_val->emplace_back(helper);
  }

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
void Shell::store_credential(
    const std::string &url,
    const mysqlshdk::utils::nullable<std::string> &password) {
  std::string pwd_to_store;
  if (password.is_null()) {
    const std::string prompt =
        "Please provide the password for '" + url + "': ";
    const auto result =
        current_console()->prompt_password(prompt, &pwd_to_store);
    if (result != shcore::Prompt_result::Ok) {
      throw shcore::cancelled("Cancelled");
    }
  } else {
    pwd_to_store = *password;
  }

  shcore::Credential_manager::get().store_credential(url, pwd_to_store);
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
void Shell::delete_credential(const std::string &url) {
  shcore::Credential_manager::get().delete_credential(url);
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
void Shell::delete_all_credentials() {
  shcore::Credential_manager::get().delete_all_credentials();
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
shcore::Array_t Shell::list_credentials() {
  shcore::Array_t ret_val = shcore::make_array();

  for (const auto &credential :
       shcore::Credential_manager::get().list_credentials()) {
    ret_val->emplace_back(credential);
  }

  return ret_val;
}

REGISTER_HELP_FUNCTION(enablePager, shell);
REGISTER_HELP(SHELL_ENABLEPAGER_BRIEF,
              "Enables pager specified in <b>shell.options.pager</b> for the "
              "current scripting mode.");
REGISTER_HELP(SHELL_ENABLEPAGER_DETAIL,
              "All subsequent text output (except for prompts and user "
              "interaction) is going to be forwarded to the pager.");
REGISTER_HELP(SHELL_ENABLEPAGER_DETAIL1,
              "This behavior is in effect until <<<disablePager>>>() is called "
              "or current scripting mode is changed.");
REGISTER_HELP(SHELL_ENABLEPAGER_DETAIL2,
              "Changing the scripting mode has the same effect as calling "
              "<<<disablePager>>>().");
REGISTER_HELP(SHELL_ENABLEPAGER_DETAIL3,
              "If the value of <b>shell.options.pager</b> option is changed "
              "after this method has been called, the new pager will be "
              "automatically used.");
REGISTER_HELP(SHELL_ENABLEPAGER_DETAIL4,
              "If <b>shell.options.pager</b> option is set to an empty string "
              "when this method is called, pager will not be active until "
              "<b>shell.options.pager</b> is set to a non-empty string.");
REGISTER_HELP(SHELL_ENABLEPAGER_DETAIL5,
              "This method has no effect in non-interactive mode.");

/**
 * $(SHELL_ENABLEPAGER_BRIEF)
 *
 * $(SHELL_ENABLEPAGER_DETAIL)
 *
 * $(SHELL_ENABLEPAGER_DETAIL1)
 *
 * $(SHELL_ENABLEPAGER_DETAIL2)
 *
 * $(SHELL_ENABLEPAGER_DETAIL3)
 *
 * $(SHELL_ENABLEPAGER_DETAIL4)
 *
 * $(SHELL_ENABLEPAGER_DETAIL5)
 */
#if DOXYGEN_JS
Undefined Shell::enablePager() {}
#elif DOXYGEN_PY
None Shell::enable_pager() {}
#endif
void Shell::enable_pager() { current_console()->enable_global_pager(); }

REGISTER_HELP_FUNCTION(disablePager, shell);
REGISTER_HELP(SHELL_DISABLEPAGER_BRIEF,
              "Disables pager for the current scripting mode.");
REGISTER_HELP(SHELL_DISABLEPAGER_DETAIL,
              "The current value of <b>shell.options.pager</b> option is not "
              "changed by calling this method.");
REGISTER_HELP(SHELL_DISABLEPAGER_DETAIL1,
              "This method has no effect in non-interactive mode.");

/**
 * $(SHELL_DISABLEPAGER_BRIEF)
 *
 * $(SHELL_DISABLEPAGER_DETAIL)
 *
 * $(SHELL_DISABLEPAGER_DETAIL1)
 */
#if DOXYGEN_JS
Undefined Shell::disablePager() {}
#elif DOXYGEN_PY
None Shell::disable_pager() {}
#endif
void Shell::disable_pager() { current_console()->disable_global_pager(); }

REGISTER_HELP_FUNCTION(registerReport, shell);
REGISTER_HELP(SHELL_REGISTERREPORT_BRIEF,
              "Registers a new user-defined report.");

REGISTER_HELP(SHELL_REGISTERREPORT_PARAM,
              "@param name Name of the registered report.");
REGISTER_HELP(SHELL_REGISTERREPORT_PARAM1,
              "@param type Type of the registered report, one of: 'list', "
              "'report' or 'print'.");
REGISTER_HELP(
    SHELL_REGISTERREPORT_PARAM2,
    "@param report Function to be called when the report is requested.");
REGISTER_HELP(SHELL_REGISTERREPORT_PARAM3,
              "@param description Optional Dictionary describing the report "
              "being registered.");

REGISTER_HELP(SHELL_REGISTERREPORT_THROWS,
              "Throws ArgumentError in the following scenarios:");
REGISTER_HELP(SHELL_REGISTERREPORT_THROWS1,
              "@li if a report with the same name is already registered.");
REGISTER_HELP(SHELL_REGISTERREPORT_THROWS2,
              "@li if 'name' of a report is not a valid scripting identifier.");
REGISTER_HELP(SHELL_REGISTERREPORT_THROWS3,
              "@li if 'type' is not one of: 'list', 'report' or 'print'.");
REGISTER_HELP(
    SHELL_REGISTERREPORT_THROWS4,
    "@li if 'name' of a report option is not a valid scripting identifier.");
REGISTER_HELP(
    SHELL_REGISTERREPORT_THROWS5,
    "@li if 'name' of a report option is reused in multiple options.");
REGISTER_HELP(
    SHELL_REGISTERREPORT_THROWS6,
    "@li if 'shortcut' of a report option is not an alphanumeric character.");
REGISTER_HELP(
    SHELL_REGISTERREPORT_THROWS7,
    "@li if 'shortcut' of a report option is reused in multiple options.");
REGISTER_HELP(SHELL_REGISTERREPORT_THROWS8,
              "@li if 'type' of a report option holds an invalid value.");
REGISTER_HELP(SHELL_REGISTERREPORT_THROWS9,
              "@li if the 'argc' key of a 'description' dictionary holds an "
              "invalid value.");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL,
              "The <b>name</b> of the report must be unique and a valid "
              "scripting identifier. A case-insensitive comparison is used to "
              "validate the uniqueness.");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL1,
              "The <b>type</b> of the report must be one of: 'list', 'report' "
              "or 'print'. This option specifies the expected result of "
              "calling this report as well as the output format if it is "
              "invoked using <b>\\show</b> or <b>\\watch</b> commands.");

REGISTER_HELP(
    SHELL_REGISTERREPORT_DETAIL2,
    "The <b>report</b> function must have the following signature: "
    "<b>Dict report(Session session, List argv, Dict options)</b>, where:");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL3, "${REPORTS_DETAIL4}");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL4, "${REPORTS_DETAIL5}");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL5, "${REPORTS_DETAIL6}");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL6, "${REPORTS_DETAIL7}");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL7, "${REPORTS_DETAIL8}");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL8,
              "The <b>description</b> dictionary may contain the following "
              "optional keys:");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL9,
              "@li brief - A string value providing a brief description of "
              "the report.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL10,
              "@li details - A list of strings providing a detailed "
              "description of the report.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL11,
              "@li options - A list of dictionaries describing the options "
              "accepted by the report. If this is not provided, the report "
              "does not accept any options.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL12,
              "@li argc - A string representing the number of additional "
              "arguments accepted by the report. This string can be either: a "
              "number specifying exact number of arguments, <b>*</b> "
              "specifying zero or more arguments, two numbers separated by a "
              "'-' specifying a range of arguments or a number and <b>*</b> "
              "separated by a '-' specifying a range of arguments without an "
              "upper bound. If this is not provided, the report does not "
              "accept any additional arguments.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL13,
              "@li examples - A list of dictionaries describing the example "
              "usage of the report.");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL14,
              "The optional <b>options</b> list must hold dictionaries with "
              "the following keys:");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL15,
              "@li name (string, required) - Name of the option, must be a "
              "valid scripting identifier. This specifies an option name in "
              "the long form (--long) when invoking the report using "
              "<b>\\show</b> or <b>\\watch</b> commands or a key name of an "
              "option when calling this report as a function. Must be unique "
              "for a report.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL16,
              "@li shortcut (string, optional) - alternate name of the option, "
              "must be an alphanumeric character. This specifies an option "
              "name in the short form (-s). The short form of an option can "
              "only be used when invoking the report using <b>\\show</b> or "
              "<b>\\watch</b> commands, it is not available when calling the "
              "report as a function. If this key is not specified, option will "
              "not have a short form. Must be unique for a report.");
REGISTER_HELP(
    SHELL_REGISTERREPORT_DETAIL17,
    "@li brief (string, optional) - brief description of the option.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL18,
              "@li details (array of strings, optional) - detailed description "
              "of the option.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL19,
              "@li type (string, optional) - value type of the option. Allowed "
              "values are: 'string', 'bool', 'integer', 'float'. If this key "
              "is not specified it defaults to 'string'. If type is specified "
              "as 'bool', this option is a switch: if it is not specified when "
              "invoking the report it defaults to 'false'; if it's specified "
              "when invoking the report using <b>\\show</b> or <b>\\watch</b> "
              "commands it does not accept any value and defaults to 'true'; "
              "if it is specified when invoking the report using the function "
              "call it must have a valid value.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL20,
              "@li required (Boolean, optional) - whether this option is "
              "required. If this key is not specified, defaults to false. If "
              "option is a 'bool' then 'required' cannot be 'true'.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL21,
              "@li values (list of strings, optional) - list of allowed "
              "values. Only 'string' options may have this key. If this key is "
              "not specified, this option accepts any values.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL22,
              "@li empty (Boolean, optional) - whether this option accepts "
              "empty strings. Only 'string' options may have this key. If this "
              "key is not specified, defaults to false.");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL23,
              "The optional <b>examples</b> list must hold dictionaries with "
              "the following keys:");
REGISTER_HELP(
    SHELL_REGISTERREPORT_DETAIL24,
    "@li description (string, required) - Description text of the example.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL25,
              "@li args (list of strings, optional) - List of the arguments "
              "used in the example.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL26,
              "@li options (dictionary of strings, optional) - Options used in "
              "the example.");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL27,
              "The type of the report determines the expected result of a "
              "report invocation:");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL28,
              "@li The 'list' report returns a list of lists of values, with "
              "the first item containing the names of the columns and "
              "remaining ones containing the rows with values.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL29,
              "@li The 'report' report returns a list with a single item.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL30,
              "@li The 'print' report returns an empty list.");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL31,
              "The type of the report also determines the output form when "
              "report is called using <b>\\show</b> or <b>\\watch</b> "
              "commands:");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL32,
              "@li The 'list' report will be displayed in tabular form (or "
              "vertical if --vertical option is used).");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL33,
              "@li The 'report' report will be displayed in YAML format.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL34,
              "@li The 'print' report will not be formatted by Shell, the "
              "report itself will print out any output.");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL35,
              "The registered report is can be called using <b>\\show</b> or "
              "<b>\\watch</b> commands in any of the scripting modes.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL36,
              "The registered report is also going to be available as a method "
              "of the <b>shell.reports</b> object.");

REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL37,
              "Users may create custom report files in the <b>init.d</b> "
              "folder located in the Shell configuration path (by default it "
              "is <b>~/.mysqlsh/init.d</b> in Unix and "
              "<b>@%AppData@%\\MySQL\\mysqlsh\\init.d</b> in Windows).");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL38,
              "Custom reports may be written in either JavaScript or Python. "
              "The standard file extension for each case should be used to get "
              "them properly loaded.");
REGISTER_HELP(SHELL_REGISTERREPORT_DETAIL39,
              "All reports registered in those files using the "
              "<<<registerReport>>>() method will be available when Shell "
              "starts.");

/**
 * $(SHELL_REGISTERREPORT_BRIEF)
 *
 * $(SHELL_REGISTERREPORT_PARAM)
 * $(SHELL_REGISTERREPORT_PARAM1)
 * $(SHELL_REGISTERREPORT_PARAM2)
 * $(SHELL_REGISTERREPORT_PARAM3)
 *
 * $(SHELL_REGISTERREPORT_THROWS)
 * $(SHELL_REGISTERREPORT_THROWS1)
 * $(SHELL_REGISTERREPORT_THROWS2)
 * $(SHELL_REGISTERREPORT_THROWS3)
 * $(SHELL_REGISTERREPORT_THROWS4)
 * $(SHELL_REGISTERREPORT_THROWS5)
 * $(SHELL_REGISTERREPORT_THROWS6)
 * $(SHELL_REGISTERREPORT_THROWS7)
 * $(SHELL_REGISTERREPORT_THROWS8)
 * $(SHELL_REGISTERREPORT_THROWS9)
 *
 * $(SHELL_REGISTERREPORT_DETAIL)
 *
 * $(SHELL_REGISTERREPORT_DETAIL1)
 *
 * $(SHELL_REGISTERREPORT_DETAIL2)
 * $(REPORTS_DETAIL4)
 * $(REPORTS_DETAIL5)
 * $(REPORTS_DETAIL6)
 *
 * $(REPORTS_DETAIL7)
 * $(REPORTS_DETAIL8)
 *
 * $(SHELL_REGISTERREPORT_DETAIL8)
 * $(SHELL_REGISTERREPORT_DETAIL9)
 * $(SHELL_REGISTERREPORT_DETAIL10)
 * $(SHELL_REGISTERREPORT_DETAIL11)
 * $(SHELL_REGISTERREPORT_DETAIL12)
 * $(SHELL_REGISTERREPORT_DETAIL13)
 *
 * $(SHELL_REGISTERREPORT_DETAIL14)
 * $(SHELL_REGISTERREPORT_DETAIL15)
 * $(SHELL_REGISTERREPORT_DETAIL16)
 * $(SHELL_REGISTERREPORT_DETAIL17)
 * $(SHELL_REGISTERREPORT_DETAIL18)
 * $(SHELL_REGISTERREPORT_DETAIL19)
 * $(SHELL_REGISTERREPORT_DETAIL20)
 * $(SHELL_REGISTERREPORT_DETAIL21)
 * $(SHELL_REGISTERREPORT_DETAIL22)
 *
 * $(SHELL_REGISTERREPORT_DETAIL23)
 * $(SHELL_REGISTERREPORT_DETAIL24)
 * $(SHELL_REGISTERREPORT_DETAIL25)
 * $(SHELL_REGISTERREPORT_DETAIL26)
 *
 * $(SHELL_REGISTERREPORT_DETAIL27)
 * $(SHELL_REGISTERREPORT_DETAIL28)
 * $(SHELL_REGISTERREPORT_DETAIL29)
 * $(SHELL_REGISTERREPORT_DETAIL30)
 *
 * $(SHELL_REGISTERREPORT_DETAIL31)
 * $(SHELL_REGISTERREPORT_DETAIL32)
 * $(SHELL_REGISTERREPORT_DETAIL33)
 * $(SHELL_REGISTERREPORT_DETAIL34)
 *
 * $(SHELL_REGISTERREPORT_DETAIL35)
 * $(SHELL_REGISTERREPORT_DETAIL36)
 *
 * $(SHELL_REGISTERREPORT_DETAIL37)
 * $(SHELL_REGISTERREPORT_DETAIL38)
 * $(SHELL_REGISTERREPORT_DETAIL39)
 */
#if DOXYGEN_JS
Undefined Shell::registerReport(String name, String type, Function report,
                                Dictionary description) {}
#elif DOXYGEN_PY
None Shell::register_report(str name, str type, Function report,
                            dict description) {}
#endif
void Shell::register_report(const std::string &name, const std::string &type,
                            const shcore::Function_base_ref &report,
                            const shcore::Dictionary_t &description) {
  m_reports->register_report(name, type, report, description);

  log_debug("The '%s' report of type '%s' has been registered.", name.c_str(),
            type.c_str());
}

REGISTER_HELP_FUNCTION(createExtensionObject, shell);
REGISTER_HELP_FUNCTION_TEXT(SHELL_CREATEEXTENSIONOBJECT, R"*(
Creates an extension object, it can be used to extend shell functionality.

An extension object is defined by adding members in it (properties and
functions).

An extension object can be either added as a property of another extension
object or registered as a shell global object.

An extension object is usable only when it has been registered as a global
object or when it has been added into another extension object that is member
in an other registered object.
)*");
/**
 * $(SHELL_CREATEEXTENSIONOBJECT_BRIEF)
 *
 * $(SHELL_CREATEEXTENSIONOBJECT)
 */
#if DOXYGEN_JS
ExtensionObject Shell::createExtensionObject() {}
#elif DOXYGEN_PY
ExtensionObject Shell::create_extension_object() {}
#endif
std::shared_ptr<Extensible_object> Shell::create_extension_object() {
  return std::make_shared<Extensible_object>("", "");
}

REGISTER_HELP_FUNCTION(addExtensionObjectMember, shell);
REGISTER_HELP_FUNCTION_TEXT(SHELL_ADDEXTENSIONOBJECTMEMBER, R"*(
Adds a member to an extension object.

@param object The object to which the member will be added.
@param name The name of the member being added.
@param member The member being added.
@param definition Optional dictionary with help information about the member.

The name parameter must be a valid identifier, this is, it should follow the
pattern [_|a..z|A..Z]([_|a..z|A..Z|0..9])*

The member parameter can be any of:

@li An extension object
@li A function
@li Scalar values: boolean, integer, float, string
@li Array
@li Dictionary
@li None/Null

When an extension object is added as a member, a read only property will be
added into the target extension object.

When a function is added as member, it will be callable on the extension object
but it will not be possible to overwrite it with a new value or function.

When any of the other types are added as a member, a read/write property will
be added into the target extension object, it will be possible to update the value
without any restriction.

IMPORTANT: every member added into an extensible object will be available only
when the object is registered, this is, registered as a global shell object,
or added as a member of another object that is already registered.

The definition parameter is an optional and can be used to define additional
information for the member.

The member definition accepts the following attributes:

@li brief: optional string containing a brief description of the member being
added.
@li details: optional array of strings containing detailed information about
the member being added.

This information will be integrated into the shell help to be available through
the built in help system (\\?).

When adding a function, the following attribute is also allowed on the member
definition:

@li parameters: a list of parameter definitions for each parameter that the
function accepts.

A parameter definition is a dictionary with the following attributes:

@li name: required, the name of the parameter, must be a valid identifier.
@li type: required, the data type of the parameter, allowed values include:
string, integer, bool, float, array, dictionary, object.
@li required: a boolean value indicating whether the parameter is mandatory or
optional.
@li brief: a string with a short description of the parameter.
@li details: a string array with additional details about the parameter.

The information defined on the brief and details attributes will also be added
to the function help on the built in help system (\\?).

If a parameter's type is 'string' the following attribute is also allowed on
the parameter definition dictionary:

@li values: string array with the only values that are allowed for the
parameter.

If a parameter's type is 'object' the following attributes are also allowed on
the parameter definition dictionary:

@li class: string defining the class of object allowed as parameter values.
@li classes: string array defining multiple classes of objects allowed as
parameter values.

The values for the class(es) properties must be a valid class exposed through
the different APIs. For details use:

@li \? mysql
@li \? mysqlx
@li \? adminapi
@li \? shellapi

To get the class name for a global object or a registered extension call the
print function passing as parameter the object, i.e. "Shell" is the class name
for the built in shell global object:<br>

@code
mysql-js> print(shell)
<Shell>
mysql-js>
@endcode

If a parameter's type is 'dictionary' the following attribute is also allowed
on the parameter definition dictionary:

@li options: list of option definition dictionaries defining the allowed
options that can be passed on the dictionary parameter.

An option definition dictionary follows exactly the same rules as the
parameter definition dictionary except for one: on a parameter definition
dictionary, required parameters must be defined first, option definition
dictionaries do not have this restriction.
)*");
/**
 * $(SHELL_ADDEXTENSIONOBJECTMEMBER_BRIEF)
 *
 * $(SHELL_ADDEXTENSIONOBJECTMEMBER)
 */
#if DOXYGEN_JS
Undefined Shell::addExtensionObjectMember(Object object, String name,
                                          Value member, Dictionary definition);
{}
#elif DOXYGEN_PY
None Shell::add_extension_object_member(Object object, str name, Value member,
                                        dict definition) {}
#endif
void Shell::add_extension_object_member(
    std::shared_ptr<Extensible_object> object, const std::string &name,
    const shcore::Value &member, const shcore::Dictionary_t &definition) {
  if (object) {
    object->register_member(name, member, definition);
  } else {
    throw shcore::Exception::type_error(
        "Argument #1 is expected to be an extension object.");
  }
}

REGISTER_HELP_FUNCTION(registerGlobal, shell);
REGISTER_HELP_FUNCTION_TEXT(SHELL_REGISTERGLOBAL, R"*(
Registers an extension object as a shell global object.

@param name The name to be given to the registered global object.
@param object The extension object to be registered as a global object.
@param definition optional dictionary containing help information about the
object.

An extension object is created with the shell.<<<createExtensionObject>>>
function. Once registered, the functionality it provides will be available for
use.

The name parameter must comply with the following restrictions:

@li It must be a valid scripting identifier.
@li It can not be the name of a built in global object.
@li It can not be the name of a previously registered object.

The name used to register an object will be exactly the name under which the
object will be available for both Python and JavaScript modes, this is, no
naming convention is enforced on global objects.

The definition parameter is a dictionary with help information about the object
being registered, it allows the following properties:

@li brief: a string with a brief description of the object.
@li details: a list of strings with additional information about the object.

When the object is registered, the help data on the definition parameter as well
as the object members is made available through the shell built in help system.
(\\?).
)*");
/**
 * $(SHELL_REGISTERGLOBAL_BRIEF)
 *
 * $(SHELL_REGISTERGLOBAL)
 */
#if DOXYGEN_JS
UserObject Shell::registerGlobal(String name, Object object,
                                 Dictionary definition) {}
#elif DOXYGEN_PY
UserObject Shell::register_global(str name, Object object, dict definition) {}
#endif

void Shell::register_global(const std::string &name,
                            std::shared_ptr<Extensible_object> object,
                            const shcore::Dictionary_t &definition) {
  if (!shcore::is_valid_identifier(name)) {
    throw shcore::Exception::argument_error("The name '" + name +
                                            "' is not a valid identifier.");
  }

  // An object that is already child of another object can not be
  // registered as a global object
  if (object->get_parent()) {
    std::string error =
        "The provided extension object is already registered as part of "
        "another extension object";

    if (object->is_registered())
      error += " at: " + object->get_qualified_name();
    error += ".";

    throw shcore::Exception::argument_error(error);
  }

  // An object that is already a global object can not be
  // registered again as a global object
  if (object->is_registered()) {
    std::string error =
        "The provided extension object is already registered as: " +
        object->get_qualified_name();

    throw shcore::Exception::argument_error(error);
  }

  // Global validation: mysql, mysqlx, os and sys are not registered as global
  // variables but should be considered as shell globals.
  bool is_global = _shell_core->is_global(name);
  if (is_global) {
    throw shcore::Exception::argument_error("A global named '" + name +
                                            "' already exists.");
  }

  if (name == "mysql" || name == "mysqlx" || name == "os" || name == "sys") {
    throw shcore::Exception::argument_error("The name '" + name +
                                            "' is reserved.");
  }

  if (object) {
    auto def = std::make_shared<mysqlsh::Member_definition>();

    if (definition) {
      shcore::Option_unpacker unpacker(definition);
      unpacker.optional("brief", &def->brief);
      unpacker.optional("details", &def->details);
      unpacker.end("at object definition.");
    }

    object->set_definition(def);

    _shell_core->set_global(name, shcore::Value(object),
                            shcore::IShell_core::all_scripting_modes());

    object->set_registered(name);

    log_debug(
        "The '%s' extension object has been registered as a global object.",
        name.c_str());
  } else {
    throw shcore::Exception::type_error(
        "Argument #2 is expected to be an extension object.");
  }
}

REGISTER_HELP_FUNCTION(dumpRows, shell);
REGISTER_HELP_FUNCTION_TEXT(SHELL_DUMPROWS, R"*(
Formats and dumps the given resultset object to the console.

@param result The resultset object to dump
@param format One of table, tabbed, vertical, json, ndjson, json/raw,
json/array, json/pretty. Default is table.
@returns The number of printed rows

This function shows a resultset object returned by a DB Session query in
the same formats supported by the shell.

Note that the resultset will be consumed by the function.
)*");

/**
 * $(SHELL_DUMPROWS_BRIEF)
 *
 * $(SHELL_DUMPROWS)
 */
#if DOXYGEN_JS
Undefined Shell::dumpRows(ShellBaseResult result, String format) {}
#elif DOXYGEN_PY
None Shell::dump_rows(ShellBaseResult result, str format) {}
#endif
int Shell::dump_rows(const std::shared_ptr<ShellBaseResult> &resultset,
                     const std::string &format) {
  if (!resultset) throw std::invalid_argument("result object must not be NULL");

  if (!format.empty() && !Resultset_dumper_base::is_valid_format(format))
    throw std::invalid_argument("Invalid format " + format);

  Resultset_dumper dumper(resultset->get_result(), "off",
                          format.empty() ? "table" : format, false, false);
  return dumper.dump("row", false, false);
}

}  // namespace mysqlsh
