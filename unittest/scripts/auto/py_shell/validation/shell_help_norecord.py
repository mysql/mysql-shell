#@<OUT> shell
NAME
      shell - Gives access to general purpose functions and properties.

DESCRIPTION
      Gives access to general purpose functions and properties.

OBJECTS
 - options Gives access to options impacting shell behavior.
 - reports Gives access to built-in and user-defined reports.

FUNCTIONS
      connect(connectionData[, password])
            Establishes the shell global session.

      delete_all_credentials()
            Deletes all credentials managed by the configured helper.

      delete_credential(url)
            Deletes credential for the given URL using the configured helper.

      disable_pager()
            Disables pager for the current scripting mode.

      enable_pager()
            Enables pager specified in shell.options.pager for the current
            scripting mode.

      get_session()
            Returns the global session.

      help([member])
            Provides help about this object and it's members

      list_credential_helpers()
            Returns a list of strings, where each string is a name of a helper
            available on the current platform.

      list_credentials()
            Retrieves a list of all URLs stored by the configured helper.

      log(level, message)
            Logs an entry to the shell's log file.

      parse_uri(uri)
            Utility function to parse a URI string.

      prompt(message[, options])
            Utility function to prompt data from the user.

      reconnect()
            Reconnect the global session.

      register_report(name, type, report[, description])
            Registers a new user-defined report.

      set_current_schema(name)
            Sets the active schema on the global session.

      set_session(session)
            Sets the global session.

      status()
            Shows connection status info for the shell.

      store_credential(url[, password])
            Stores given credential using the configured helper.

#@<OUT> shell.connect
NAME
      connect - Establishes the shell global session.

SYNTAX
      shell.connect(connectionData[, password])

WHERE
      connectionData: the connection data to be used to establish the session.
      password: The password to be used when establishing the session.

DESCRIPTION
      This function will establish the global session with the received
      connection data.

      The connection data may be specified in the following formats:

      - A URI string
      - A dictionary with the connection options

      A basic URI string has the following format:

      [scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

      SSL Connection Options

      The following options are valid for use either in a URI or in a
      dictionary:

      - ssl-mode: the SSL mode to be used in the connection.
      - ssl-ca: the path to the X509 certificate authority in PEM format.
      - ssl-capath: the path to the directory that contains the X509
        certificates authorities in PEM format.
      - ssl-cert: The path to the X509 certificate in PEM format.
      - ssl-key: The path to the X509 key in PEM format.
      - ssl-crl: The path to file that contains certificate revocation lists.
      - ssl-crlpath: The path of directory that contains certificate revocation
        list files.
      - ssl-cipher: SSL Cipher to use.
      - tls-version: List of protocols permitted for secure connections
      - auth-method: Authentication method
      - get-server-public-key: Request public key from the server required for
        RSA key pair-based password exchange. Use when connecting to MySQL 8.0
        servers with classic MySQL sessions with SSL mode DISABLED.
      - server-public-key-path: The path name to a file containing a
        client-side copy of the public key required by the server for RSA key
        pair-based password exchange. Use when connecting to MySQL 8.0 servers
        with classic MySQL sessions with SSL mode DISABLED.
      - connect-timeout: The connection timeout in milliseconds. If not
        provided a default timeout of 10 seconds will be used. Specifying a
        value of 0 disables the connection timeout.
      - compression: Enable/disable compression in client/server protocol,
        valid values: "true", "false", "1", and "0".

      When these options are defined in a URI, their values must be URL
      encoded.

      The following options are also valid when a dictionary is used:

      Base Connection Options

      - scheme: the protocol to be used on the connection.
      - user: the MySQL user name to be used on the connection.
      - dbUser: alias for user.
      - password: the password to be used on the connection.
      - dbPassword: same as password.
      - host: the hostname or IP address to be used on a TCP connection.
      - port: the port to be used in a TCP connection.
      - socket: the socket file name to be used on a connection through unix
        sockets.
      - schema: the schema to be selected once the connection is done.

      ATTENTION: The dbUser and dbPassword options are will be removed in a
                 future release.

      The connection options are case insensitive and can only be defined once.

      If an option is defined more than once, an error will be generated.

      For additional information on connection data use \? connection.

      If no scheme is provided, a first attempt will be made to establish a
      NodeSession, and if it detects the used port is for the mysql protocol,
      it will attempt a ClassicSession

      The password may be included on the connectionData, the optional
      parameter should be used only if the connectionData does not contain it
      already. If both are specified the password parameter will override the
      password defined on the connectionData.

#@<OUT> shell.delete_all_credentials
NAME
      delete_all_credentials - Deletes all credentials managed by the
                               configured helper.

SYNTAX
      shell.delete_all_credentials()

EXCEPTIONS
      Throws RuntimeError in the following scenarios:

      - if configured credential helper is invalid.
      - if deleting the credentials fails.

#@<OUT> shell.delete_credential
NAME
      delete_credential - Deletes credential for the given URL using the
                          configured helper.

SYNTAX
      shell.delete_credential(url)

WHERE
      url: URL of the server to delete.

DESCRIPTION
      URL needs to be in the following form: user@(host[:port]|socket).

EXCEPTIONS
      Throws ArgumentError if URL has invalid form.

      Throws RuntimeError in the following scenarios:

      - if configured credential helper is invalid.
      - if deleting the credential fails.

#@<OUT> shell.disable_pager
NAME
      disable_pager - Disables pager for the current scripting mode.

SYNTAX
      shell.disable_pager()

DESCRIPTION
      The current value of shell.options.pager option is not changed by calling
      this method.

      This method has no effect in non-interactive mode.

#@<OUT> shell.enable_pager
NAME
      enable_pager - Enables pager specified in shell.options.pager for the
                     current scripting mode.

SYNTAX
      shell.enable_pager()

DESCRIPTION
      All subsequent text output (except for prompts and user interaction) is
      going to be forwarded to the pager.

      This behavior is in effect until disable_pager() is called or current
      scripting mode is changed.

      Changing the scripting mode has the same effect as calling
      disable_pager().

      If the value of shell.options.pager option is changed after this method
      has been called, the new pager will be automatically used.

      If shell.options.pager option is set to an empty string when this method
      is called, pager will not be active until shell.options.pager is set to a
      non-empty string.

      This method has no effect in non-interactive mode.

#@<OUT> shell.get_session
NAME
      get_session - Returns the global session.

SYNTAX
      shell.get_session()

DESCRIPTION
      The returned object will be either a ClassicSession or a Session object,
      depending on how the global session was created.

#@<OUT> shell.help
NAME
      help - Provides help about this object and it's members

SYNTAX
      shell.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> shell.list_credential_helpers
NAME
      list_credential_helpers - Returns a list of strings, where each string is
                                a name of a helper available on the current
                                platform.

SYNTAX
      shell.list_credential_helpers()

RETURNS
       A list of string with names of available credential helpers.

DESCRIPTION
      The special values "default" and "<disabled>" are not on the list.

      Only values on this list (plus "default" and "<disabled>") can be used to
      set the "credentialStore.helper" option.

#@<OUT> shell.list_credentials
NAME
      list_credentials - Retrieves a list of all URLs stored by the configured
                         helper.

SYNTAX
      shell.list_credentials()

RETURNS
       A list of URLs stored by the configured credential helper.

EXCEPTIONS
      Throws RuntimeError in the following scenarios:

      - if configured credential helper is invalid.
      - if listing the URLs fails.

#@<OUT> shell.log
NAME
      log - Logs an entry to the shell's log file.

SYNTAX
      shell.log(level, message)

WHERE
      level: one of ERROR, WARNING, INFO, DEBUG, DEBUG2 as a string
      message: the text to be logged

DESCRIPTION
      Only messages that have a level value equal to or lower than the active
      one (set via --log-level) are logged.

#@<OUT> shell.options
NAME
      options - Gives access to options impacting shell behavior.

SYNTAX
      shell.options

DESCRIPTION
      The options object acts as a dictionary, it may contain the following
      attributes:

      - autocomplete.nameCache: true if auto-refresh of DB object name cache is
        enabled. The \rehash command can be used for manual refresh
      - batchContinueOnError: read-only, boolean value to indicate if the
        execution of an SQL script in batch mode shall continue if errors occur
      - credentialStore.excludeFilters: array of URLs for which automatic
        password storage is disabled, supports glob characters '*' and '?'
      - credentialStore.helper: name of the credential helper to use to
        fetch/store passwords; a special value "default" is supported to use
        platform default helper; a special value "<disabled>" is supported to
        disable the credential store
      - credentialStore.savePasswords: controls automatic password storage,
        allowed values: "always", "prompt" or "never"
      - dba.gtidWaitTimeout: timeout value in seconds to wait for GTIDs to be
        synchronized
      - defaultCompress: Enable compression in client/server protocol by
        default in global shell sessions.
      - defaultMode: shell mode to use when shell is started, allowed values:
        "js", "py", "sql" or "none"
      - devapi.dbObjectHandles: true to enable schema collection and table name
        aliases in the db object, for DevAPI operations.
      - history.autoSave: true to save command history when exiting the shell
      - history.maxSize: number of entries to keep in command history
      - history.sql.ignorePattern: colon separated list of glob patterns to
        filter out of the command history in SQL mode
      - interactive: read-only, boolean value that indicates if the shell is
        running in interactive mode
      - logLevel: current log level
      - resultFormat: controls the type of output produced for SQL results.
      - pager: string which specifies the external command which is going to be
        used to display the paged output
      - passwordsFromStdin: boolean value that indicates if the shell should
        read passwords from stdin instead of the tty
      - sandboxDir: default path where the new sandbox instances for InnoDB
        cluster will be deployed
      - showColumnTypeInfo: display column type information in SQL mode. Please
        be aware that output may depend on the protocol you are using to
        connect to the server, e.g. DbType field is approximated when using X
        protocol.
      - showWarnings: boolean value to indicate whether warnings shall be
        included when printing an SQL result
      - useWizards: read-only, boolean value to indicate if the Shell is using
        the interactive wrappers (wizard mode)

      The resultFormat option supports the following values:

      - table: displays the output in table format (default)
      - json: displays the output in JSON format
      - json/raw: displays the output in a JSON format but in a single line
      - vertical: displays the outputs vertically, one line per column value

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      set(optionName, value)
            Sets value of an option.

      set_persist(optionName, value)
            Sets value of an option and stores it in the configuration file.

      unset(optionName)
            Resets value of an option to default.

      unset_persist(optionName)
            Resets value of an option to default and removes it from the
            configuration file.

#@<OUT> shell.parse_uri
NAME
      parse_uri - Utility function to parse a URI string.

SYNTAX
      shell.parse_uri(uri)

WHERE
      uri: a URI string.

RETURNS
       A dictionary containing all the elements contained on the given URI
      string.

DESCRIPTION
      Parses a URI string and returns a dictionary containing an item for each
      found element.

      A basic URI string has the following format:

      [scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

      For more details about how a URI is created as well as the returned
      dictionary, use \? connection

#@<OUT> shell.prompt
NAME
      prompt - Utility function to prompt data from the user.

SYNTAX
      shell.prompt(message[, options])

WHERE
      message: a string with the message to be shown to the user.
      options: Dictionary with options that change the function behavior.

RETURNS
       A string value containing the input from the user.

DESCRIPTION
      This function allows creating scripts that require interaction with the
      user to gather data.

      Calling this function with no options will show the given message to the
      user and wait for the input. The information entered by the user will be
      the returned value

      The options dictionary may contain the following options:

      - defaultValue: a string value to be returned if the provides no data.
      - type: a string value to define the prompt type.

      The type option supports the following values:

      - password: the user input will not be echoed on the screen.

#@<OUT> shell.reconnect
NAME
      reconnect - Reconnect the global session.

SYNTAX
      shell.reconnect()

#@<OUT> shell.reports
NAME
      reports - Gives access to built-in and user-defined reports.

SYNTAX
      shell.reports

DESCRIPTION
      The 'reports' object provides access to built-in reports.

      All user-defined reports registered using the shell.register_report()
      method are also available here.

      The reports are provided as methods of this object, with names
      corresponding to the names of the available reports.

      All methods have the same signature: Dict report(Session session, List
      argv, Dict options), where:

      - session - Session object used by the report to obtain the data.
      - argv (optional) - Array of strings representing additional arguments.
      - options (optional) - Dictionary with values for various report-specific
        options.

      Each report returns a dictionary with the following keys:

      - report (required) - List of JSON objects containing the report. The
        number and types of items in this list depend on type of the report.

      For more information on a report use: shell.reports.help('report_name').

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      query(session, argv)
            Executes the SQL statement given as arguments.

#@<OUT> shell.register_report
NAME
      register_report - Registers a new user-defined report.

SYNTAX
      shell.register_report(name, type, report[, description])

WHERE
      name: Name of the registered report.
      type: Type of the registered report, one of: 'list', 'report' or 'print'.
      report: Function to be called when the report is requested.
      description: Dictionary describing the report being registered.

DESCRIPTION
      The name of the report must be unique and a valid scripting identifier. A
      case-insensitive comparison is used to validate the uniqueness.

      The type of the report must be one of: 'list', 'report' or 'print'. This
      option specifies the expected result of calling this report as well as
      the output format if it is invoked using \show or \watch commands.

      The report function must have the following signature: Dict
      report(Session session, List argv, Dict options), where:

      - session - Session object used by the report to obtain the data.
      - argv (optional) - Array of strings representing additional arguments.
      - options (optional) - Dictionary with values for various report-specific
        options.

      Each report returns a dictionary with the following keys:

      - report (required) - List of JSON objects containing the report. The
        number and types of items in this list depend on type of the report.

      The description dictionary may contain the following optional keys:

      - brief - A string value providing a brief description of the report.
      - details - A list of strings providing a detailed description of the
        report.
      - options - A list of dictionaries describing the options accepted by the
        report. If this is not provided, the report does not accept any
        options.
      - argc - A string representing the number of additional arguments
        accepted by the report. This string can be either: a number specifying
        exact number of arguments, * specifying zero or more arguments, two
        numbers separated by a '-' specifying a range of arguments or a number
        and * separated by a '-' specifying a range of arguments without an
        upper bound. If this is not provided, the report does not accept any
        additional arguments.

      The optional options list must hold dictionaries with the following keys:

      - name (string, required) - Name of the option, must be a valid scripting
        identifier. This specifies an option name in the long form (--long)
        when invoking the report using \show or \watch commands or a key name
        of an option when calling this report as a function. Must be unique for
        a report.
      - shortcut (string, optional) - alternate name of the option, must be an
        alphanumeric character. This specifies an option name in the short form
        (-s). The short form of an option can only be used when invoking the
        report using \show or \watch commands, it is not available when calling
        the report as a function. If this key is not specified, option will not
        have a short form. Must be unique for a report.
      - brief (string, optional) - brief description of the option.
      - details (array of strings, optional) - detailed description of the
        option.
      - type (string, optional) - value type of the option. Allowed values are:
        'string', 'bool', 'integer', 'float'. If this key is not specified it
        defaults to 'string'. If type is specified as 'bool', this option is a
        switch: if it is not specified when invoking the report it defaults to
        'false'; if it's specified when invoking the report using \show or
        \watch commands it does not accept any value and defaults to 'true'; if
        it is specified when invoking the report using the function call it
        must have a valid value.
      - required (Boolean, optional) - whether this option is required. If this
        key is not specified, defaults to false. If option is a 'bool' then
        'required' cannot be 'true'.
      - values (list of strings, optional) - list of allowed values. Only
        'string' options may have this key. If this key is not specified, this
        option accepts any values.

      The type of the report determines the expected result of a report
      invocation:

      - The 'list' report returns a list of lists of values, with the first
        item containing the names of the columns and remaining ones containing
        the rows with values.
      - The 'report' report returns a list with a single item.
      - The 'print' report returns an empty list.

      The type of the report also determines the output form when report is
      called using \show or \watch commands:

      - The 'list' report will be displayed in tabular form (or vertical if
        --vertical option is used).
      - The 'report' report will be displayed in YAML format.
      - The 'print' report will not be formatted by Shell, the report itself
        will print out any output.

      The registered report is can be called using \show or \watch commands in
      any of the scripting modes.

      The registered report is also going to be available as a method of the
      shell.reports object.

      Users may create custom report files in the init.d folder located in the
      Shell configuration path (by default it is ~/.mysqlsh/init.d in Unix and
      %AppData%\MySQL\mysqlsh\init.d in Windows).

      Custom reports may be written in either JavaScript or Python. The
      standard file extension for each case should be used to get them properly
      loaded.

      All reports registered in those files using the register_report() method
      will be available when Shell starts.

EXCEPTIONS
      Throws ArgumentError in the following scenarios:

      - if a report with the same name is already registered.
      - if 'name' of a report is not a valid scripting identifier.
      - if 'type' is not one of: 'list', 'report' or 'print'.
      - if 'name' of a report option is not a valid scripting identifier.
      - if 'name' of a report option is reused in multiple options.
      - if 'shortcut' of a report option is not an alphanumeric character.
      - if 'shortcut' of a report option is reused in multiple options.
      - if 'type' of a report option holds an invalid value.
      - if the 'argc' key of a 'description' dictionary holds an invalid value.

#@<OUT> shell.set_current_schema
NAME
      set_current_schema - Sets the active schema on the global session.

SYNTAX
      shell.set_current_schema(name)

WHERE
      name: The name of the schema to be set as active.

DESCRIPTION
      Once the schema is set as active, it will be available through the db
      global object.

#@<OUT> shell.set_session
NAME
      set_session - Sets the global session.

SYNTAX
      shell.set_session(session)

WHERE
      session: The session object to be used as global session.

DESCRIPTION
      Sets the global session using a session from a variable.

#@<OUT> shell.status
NAME
      status - Shows connection status info for the shell.

SYNTAX
      shell.status()

DESCRIPTION
      This shows the same information shown by the \status command.

#@<OUT> shell.store_credential
NAME
      store_credential - Stores given credential using the configured helper.

SYNTAX
      shell.store_credential(url[, password])

WHERE
      url: URL of the server for the password to be stored.
      password: Password for the given URL.

DESCRIPTION
      If password is not provided, displays password prompt.

      If URL is already in storage, it's value is overwritten.

      URL needs to be in the following form: user@(host[:port]|socket).

EXCEPTIONS
      Throws ArgumentError if URL has invalid form.

      Throws RuntimeError in the following scenarios:

      - if configured credential helper is invalid.
      - if storing the credential fails.

#@<OUT> BUG28393119 UNABLE TO GET HELP ON CONNECTION DATA, before session
The connection data may be specified in the following formats:

- A URI string
- A dictionary with the connection options

A basic URI string has the following format:

[scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

SSL Connection Options

The following options are valid for use either in a URI or in a dictionary:

- ssl-mode: the SSL mode to be used in the connection.
- ssl-ca: the path to the X509 certificate authority in PEM format.
- ssl-capath: the path to the directory that contains the X509 certificates
  authorities in PEM format.
- ssl-cert: The path to the X509 certificate in PEM format.
- ssl-key: The path to the X509 key in PEM format.
- ssl-crl: The path to file that contains certificate revocation lists.
- ssl-crlpath: The path of directory that contains certificate revocation list
  files.
- ssl-cipher: SSL Cipher to use.
- tls-version: List of protocols permitted for secure connections
- auth-method: Authentication method
- get-server-public-key: Request public key from the server required for RSA
  key pair-based password exchange. Use when connecting to MySQL 8.0 servers
  with classic MySQL sessions with SSL mode DISABLED.
- server-public-key-path: The path name to a file containing a client-side copy
  of the public key required by the server for RSA key pair-based password
  exchange. Use when connecting to MySQL 8.0 servers with classic MySQL
  sessions with SSL mode DISABLED.
- connect-timeout: The connection timeout in milliseconds. If not provided a
  default timeout of 10 seconds will be used. Specifying a value of 0 disables
  the connection timeout.
- compression: Enable/disable compression in client/server protocol, valid
  values: "true", "false", "1", and "0".

When these options are defined in a URI, their values must be URL encoded.

The following options are also valid when a dictionary is used:

Base Connection Options

- scheme: the protocol to be used on the connection.
- user: the MySQL user name to be used on the connection.
- dbUser: alias for user.
- password: the password to be used on the connection.
- dbPassword: same as password.
- host: the hostname or IP address to be used on a TCP connection.
- port: the port to be used in a TCP connection.
- socket: the socket file name to be used on a connection through unix sockets.
- schema: the schema to be selected once the connection is done.

ATTENTION: The dbUser and dbPassword options are will be removed in a future
           release.

The connection options are case insensitive and can only be defined once.

If an option is defined more than once, an error will be generated.

Protocol Selection

The scheme option defines the protocol to be used on the connection, the
following are the accepted values:

- mysql: for connections using the Classic protocol.
- mysqlx: for connections using the X protocol.

Socket Connections

To define a socket connection, replace the host and port by the socket path.

When using a connection dictionary, the path is set as the value for the socket
option.

When using a URI, the socket path must be URL encoded. A socket path may be
specified in a URI in one of the following ways:

?{__os_type=='windows'}
- \.\named.pipe
- (\.\named.pipe)
?{}
?{__os_type!='windows'}
- /path%2Fto%2Fsocket.sock
- (/path/to/socket.sock)
- ./path%2Fto%2Fsocket.sock
- (./path/to/socket.sock)
- ../path%2Fto%2Fsocket.sock
- (../path/to/socket.sock)
?{}

SSL Mode

The ssl-mode option accepts the following values:

- DISABLED
- PREFERRED
- REQUIRED
- VERIFY_CA
- VERIFY_IDENTITY

TLS Version

The tls-version option accepts the following values:

- TLSv1
- TLSv1.1
- TLSv1.2 (Supported only on commercial packages)

URL Encoding

URL encoded values only accept alphanumeric characters and the next symbols:
-._~!$'()*+;

Any other character must be URL encoded.

URL encoding is done by replacing the character being encoded by the sequence:
%XX

Where XX is the hexadecimal ASCII value of the character being encoded.

If host is a literal IPv6 address it should be enclosed in "[" and "]"
characters.

If host is a literal IPv6 address with zone ID, the '%' character separating
address from the zone ID needs to be URL encoded.

#@<OUT> BUG28393119 UNABLE TO GET HELP ON CONNECTION DATA, after session
The connection data may be specified in the following formats:

- A URI string
- A dictionary with the connection options

A basic URI string has the following format:

[scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

SSL Connection Options

The following options are valid for use either in a URI or in a dictionary:

- ssl-mode: the SSL mode to be used in the connection.
- ssl-ca: the path to the X509 certificate authority in PEM format.
- ssl-capath: the path to the directory that contains the X509 certificates
  authorities in PEM format.
- ssl-cert: The path to the X509 certificate in PEM format.
- ssl-key: The path to the X509 key in PEM format.
- ssl-crl: The path to file that contains certificate revocation lists.
- ssl-crlpath: The path of directory that contains certificate revocation list
  files.
- ssl-cipher: SSL Cipher to use.
- tls-version: List of protocols permitted for secure connections
- auth-method: Authentication method
- get-server-public-key: Request public key from the server required for RSA
  key pair-based password exchange. Use when connecting to MySQL 8.0 servers
  with classic MySQL sessions with SSL mode DISABLED.
- server-public-key-path: The path name to a file containing a client-side copy
  of the public key required by the server for RSA key pair-based password
  exchange. Use when connecting to MySQL 8.0 servers with classic MySQL
  sessions with SSL mode DISABLED.
- connect-timeout: The connection timeout in milliseconds. If not provided a
  default timeout of 10 seconds will be used. Specifying a value of 0 disables
  the connection timeout.
- compression: Enable/disable compression in client/server protocol, valid
  values: "true", "false", "1", and "0".

When these options are defined in a URI, their values must be URL encoded.

The following options are also valid when a dictionary is used:

Base Connection Options

- scheme: the protocol to be used on the connection.
- user: the MySQL user name to be used on the connection.
- dbUser: alias for user.
- password: the password to be used on the connection.
- dbPassword: same as password.
- host: the hostname or IP address to be used on a TCP connection.
- port: the port to be used in a TCP connection.
- socket: the socket file name to be used on a connection through unix sockets.
- schema: the schema to be selected once the connection is done.

ATTENTION: The dbUser and dbPassword options are will be removed in a future
           release.

The connection options are case insensitive and can only be defined once.

If an option is defined more than once, an error will be generated.

Protocol Selection

The scheme option defines the protocol to be used on the connection, the
following are the accepted values:

- mysql: for connections using the Classic protocol.
- mysqlx: for connections using the X protocol.

Socket Connections

To define a socket connection, replace the host and port by the socket path.

When using a connection dictionary, the path is set as the value for the socket
option.

When using a URI, the socket path must be URL encoded. A socket path may be
specified in a URI in one of the following ways:

?{__os_type=='windows'}
- \.\named.pipe
- (\.\named.pipe)
?{}
?{__os_type!='windows'}
- /path%2Fto%2Fsocket.sock
- (/path/to/socket.sock)
- ./path%2Fto%2Fsocket.sock
- (./path/to/socket.sock)
- ../path%2Fto%2Fsocket.sock
- (../path/to/socket.sock)
?{}

SSL Mode

The ssl-mode option accepts the following values:

- DISABLED
- PREFERRED
- REQUIRED
- VERIFY_CA
- VERIFY_IDENTITY

TLS Version

The tls-version option accepts the following values:

- TLSv1
- TLSv1.1
- TLSv1.2 (Supported only on commercial packages)

URL Encoding

URL encoded values only accept alphanumeric characters and the next symbols:
-._~!$'()*+;

Any other character must be URL encoded.

URL encoding is done by replacing the character being encoded by the sequence:
%XX

Where XX is the hexadecimal ASCII value of the character being encoded.

If host is a literal IPv6 address it should be enclosed in "[" and "]"
characters.

If host is a literal IPv6 address with zone ID, the '%' character separating
address from the zone ID needs to be URL encoded.

SEE ALSO

Additional entries were found matching connection

The following topics were found at the SQL Syntax category:

- ALTER TABLE
- CREATE TABLE
- EXPLAIN
- KILL

For help on a specific topic use: \? <topic>

e.g.: \? ALTER TABLE

