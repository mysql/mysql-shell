//@<OUT> Shell Help
NAME
      shell - Gives access to general purpose functions and properties.

DESCRIPTION
      Gives access to general purpose functions and properties.

PROPERTIES
      options
            Gives access to options impacting shell behavior.

      reports
            Gives access to built-in and user-defined reports.

FUNCTIONS
      addExtensionObjectMember(object, name, member[, definition])
            Adds a member to an extension object.

      connect(connectionData[, password])
            Establishes the shell global session.

      createExtensionObject()
            Creates an extension object, it can be used to extend shell
            functionality.

      deleteAllCredentials()
            Deletes all credentials managed by the configured helper.

      deleteCredential(url)
            Deletes credential for the given URL using the configured helper.

      disablePager()
            Disables pager for the current scripting mode.

      dumpRows(result, format)
            Formats and dumps the given resultset object to the console.

      enablePager()
            Enables pager specified in shell.options.pager for the current
            scripting mode.

      getSession()
            Returns the global session.

      help([member])
            Provides help about this object and it's members

      listCredentialHelpers()
            Returns a list of strings, where each string is a name of a helper
            available on the current platform.

      listCredentials()
            Retrieves a list of all URLs stored by the configured helper.

      log(level, message)
            Logs an entry to the shell's log file.

      parseUri(uri)
            Utility function to parse a URI string.

      prompt(message[, options])
            Utility function to prompt data from the user.

      reconnect()
            Reconnect the global session.

      registerGlobal()
            Registers an extension object as a shell global object.

      registerReport(name, type, report[, description])
            Registers a new user-defined report.

      setCurrentSchema(name)
            Sets the active schema on the global session.

      setSession(session)
            Sets the global session.

      status()
            Shows connection status info for the shell.

      storeCredential(url[, password])
            Stores given credential using the configured helper.

      unparseUri(options)
            Formats the given connection options to a URI string suitable for
            mysqlsh.

//@<OUT> Help on Options
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
        included when printing a SQL result
      - useWizards: read-only, boolean value to indicate if interactive
        prompting and wizards are enabled by default in AdminAPI and others.
        Use --no-wizard to disable.
      - verbose: 0..4, verbose output level. If >0, additional output that may
        help diagnose issues is printed to the screen. Larger values mean more
        verbose. Default is 0.

      The resultFormat option supports the following values to modify the
      format of printed query results:

      - table: tabular format with a ascii character frame (default)
      - tabbed: tabular format with no frame, columns separated by tabs
      - vertical: displays the outputs vertically, one line per column value
      - json: same as json/pretty
      - ndjson: newline delimited JSON, same as json/raw
      - json/array: one JSON document per line, inside an array
      - json/pretty: pretty printed JSON
      - json/raw: one JSON document per line

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      set(optionName, value)
            Sets value of an option.

      setPersist(optionName, value)
            Sets value of an option and stores it in the configuration file.

      unset(optionName)
            Resets value of an option to default.

      unsetPersist(optionName)
            Resets value of an option to default and removes it from the
            configuration file.

//@<OUT> Help on Reports
NAME
      reports - Gives access to built-in and user-defined reports.

SYNTAX
      shell.reports

DESCRIPTION
      The 'reports' object provides access to built-in reports.

      All user-defined reports registered using the shell.registerReport()
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

//@<OUT> Help on addExtensionObjectMember
NAME
      addExtensionObjectMember - Adds a member to an extension object.

SYNTAX
      shell.addExtensionObjectMember(object, name, member[, definition])

WHERE
      object: The object to which the member will be added.
      name: The name of the member being added.
      member: The member being added.
      definition: Dictionary with help information about the member.

DESCRIPTION
      The name parameter must be a valid identifier, this is, it should follow
      the pattern [_|a..z|A..Z]([_|a..z|A..Z|0..9])*

      The member parameter can be any of:

      - An extension object
      - A function
      - Scalar values: boolean, integer, float, string
      - Array
      - Dictionary
      - None/Null

      When an extension object is added as a member, a read only property will
      be added into the target extension object.

      When a function is added as member, it will be callable on the extension
      object but it will not be possible to overwrite it with a new value or
      function.

      When any of the other types are added as a member, a read/write property
      will be added into the target extension object, it will be possible to
      update the value without any restriction.

      IMPORTANT: every member added into an extensible object will be available
      only when the object is registered, this is, registered as a global shell
      object, or added as a member of another object that is already
      registered.

      The definition parameter is an optional and can be used to define
      additional information for the member.

      The member definition accepts the following attributes:

      - brief: optional string containing a brief description of the member
        being added.
      - details: optional array of strings containing detailed information
        about the member being added.

      This information will be integrated into the shell help to be available
      through the built in help system (\?).

      When adding a function, the following attribute is also allowed on the
      member definition:

      - parameters: a list of parameter definitions for each parameter that the
        function accepts.

      A parameter definition is a dictionary with the following attributes:

      - name: required, the name of the parameter, must be a valid identifier.
      - type: required, the data type of the parameter, allowed values include:
        string, integer, bool, float, array, dictionary, object.
      - required: a boolean value indicating whether the parameter is mandatory
        or optional.
      - brief: a string with a short description of the parameter.
      - details: a string array with additional details about the parameter.

      The information defined on the brief and details attributes will also be
      added to the function help on the built in help system (\?).

      If a parameter's type is 'string' the following attribute is also allowed
      on the parameter definition dictionary:

      - values: string array with the only values that are allowed for the
        parameter.

      If a parameter's type is 'object' the following attributes are also
      allowed on the parameter definition dictionary:

      - class: string defining the class of object allowed as parameter values.
      - classes: string array defining multiple classes of objects allowed as
        parameter values.

      The values for the class(es) properties must be a valid class exposed
      through the different APIs. For details use:

      - \? mysql
      - \? mysqlx
      - \? adminapi
      - \? shellapi

      To get the class name for a global object or a registered extension call
      the print function passing as parameter the object, i.e. "Shell" is the
      class name for the built in shell global object:

      mysql-js> print(shell)
      <Shell>
      mysql-js>

      If a parameter's type is 'dictionary' the following attribute is also
      allowed on the parameter definition dictionary:

      - options: list of option definition dictionaries defining the allowed
        options that can be passed on the dictionary parameter.

      An option definition dictionary follows exactly the same rules as the
      parameter definition dictionary except for one: on a parameter definition
      dictionary, required parameters must be defined first, option definition
      dictionaries do not have this restriction.

//@<OUT> Help on Connect
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

//@<OUT> Help on createExtensionObject
NAME
      createExtensionObject - Creates an extension object, it can be used to
                              extend shell functionality.

SYNTAX
      shell.createExtensionObject()

DESCRIPTION
      An extension object is defined by adding members in it (properties and
      functions).

      An extension object can be either added as a property of another
      extension object or registered as a shell global object.

      An extension object is usable only when it has been registered as a
      global object or when it has been added into another extension object
      that is member in an other registered object.

//@<OUT> Help on deleteAllCredentials
NAME
      deleteAllCredentials - Deletes all credentials managed by the configured
                             helper.

SYNTAX
      shell.deleteAllCredentials()

EXCEPTIONS
      Throws RuntimeError in the following scenarios:

      - if configured credential helper is invalid.
      - if deleting the credentials fails.

//@<OUT> Help on deleteCredential
NAME
      deleteCredential - Deletes credential for the given URL using the
                         configured helper.

SYNTAX
      shell.deleteCredential(url)

WHERE
      url: URL of the server to delete.

DESCRIPTION
      URL needs to be in the following form: user@(host[:port]|socket).

EXCEPTIONS
      Throws ArgumentError if URL has invalid form.

      Throws RuntimeError in the following scenarios:

      - if configured credential helper is invalid.
      - if deleting the credential fails.

//@<OUT> Help on disablePager
NAME
      disablePager - Disables pager for the current scripting mode.

SYNTAX
      shell.disablePager()

DESCRIPTION
      The current value of shell.options.pager option is not changed by calling
      this method.

      This method has no effect in non-interactive mode.

//@<OUT> Help on enablePager
NAME
      enablePager - Enables pager specified in shell.options.pager for the
                    current scripting mode.

SYNTAX
      shell.enablePager()

DESCRIPTION
      All subsequent text output (except for prompts and user interaction) is
      going to be forwarded to the pager.

      This behavior is in effect until disablePager() is called or current
      scripting mode is changed.

      Changing the scripting mode has the same effect as calling
      disablePager().

      If the value of shell.options.pager option is changed after this method
      has been called, the new pager will be automatically used.

      If shell.options.pager option is set to an empty string when this method
      is called, pager will not be active until shell.options.pager is set to a
      non-empty string.

      This method has no effect in non-interactive mode.

//@<OUT> Help on getSession
NAME
      getSession - Returns the global session.

SYNTAX
      shell.getSession()

DESCRIPTION
      The returned object will be either a ClassicSession or a Session object,
      depending on how the global session was created.

//@<OUT> Help on Help
NAME
      help - Provides help about this object and it's members

SYNTAX
      shell.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on listCredentialHelpers
NAME
      listCredentialHelpers - Returns a list of strings, where each string is a
                              name of a helper available on the current
                              platform.

SYNTAX
      shell.listCredentialHelpers()

RETURNS
       A list of string with names of available credential helpers.

DESCRIPTION
      The special values "default" and "<disabled>" are not on the list.

      Only values on this list (plus "default" and "<disabled>") can be used to
      set the "credentialStore.helper" option.

//@<OUT> Help on listCredentials
NAME
      listCredentials - Retrieves a list of all URLs stored by the configured
                        helper.

SYNTAX
      shell.listCredentials()

RETURNS
       A list of URLs stored by the configured credential helper.

EXCEPTIONS
      Throws RuntimeError in the following scenarios:

      - if configured credential helper is invalid.
      - if listing the URLs fails.

//@<OUT> Help on Log
NAME
      log - Logs an entry to the shell's log file.

SYNTAX
      shell.log(level, message)

WHERE
      level: one of ERROR, WARNING, INFO, DEBUG, DEBUG2, DEBUG3 as a string
      message: the text to be logged

DESCRIPTION
      Only messages that have a level value equal to or lower than the active
      one (set via --log-level) are logged.

//@<OUT> Help on parseUri
NAME
      parseUri - Utility function to parse a URI string.

SYNTAX
      shell.parseUri(uri)

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

//@<OUT> Help on Prompt
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

//@<OUT> Help on reconnect
NAME
      reconnect - Reconnect the global session.

SYNTAX
      shell.reconnect()

//@<OUT> Help on registerGlobal
NAME
      registerGlobal - Registers an extension object as a shell global object.

SYNTAX
      shell.registerGlobal()

DESCRIPTION
      As a shell global object everything in the object will be available from
      both JavaScript and Python despite if the member implementation was done
      in one language or the other.

      When the object is registered as a global, all the help data associated
      to the object and it's members is made available through the shell built
      in help system (\?).

//@<OUT> Help on registerReport
NAME
      registerReport - Registers a new user-defined report.

SYNTAX
      shell.registerReport(name, type, report[, description])

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

      All reports registered in those files using the registerReport() method
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

//@<OUT> Help on setCurrentSchema
NAME
      setCurrentSchema - Sets the active schema on the global session.

SYNTAX
      shell.setCurrentSchema(name)

WHERE
      name: The name of the schema to be set as active.

DESCRIPTION
      Once the schema is set as active, it will be available through the db
      global object.

//@<OUT> Help on setSession
NAME
      setSession - Sets the global session.

SYNTAX
      shell.setSession(session)

WHERE
      session: The session object to be used as global session.

DESCRIPTION
      Sets the global session using a session from a variable.

//@<OUT> Help on status
NAME
      status - Shows connection status info for the shell.

SYNTAX
      shell.status()

DESCRIPTION
      This shows the same information shown by the \status command.

//@<OUT> Help on storeCredential
NAME
      storeCredential - Stores given credential using the configured helper.

SYNTAX
      shell.storeCredential(url[, password])

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

//@<OUT> BUG28393119 UNABLE TO GET HELP ON CONNECTION DATA, before session
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

//@<OUT> BUG28393119 UNABLE TO GET HELP ON CONNECTION DATA, after session
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

//@<OUT> Help on plugins
The MySQL Shell allows extending its base functionality through the creation of
plugins.

An plugin is a folder containing the code that provides the functionality to be
made available on the MySQL Shell.

User defined plugins should be located at plugins folder at the following
paths:

- Windows: %AppData%\MySQL\mysqlsh\plugins
- Others: ~/.mysqlsh/plugins

A plugin must contain an init file which is the entry point to load the
extension:

- init.js for plugins written in JavaScript.
- init.py for plugins written in Python.

On startup, the shell traverses the folders inside of the *plugins* folder
searching for the plugin init file. The init file will be loaded on the
corresponding context (JavaScript or Python).

Use Cases

The main use cases for MySQL Shell plugins include:

- Definition of shell reports to be used with the \show and \watch Shell
  Commands.
- Definition of new Global Objects with user defined functionality.

For additional information on shell reports execute: \? reports

For additional information on extension objects execute: \? extension objects

//@<OUT> Help on extension objects
The MySQL Shell allows for an extension of its base functionality by using
special objects called Extension Objects.

Once an extension object has been created it is possible to attach different
type of object members such as:

- Functions
- Properties
- Other extension objects

Once an extension object has been fully defined it can be registered as a
regular Global Object. This way the object and its members will be available in
the scripting languages the MySQL Shell supports, just as any other global
object such as shell, util, etc.

Extending the MySQL Shell with extension objects follows this pattern:

- Creation of a new extension object.
- Addition of members (properties, functions)
- Registration of the object

Creation of a new extension object is done through the
shell.createExtensionObject function.

Members can be attached to the extension object by calling the
shell.addExtensionObjectMember function.

Registration of an extension object as a global object can be done by calling
the shell.registerGlobal function.

Alternatively, the extension object can be added as a member of another
extension object by calling the shell.addExtensionObjectMember function.

You can get more details on these functions by executing:

- \? createExtensionObject
- \? addExtensionObjectMember
- \? registerGlobal

Naming Convention

The core MySQL Shell APIs follow a specific naming convention for object
members. Extension objects should follow the same naming convention for
consistency reasons.

- JavaScript members use camelCaseNaming
- Python members use snake_case_naming

To simplify this across languages, it is important to use camelCaseNaming when
specifying the 'name' parameter of the shell.addExtensionObjectMember function.

The MySQL Shell will then automatically handle the snake_case_naming for that
member when it is switched to Python mode.

NOTE: the naming convention is only applicable for extension object members.
However, when a global object is registered, the name used to register that
object will be exactly the same in both JavaScript and Python modes.

Example:

# Sample python function to be added to an extension object
def some_python_function():
  print ("Hello world!")

# The extension object is created
obj = shell.create_extension_object()

# The sample function is added as member
# NOTE: The member name using camelCaseNaming
shell.add_extension_object_member(obj, "mySampleFunction",
some_python_function)

# The extension object is registered
shell.register_global("myCustomObject", obj)

Calling in JavaScript:

// Member is available using camelCaseNaming
mysql-js> myCustomObject.mySampleFunction()
Hello World!
mysql-js>

Calling in Python:

# Member is available using snake_case_naming
mysql-py> myCustomObject.my_sample_function()
Hello World!
mysql-py>

Automatic Loading of Extension Objects

The MySQL Shell startup logic scans for extension scripts at the following
paths:

- Windows: %AppData%/MySQL/mysqlsh/init.d
- Others ~/.mysqlsh/init.d

An extension script is either a JavaScript (*.js) or Python (*.py) file which
will be automatically processed when the MySQL Shell starts.

These scripts can be used to define extension objects so ther are available
right away when the MySQL Shell starts.

//@<OUT> Help on unparseUri
NAME
      unparseUri - Formats the given connection options to a URI string
                   suitable for mysqlsh.

SYNTAX
      shell.unparseUri(options)

WHERE
      options: a dictionary with the connection options.

RETURNS
       A URI string

DESCRIPTION
      This function assembles a MySQL connection string which can be used in
      the shell or X DevAPI connectors.

//@<OUT> Help on dumpRows
NAME
      dumpRows - Formats and dumps the given resultset object to the console.

SYNTAX
      shell.dumpRows(result, format)

WHERE
      result: The resultset object to dump
      format: One of table, tabbed, vertical, json, ndjson, json/raw,
              json/array, json/pretty. Default is table.

RETURNS
       Nothing

DESCRIPTION
      This function shows a resultset object returned by a DB Session query in
      the same formats supported by the shell.

      Note that the resultset will be consumed by the function.

