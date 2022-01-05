#@<OUT> shell
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
      add_extension_object_member(object, name, member[, definition])
            Adds a member to an extension object.

      connect(connectionData[, password])
            Establishes the shell global session.

      connect_to_primary([connectionData][, password])
            Establishes the shell global session, connecting to a primary of an
            InnoDB cluster or ReplicaSet.

      create_context(options)
            Create a shell context wrapper used in multiuser environments. The
            returned object should be explicitly deleted when no longer needed.

      create_extension_object()
            Creates an extension object, it can be used to extend shell
            functionality.

      delete_all_credentials()
            Deletes all credentials managed by the configured helper.

      delete_credential(url)
            Deletes credential for the given URL using the configured helper.

      disable_pager()
            Disables pager for the current scripting mode.

      disconnect()
            Disconnects the global session.

      dump_rows(result, format)
            Formats and dumps the given resultset object to the console.

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

      list_ssh_connections()
            Retrieves a list of all active SSH tunnels.

      log(level, message)
            Logs an entry to the shell's log file.

      open_session(connectionData[, password])
            Establishes and returns session.

      parse_uri(uri)
            Utility function to parse a URI string.

      prompt(message[, options])
            Utility function to prompt data from the user.

      reconnect()
            Reconnect the global session.

      register_global(name, object[, definition])
            Registers an extension object as a shell global object.

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

      unparse_uri(options)
            Formats the given connection options to a URI string suitable for
            mysqlsh.

CLASSES
 - ShellContextWrapper Holds shell context which is used by a thread, gives
                       access to the shell context object through get_shell().

#@<OUT> shell.add_extension_object_member
NAME
      add_extension_object_member - Adds a member to an extension object.

SYNTAX
      shell.add_extension_object_member(object, name, member[, definition])

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

      The password may be included on the connectionData, the optional
      parameter should be used only if the connectionData does not contain it
      already. If both are specified the password parameter will override the
      password defined on the connectionData.

      The connection data may be specified in the following formats:

      - A URI string
      - A dictionary with the connection options

      A basic URI string has the following format:

      [scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

      Connection Options

      The following options are valid for use either in a URI or in a
      dictionary:

      - ssl-mode: The SSL mode to be used in the connection.
      - ssl-ca: The path to the X509 certificate authority file in PEM format.
      - ssl-capath: The path to the directory that contains the X509
        certificate authority files in PEM format.
      - ssl-cert: The path to the SSL public key certificate file in PEM
        format.
      - ssl-key: The path to the SSL private key file in PEM format.
      - ssl-crl: The path to file that contains certificate revocation lists.
      - ssl-crlpath: The path of directory that contains certificate revocation
        list files.
      - ssl-cipher: The list of permissible encryption ciphers for connections
        that use TLS protocols up through TLSv1.2.
      - tls-version: List of protocols permitted for secure connections.
      - tls-ciphers: List of TLS v1.3 ciphers to use.
      - auth-method: Authentication method.
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
      - compression: Enable compression in client/server protocol.
      - compression-algorithms: Use compression algorithm in server/client
        protocol.
      - compression-level: Use this compression level in the client/server
        protocol.
      - connection-attributes: List of connection attributes to be registered
        at the PERFORMANCE_SCHEMA connection attributes tables.
      - local-infile: Enable/disable LOAD DATA LOCAL INFILE.
      - net-buffer-length: The buffer size for TCP/IP and socket communication.

      When these options are defined in a URI, their values must be URL
      encoded.

      The following options are also valid when a dictionary is used:

      Base Connection Options

      - uri: a URI string.
      - scheme: the protocol to be used on the connection.
      - user: the MySQL user name to be used on the connection.
      - dbUser: alias for user.
      - password: the password to be used on the connection.
      - dbPassword: same as password.
      - host: the hostname or IP address to be used on the connection.
      - port: the port to be used in a TCP connection.
      - socket: the socket file name to be used on a connection through unix
        sockets.
      - schema: the schema to be selected once the connection is done.

      SSH Tunnel Connection Options

      - ssh: a SSHURI string used when SSH tunnel is required.
      - ssh-password: the password the be used on the SSH connection.
      - ssh-identity-file: the key file to be used on the SSH connection.
      - ssh-identity-file-password: the SSH key file password.
      - ssh-config-file: the SSH configuration file, default is the value of
        shell.options['ssh.configFile']

      ATTENTION: The dbUser and dbPassword options are will be removed in a
                 future release.

      ATTENTION: The connection options have precedence over options specified
                 in the connection options uri

      The connection options are case insensitive and can only be defined once.

      If an option is defined more than once, an error will be generated.

      For additional information on connection data use \? connection.

#@<OUT> Help on connect_to_primary
NAME
      connect_to_primary - Establishes the shell global session, connecting to
                           a primary of an InnoDB cluster or ReplicaSet.

SYNTAX
      shell.connect_to_primary([connectionData][, password])

WHERE
      connectionData: The connection data to be used to establish the session.
      password: The password to be used when establishing the session.

RETURNS
      The established session.

DESCRIPTION
      Ensures that the target server is a member of an InnoDB cluster or
      ReplicaSet and if it is not a PRIMARY, finds the PRIMARY and connects to
      it. Sets the global session object to the established session and returns
      that object.

      If connectionData is not given, this function uses the global shell
      session, if there is none, an exception is raised.

      The password may be included on the connectionData, the optional
      parameter should be used only if the connectionData does not contain it
      already. If both are specified the password parameter will override the
      password defined on the connectionData.

      The connection data may be specified in the following formats:

      - A URI string
      - A dictionary with the connection options

      A basic URI string has the following format:

      [scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

      Connection Options

      The following options are valid for use either in a URI or in a
      dictionary:

      - ssl-mode: The SSL mode to be used in the connection.
      - ssl-ca: The path to the X509 certificate authority file in PEM format.
      - ssl-capath: The path to the directory that contains the X509
        certificate authority files in PEM format.
      - ssl-cert: The path to the SSL public key certificate file in PEM
        format.
      - ssl-key: The path to the SSL private key file in PEM format.
      - ssl-crl: The path to file that contains certificate revocation lists.
      - ssl-crlpath: The path of directory that contains certificate revocation
        list files.
      - ssl-cipher: The list of permissible encryption ciphers for connections
        that use TLS protocols up through TLSv1.2.
      - tls-version: List of protocols permitted for secure connections.
      - tls-ciphers: List of TLS v1.3 ciphers to use.
      - auth-method: Authentication method.
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
      - compression: Enable compression in client/server protocol.
      - compression-algorithms: Use compression algorithm in server/client
        protocol.
      - compression-level: Use this compression level in the client/server
        protocol.
      - connection-attributes: List of connection attributes to be registered
        at the PERFORMANCE_SCHEMA connection attributes tables.
      - local-infile: Enable/disable LOAD DATA LOCAL INFILE.
      - net-buffer-length: The buffer size for TCP/IP and socket communication.

      When these options are defined in a URI, their values must be URL
      encoded.

      The following options are also valid when a dictionary is used:

      Base Connection Options

      - uri: a URI string.
      - scheme: the protocol to be used on the connection.
      - user: the MySQL user name to be used on the connection.
      - dbUser: alias for user.
      - password: the password to be used on the connection.
      - dbPassword: same as password.
      - host: the hostname or IP address to be used on the connection.
      - port: the port to be used in a TCP connection.
      - socket: the socket file name to be used on a connection through unix
        sockets.
      - schema: the schema to be selected once the connection is done.

      SSH Tunnel Connection Options

      - ssh: a SSHURI string used when SSH tunnel is required.
      - ssh-password: the password the be used on the SSH connection.
      - ssh-identity-file: the key file to be used on the SSH connection.
      - ssh-identity-file-password: the SSH key file password.
      - ssh-config-file: the SSH configuration file, default is the value of
        shell.options['ssh.configFile']

      ATTENTION: The dbUser and dbPassword options are will be removed in a
                 future release.

      ATTENTION: The connection options have precedence over options specified
                 in the connection options uri

      The connection options are case insensitive and can only be defined once.

      If an option is defined more than once, an error will be generated.

      For additional information on connection data use \? connection.

EXCEPTIONS
      RuntimeError in the following scenarios:

      - If connectionData was not given and there is no global shell session.
      - If there is no primary member of an InnoDB cluster or ReplicaSet.
      - If the target server is not a member of an InnoDB cluster or
        ReplicaSet.

#@<OUT> shell.create_extension_object
NAME
      create_extension_object - Creates an extension object, it can be used to
                                extend shell functionality.

SYNTAX
      shell.create_extension_object()

DESCRIPTION
      An extension object is defined by adding members in it (properties and
      functions).

      An extension object can be either added as a property of another
      extension object or registered as a shell global object.

      An extension object is usable only when it has been registered as a
      global object or when it has been added into another extension object
      that is member in an other registered object.

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

#@<OUT> shell.list_ssh_connections
NAME
      list_ssh_connections - Retrieves a list of all active SSH tunnels.

SYNTAX
      shell.list_ssh_connections()

RETURNS
      A list of active SSH tunnel connections.

#@<OUT> shell.log
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
      - connectTimeout: float, default connection timeout used by Shell
        sessions, in seconds
      - credentialStore.excludeFilters: array of URLs for which automatic
        password storage is disabled, supports glob characters '*' and '?'
      - credentialStore.helper: name of the credential helper to use to
        fetch/store passwords; a special value "default" is supported to use
        platform default helper; a special value "<disabled>" is supported to
        disable the credential store
      - credentialStore.savePasswords: controls automatic password storage,
        allowed values: "always", "prompt" or "never"
      - dba.connectTimeout: float, default connection timeout used for sessions
        created in AdminAPI operations, in seconds
      - dba.gtidWaitTimeout: timeout value in seconds to wait for GTIDs to be
        synchronized
      - dba.logSql: 0..2, log SQL statements executed by AdminAPI operations: 0
        - logging disabled; 1 - log statements other than SELECT and SHOW; 2 -
        log all statements.
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
      - history.sql.syslog: true to log filtered interactive commands to the
        system log, filtering of commands depends on the value of
        history.sql.ignorePattern
      - interactive: read-only, boolean value that indicates if the shell is
        running in interactive mode
      - logLevel: current log level
      - mysqlPluginDir: Directory for client-side authentication plugins
      - oci.configFile: Path to OCI (Oracle Cloud Infrastructure) configuration
        file
      - oci.profile: Specify which section in oci.configFile will be used as
        profile settings
      - pager: string which specifies the external command which is going to be
        used to display the paged output
      - passwordsFromStdin: boolean value that indicates if the shell should
        read passwords from stdin instead of the tty
      - resultFormat: controls the type of output produced for SQL results.
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
      - ssh.configFile string path default empty, custom path for SSH
        configuration. If not defined the standard SSH paths will be used
        (~/.ssh/config).
      - ssh.bufferSize integer default 10240 bytes, used for tunnel data
        transfer

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
      A string value containing the result based on the user input.

DESCRIPTION
      This function allows creating scripts that require interaction with the
      user to gather data.

      The options dictionary may contain the following options:

      - title: a string to identify the prompt.
      - description: a string list with a description of the prompt, each entry
        represents a paragraph.
      - type: a string value to define the prompt type. Supported types
        include: text, password, confirm, select, fileOpen, fileSave,
        directory. Default value: text.
      - defaultValue: defines the default value to be used in case the user
        accepts the prompt without providing custom information. The value of
        this option depends on the prompt type.
      - yes: string value to overwrite the default '&Yes' option on 'confirm'
        prompts.
      - no: string value to overwrite the default '&No' option on 'confirm'
        prompts.
      - alt: string value to define an additional option for 'confirm' prompts.
      - options: string list of options to be used in 'select' prompts.

      General Behavior

      The 'title' option is not used in the Shell but it might be useful for
      better integration with external plugins handling prompts.

      When the 'description' option is provided, each entry will be printed as
      a separate paragraph.

      Once the description is printed, the content of the message parameter
      will be printed and input from the user will be required.

      The value of the 'defaultValue' option and the returned value will depend
      on the 'type' option, as explained on the following sections.

      Open Prompts

      These represent prompts without a pre-defined set of valid answers, the
      prompt types on this category include: text, password, fileOpen, fileSave
      and directory.

      The 'defaultValue' on these prompts can be set to a string to be returned
      by the function if the user replies to the prompt with an empty string,
      if not defined then the prompt will accept and return the empty string.

      The returned value on these prompts will be either the value set on
      'defaultValue' option or the value entered by the user.

      In the case of password type prompts, the data entered by the user will
      not be displayed on the screen.

      Confirm Prompts

      The default behaviour of these prompts if to allow the user answering
      Yes/No questions, however, the default '&Yes' and '&No' options can be
      overriden through the 'yes' and 'no' options.

      An additional option can be added to the prompt by defining the 'alt'
      option.

      The 'yes', 'no' and 'alt' options are used to define the valid answers
      for the prompt. The ampersand (&) symbol can be used to define a single
      character which acts as a shortcut for the user to select the indicated
      answer.

      i.e. using an option like: '&Yes' causes the following to be valid
      answers (case insensitive): 'Yes', '&Yes', 'Y'.

      All the valid answers must be unique within the prompt call.

      If the 'defaultValue' option is defined, it must be set equal to any of
      the valid prompt answers, i.e. if the 'yes', 'no' and 'alt' options are
      not defined, then 'defaultValue' can be set to one of '&Yes', 'Yes', 'Y',
      '&No', 'No' or 'N', case insensitive, otherwise, it must be set to one of
      the valid answers based on the values defined in the 'yes', 'no' or 'alt'
      options.

      This prompt will be shown repeatedly until the user explicitly replies
      with one of the valid answers unless a 'defaultValue' is provided, which
      will be used in case the user replies with an empty answer.

      The returned value will be the label associated to the user provided
      answer.

      i.e. if the prompt is using the default options ('&Yes' and '&No') and
      the user response is 'n' the prompt function will return '&No'.

      Select Prompts

      These prompts allow the user selecting an option from a pre-defined list
      of options.

      To define the list of options to be used in the prompt the 'options'
      option should be used.

      If the 'defaultValue' option is defined, it must be a number representing
      the 1 based index of the option to be selected by default.

      This prompt will be shown repeatedly until the user explicitly replies
      with the 1 based index of the option to be selected unless a default
      option is pre-defined through the 'defaultValue' option.

      The returned value will be the text of the selected option.

#@<OUT> shell.disconnect
NAME
      disconnect - Disconnects the global session.

SYNTAX
      shell.disconnect()

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

      threads(session[, argv][, options])
            Lists threads that belong to the user who owns the current session.

      thread(session[, argv][, options])
            Provides various information regarding the specified thread.

#@<OUT> shell.register_global
NAME
      register_global - Registers an extension object as a shell global object.

SYNTAX
      shell.register_global(name, object[, definition])

WHERE
      name: The name to be given to the registered global object.
      object: The extension object to be registered as a global object.
      definition: Dictionary containing help information about the object.

DESCRIPTION
      An extension object is created with the shell.create_extension_object
      function. Once registered, the functionality it provides will be
      available for use.

      The name parameter must comply with the following restrictions:

      - It must be a valid scripting identifier.
      - It can not be the name of a built in global object.
      - It can not be the name of a previously registered object.

      The name used to register an object will be exactly the name under which
      the object will be available for both Python and JavaScript modes, this
      is, no naming convention is enforced on global objects.

      The definition parameter is a dictionary with help information about the
      object being registered, it allows the following properties:

      - brief: a string with a brief description of the object.
      - details: a list of strings with additional information about the
        object.

      When the object is registered, the help data on the definition parameter
      as well as the object members is made available through the shell built
      in help system. (\?).

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
      - examples - A list of dictionaries describing the example usage of the
        report.

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
      - empty (Boolean, optional) - whether this option accepts empty strings.
        Only 'string' options may have this key. If this key is not specified,
        defaults to false.

      The optional examples list must hold dictionaries with the following
      keys:

      - description (string, required) - Description text of the example.
      - args (list of strings, optional) - List of the arguments used in the
        example.
      - options (dictionary of strings, optional) - Options used in the
        example.

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

#@<OUT> shell.create_context
NAME
      create_context - Create a shell context wrapper used in multiuser
                       environments. The returned object should be explicitly
                       deleted when no longer needed.

SYNTAX
      shell.create_context(options)

WHERE
      options: Dictionary that holds optional callbacks and additional options.

RETURNS
      ShellContextWrapper Wrapper around context isolating the shell scope.

DESCRIPTION
      This function is meant to be used only inside of the Python in new
      thread. It's creating a scope which will isolate logger, interrupt
      handlers and delegates. All the delegates can run into infinite loop if
      one would try to print to stdout directly from the callback functions.

      - printDelegate: function (default not set) which will be called when
        shell function will need to print something to stdout, expected
        signature: None printDelegate(str).
      - errorDelegate: function (default not set) which will be called when
        shell function will need to print something to stderr, expected
        signature: None errorDelegate(str).
      - diagDelegate: function (default not set) which will be called when
        shell function will need to print something diagnostics, expected
        signature: None diagDelegate(str).
      - promptDelegate: function (default not set) which will be called when
        shell function will need to ask user for input, this should return a
        tuple where first arguments is boolean indicating if user did input
        something, and second is a user input string, expected signature:
        tuple<bool,str> promptDelegate(str).
      - passwordDelegate: function (default not set) which will be called when
        shell function will need to ask user for input, this should return a
        tuple where first arguments is boolean indicating if user did input
        anything, and second is a user input string, expected signature:
        tuple<bool,str> passwordDelegate(str).
      - logFile string which spcifies the location for the log file for new
        context. If this is not specified, logs from all threads will be stored
        in the mysqlsh.log file. If the file cannot be created, and exception
        will be thrown.

#@<OUT> BUG28393119 UNABLE TO GET HELP ON CONNECTION DATA, before session
The connection data may be specified in the following formats:

- A URI string
- A dictionary with the connection options

A basic URI string has the following format:

[scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

Connection Options

The following options are valid for use either in a URI or in a dictionary:

- ssl-mode: The SSL mode to be used in the connection.
- ssl-ca: The path to the X509 certificate authority file in PEM format.
- ssl-capath: The path to the directory that contains the X509 certificate
  authority files in PEM format.
- ssl-cert: The path to the SSL public key certificate file in PEM format.
- ssl-key: The path to the SSL private key file in PEM format.
- ssl-crl: The path to file that contains certificate revocation lists.
- ssl-crlpath: The path of directory that contains certificate revocation list
  files.
- ssl-cipher: The list of permissible encryption ciphers for connections that
  use TLS protocols up through TLSv1.2.
- tls-version: List of protocols permitted for secure connections.
- tls-ciphers: List of TLS v1.3 ciphers to use.
- auth-method: Authentication method.
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
- compression: Enable compression in client/server protocol.
- compression-algorithms: Use compression algorithm in server/client protocol.
- compression-level: Use this compression level in the client/server protocol.
- connection-attributes: List of connection attributes to be registered at the
  PERFORMANCE_SCHEMA connection attributes tables.
- local-infile: Enable/disable LOAD DATA LOCAL INFILE.
- net-buffer-length: The buffer size for TCP/IP and socket communication.

When these options are defined in a URI, their values must be URL encoded.

The following options are also valid when a dictionary is used:

Base Connection Options

- uri: a URI string.
- scheme: the protocol to be used on the connection.
- user: the MySQL user name to be used on the connection.
- dbUser: alias for user.
- password: the password to be used on the connection.
- dbPassword: same as password.
- host: the hostname or IP address to be used on the connection.
- port: the port to be used in a TCP connection.
- socket: the socket file name to be used on a connection through unix sockets.
- schema: the schema to be selected once the connection is done.

SSH Tunnel Connection Options

- ssh: a SSHURI string used when SSH tunnel is required.
- ssh-password: the password the be used on the SSH connection.
- ssh-identity-file: the key file to be used on the SSH connection.
- ssh-identity-file-password: the SSH key file password.
- ssh-config-file: the SSH configuration file, default is the value of
  shell.options['ssh.configFile']

ATTENTION: The dbUser and dbPassword options are will be removed in a future
           release.

ATTENTION: The connection options have precedence over options specified in the
           connection options uri

The connection options are case insensitive and can only be defined once.

If an option is defined more than once, an error will be generated.

The options specified in the connection data determine the type of connection
to be used.

The scheme option defines the protocol to be used on the connection, the
following are the accepted values:

- mysql: for connections using the MySQL protocol.
- mysqlx: for connections using the X protocol.

If no protocol is specified in the connection data, the shell will first
attempt connecting using the X protocol, if the connection fails it will then
try to connect using the MySQL protocol.

In general, the Shell connects to the server using TCP connections, unless the
connection data contains the options required to create any of the connections
described below.

Unix-domain Socket Connections

To connect to a local MySQL server using a Unix-domain socket, the host must be
set to 'localhost', no port number should be provided and the socket path
should be provided.

When using the MySQL protocol, the socket path might not be provided, in such
case a default path for the socket file will be used.

When using a connection dictionary, the socket path is set as the value for the
socket option.

When using a URI, the socket path must be URL encoded as follows:

- user@/path%2Fto%2Fsocket.sock
- user@./path%2Fto%2Fsocket.sock
- user@../path%2Fto%2Fsocket.sock

It is possible to skip the URL encoding by enclosing the socket path in
parenthesis:

- user@(/path/to/socket.sock)
- user@(./path/to/socket.sock)
- user@(../path/to/socket.sock)

Windows Named Pipe Connections

To connect to MySQL server using a named pipe the host must be set to '.', no
port number should be provided.

If the pipe name is not provided the default pipe name will be used: MySQL.

When using a connection dictionary, the named pipe name is set as the value for
the socket option.

When using a URI, if the named pipe has invalid characters for a URL, they must
be URL encoded. URL encoding can be skipped by enclosing the pipe name in
parenthesis:

- user@\\.\named.pipe
- user@(\\.\named.pipe)

Named pipe connections are only supported on the MySQL protocol.

Windows Shared Memory Connections

Shared memory connections are only allowed if the server has shared memory
connections enabled using the default shared memory base name: MySQL.

To connect to a local MySQL server using shared memory the host should be set
to 'localhost' and no port should be provided.

If the server does not have shared memory connections enabled using the default
base name, the connection will be done using TCP.

Shared memory connections are only supported on the MySQL protocol.

SSL Mode

The ssl-mode option accepts the following values:

- DISABLED
- PREFERRED
- REQUIRED
- VERIFY_CA
- VERIFY_IDENTITY

TLS Version

The tls-version option accepts values in the following format: TLSv<version>,
e.g. TLSv1.2, TLSv1.3.

Authentication method

In case of classic session, this is the name of the authentication plugin to
use, i.e. caching_sha2_password.

In case of X protocol session, it should be one of:

- AUTO,
- FROM_CAPABILITIES,
- FALLBACK,
- MYSQL41,
- PLAIN,
- SHA256_MEMORY.

Connection Compression

Connection compression is governed by following connection options:
"compression", "compression-algorithms", and "compression-level".

"compression" accepts following values:

- REQUIRED: connection will only be made when compression negotiation is
  succesful.
- PREFFERED: (default for X protocol connections) shell will attempt to
  establish connection with compression enabled, but if compression negotiation
  fails, connection will be established without compression.
- DISABLED: (defalut for classic protocol connections) connection will be
  established without compression.

For convenience "compression" also accepts Boolean: 'True', 'False', '1', and
'0' values which map to REQUIRED and DISABLED respectively.

"compression-algorithms" expects comma separated list of algorithms. Supported
algorithms include:

- zstd
- zlib
- lz4 (X protocol only)
- uncompressed - special value, which if it appears in the list, causes
  connection to succeed even if compression negotiation fails.

If "compression" connection option is not defined, its value will be deduced
from "compression-algorithms" value when it is provided.

"compression-level" expects an integer value. Valid range depends on the
compression algorithm and server configuration, but generally following is
expected:

- zstd: 1-22 (default 3)
- zlib: 1-9 (default 3), supported only by X protocol
- lz4: 0-16 (default 2), supported only by X protocol.

Connection Attributes

Connection attributes are key-value pairs to be sent to the server at connect
time. They are stored at the following PERFORMANCE_SCHEMA tables:

- session_account_connect_attrs: attributes for the current session, and other
  sessions associated with the session account.
- session_connect_attrs: attributes for all sessions.

These attributes should be defined when creating a session and are immutable
during the life-time of the session.

To define connection attributes on a URI, the connection-attributes should be
defined as part of the URI as follows:

root@localhost:port/schema?connection-attributes=[att1=value1,att2=val2,...]

Note that the characters used for the attribute name and value must follow the
URI standard, this is, if the character is not allowed it must be percent
encoded.

To define connection attributes when creating a session using a dictionary the
connection-attributes option should be defined, its value can be set in the
following formats:

- Array of "key=value" pairs.
- Dictionary containing the key-value pairs.

Note that the connection-attribute values are expected to be strings, if other
data type is used in the dictionary, the string representation of the used data
will be stored on the database.

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

Connection Options

The following options are valid for use either in a URI or in a dictionary:

- ssl-mode: The SSL mode to be used in the connection.
- ssl-ca: The path to the X509 certificate authority file in PEM format.
- ssl-capath: The path to the directory that contains the X509 certificate
  authority files in PEM format.
- ssl-cert: The path to the SSL public key certificate file in PEM format.
- ssl-key: The path to the SSL private key file in PEM format.
- ssl-crl: The path to file that contains certificate revocation lists.
- ssl-crlpath: The path of directory that contains certificate revocation list
  files.
- ssl-cipher: The list of permissible encryption ciphers for connections that
  use TLS protocols up through TLSv1.2.
- tls-version: List of protocols permitted for secure connections.
- tls-ciphers: List of TLS v1.3 ciphers to use.
- auth-method: Authentication method.
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
- compression: Enable compression in client/server protocol.
- compression-algorithms: Use compression algorithm in server/client protocol.
- compression-level: Use this compression level in the client/server protocol.
- connection-attributes: List of connection attributes to be registered at the
  PERFORMANCE_SCHEMA connection attributes tables.
- local-infile: Enable/disable LOAD DATA LOCAL INFILE.
- net-buffer-length: The buffer size for TCP/IP and socket communication.

When these options are defined in a URI, their values must be URL encoded.

The following options are also valid when a dictionary is used:

Base Connection Options

- uri: a URI string.
- scheme: the protocol to be used on the connection.
- user: the MySQL user name to be used on the connection.
- dbUser: alias for user.
- password: the password to be used on the connection.
- dbPassword: same as password.
- host: the hostname or IP address to be used on the connection.
- port: the port to be used in a TCP connection.
- socket: the socket file name to be used on a connection through unix sockets.
- schema: the schema to be selected once the connection is done.

SSH Tunnel Connection Options

- ssh: a SSHURI string used when SSH tunnel is required.
- ssh-password: the password the be used on the SSH connection.
- ssh-identity-file: the key file to be used on the SSH connection.
- ssh-identity-file-password: the SSH key file password.
- ssh-config-file: the SSH configuration file, default is the value of
  shell.options['ssh.configFile']

ATTENTION: The dbUser and dbPassword options are will be removed in a future
           release.

ATTENTION: The connection options have precedence over options specified in the
           connection options uri

The connection options are case insensitive and can only be defined once.

If an option is defined more than once, an error will be generated.

The options specified in the connection data determine the type of connection
to be used.

The scheme option defines the protocol to be used on the connection, the
following are the accepted values:

- mysql: for connections using the MySQL protocol.
- mysqlx: for connections using the X protocol.

If no protocol is specified in the connection data, the shell will first
attempt connecting using the X protocol, if the connection fails it will then
try to connect using the MySQL protocol.

In general, the Shell connects to the server using TCP connections, unless the
connection data contains the options required to create any of the connections
described below.

Unix-domain Socket Connections

To connect to a local MySQL server using a Unix-domain socket, the host must be
set to 'localhost', no port number should be provided and the socket path
should be provided.

When using the MySQL protocol, the socket path might not be provided, in such
case a default path for the socket file will be used.

When using a connection dictionary, the socket path is set as the value for the
socket option.

When using a URI, the socket path must be URL encoded as follows:

- user@/path%2Fto%2Fsocket.sock
- user@./path%2Fto%2Fsocket.sock
- user@../path%2Fto%2Fsocket.sock

It is possible to skip the URL encoding by enclosing the socket path in
parenthesis:

- user@(/path/to/socket.sock)
- user@(./path/to/socket.sock)
- user@(../path/to/socket.sock)

Windows Named Pipe Connections

To connect to MySQL server using a named pipe the host must be set to '.', no
port number should be provided.

If the pipe name is not provided the default pipe name will be used: MySQL.

When using a connection dictionary, the named pipe name is set as the value for
the socket option.

When using a URI, if the named pipe has invalid characters for a URL, they must
be URL encoded. URL encoding can be skipped by enclosing the pipe name in
parenthesis:

- user@\\.\named.pipe
- user@(\\.\named.pipe)

Named pipe connections are only supported on the MySQL protocol.

Windows Shared Memory Connections

Shared memory connections are only allowed if the server has shared memory
connections enabled using the default shared memory base name: MySQL.

To connect to a local MySQL server using shared memory the host should be set
to 'localhost' and no port should be provided.

If the server does not have shared memory connections enabled using the default
base name, the connection will be done using TCP.

Shared memory connections are only supported on the MySQL protocol.

SSL Mode

The ssl-mode option accepts the following values:

- DISABLED
- PREFERRED
- REQUIRED
- VERIFY_CA
- VERIFY_IDENTITY

TLS Version

The tls-version option accepts values in the following format: TLSv<version>,
e.g. TLSv1.2, TLSv1.3.

Authentication method

In case of classic session, this is the name of the authentication plugin to
use, i.e. caching_sha2_password.

In case of X protocol session, it should be one of:

- AUTO,
- FROM_CAPABILITIES,
- FALLBACK,
- MYSQL41,
- PLAIN,
- SHA256_MEMORY.

Connection Compression

Connection compression is governed by following connection options:
"compression", "compression-algorithms", and "compression-level".

"compression" accepts following values:

- REQUIRED: connection will only be made when compression negotiation is
  succesful.
- PREFFERED: (default for X protocol connections) shell will attempt to
  establish connection with compression enabled, but if compression negotiation
  fails, connection will be established without compression.
- DISABLED: (defalut for classic protocol connections) connection will be
  established without compression.

For convenience "compression" also accepts Boolean: 'True', 'False', '1', and
'0' values which map to REQUIRED and DISABLED respectively.

"compression-algorithms" expects comma separated list of algorithms. Supported
algorithms include:

- zstd
- zlib
- lz4 (X protocol only)
- uncompressed - special value, which if it appears in the list, causes
  connection to succeed even if compression negotiation fails.

If "compression" connection option is not defined, its value will be deduced
from "compression-algorithms" value when it is provided.

"compression-level" expects an integer value. Valid range depends on the
compression algorithm and server configuration, but generally following is
expected:

- zstd: 1-22 (default 3)
- zlib: 1-9 (default 3), supported only by X protocol
- lz4: 0-16 (default 2), supported only by X protocol.

Connection Attributes

Connection attributes are key-value pairs to be sent to the server at connect
time. They are stored at the following PERFORMANCE_SCHEMA tables:

- session_account_connect_attrs: attributes for the current session, and other
  sessions associated with the session account.
- session_connect_attrs: attributes for all sessions.

These attributes should be defined when creating a session and are immutable
during the life-time of the session.

To define connection attributes on a URI, the connection-attributes should be
defined as part of the URI as follows:

root@localhost:port/schema?connection-attributes=[att1=value1,att2=val2,...]

Note that the characters used for the attribute name and value must follow the
URI standard, this is, if the character is not allowed it must be percent
encoded.

To define connection attributes when creating a session using a dictionary the
connection-attributes option should be defined, its value can be set in the
following formats:

- Array of "key=value" pairs.
- Dictionary containing the key-value pairs.

Note that the connection-attribute values are expected to be strings, if other
data type is used in the dictionary, the string representation of the used data
will be stored on the database.

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
?{(VER(>=5.7.27) and VER(<8.0.0)) or VER(>=8.0.17)}
- DESC
- DESCRIBE
?{}
- EXPLAIN
- KILL

For help on a specific topic use: \? <topic>

e.g.: \? ALTER TABLE

#@<OUT> Help on connection attributes
Connection Attributes

Connection attributes are key-value pairs to be sent to the server at connect
time. They are stored at the following PERFORMANCE_SCHEMA tables:

- session_account_connect_attrs: attributes for the current session, and other
  sessions associated with the session account.
- session_connect_attrs: attributes for all sessions.

These attributes should be defined when creating a session and are immutable
during the life-time of the session.

To define connection attributes on a URI, the connection-attributes should be
defined as part of the URI as follows:

root@localhost:port/schema?connection-attributes=[att1=value1,att2=val2,...]

Note that the characters used for the attribute name and value must follow the
URI standard, this is, if the character is not allowed it must be percent
encoded.

To define connection attributes when creating a session using a dictionary the
connection-attributes option should be defined, its value can be set in the
following formats:

- Array of "key=value" pairs.
- Dictionary containing the key-value pairs.

Note that the connection-attribute values are expected to be strings, if other
data type is used in the dictionary, the string representation of the used data
will be stored on the database.

#@<OUT> Help on extension objects
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
shell.create_extension_object function.

Members can be attached to the extension object by calling the
shell.add_extension_object_member function.

Registration of an extension object as a global object can be done by calling
the shell.register_global function.

Alternatively, the extension object can be added as a member of another
extension object by calling the shell.add_extension_object_member function.

You can get more details on these functions by executing:

- \? create_extension_object
- \? add_extension_object_member
- \? register_global

Naming Convention

The core MySQL Shell APIs follow a specific naming convention for object
members. Extension objects should follow the same naming convention for
consistency reasons.

- JavaScript members use camelCaseNaming
- Python members use snake_case_naming

To simplify this across languages, it is important to use camelCaseNaming when
specifying the 'name' parameter of the shell.add_extension_object_member
function.

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

These scripts can be used to define extension objects so there are available
right away when the MySQL Shell starts.

#@<OUT> unparse_uri
NAME
      unparse_uri - Formats the given connection options to a URI string
                    suitable for mysqlsh.

SYNTAX
      shell.unparse_uri(options)

WHERE
      options: a dictionary with the connection options.

RETURNS
      A URI string

DESCRIPTION
      This function assembles a MySQL connection string which can be used in
      the shell or X DevAPI connectors.

#@<OUT> dump_rows
NAME
      dump_rows - Formats and dumps the given resultset object to the console.

SYNTAX
      shell.dump_rows(result, format)

WHERE
      result: The resultset object to dump
      format: One of table, tabbed, vertical, json, ndjson, json/raw,
              json/array, json/pretty. Default is table.

RETURNS
      The number of printed rows

DESCRIPTION
      This function shows a resultset object returned by a DB Session query in
      the same formats supported by the shell.

      Note that the resultset will be consumed by the function.

#@<OUT> ShellContextWrapper
NAME
      ShellContextWrapper - Holds shell context which is used by a thread,
                            gives access to the shell context object through
                            get_shell().

DESCRIPTION
      Holds shell context which is used by a thread, gives access to the shell
      context object through get_shell().

PROPERTIES
      globals
            Gives access to the global objects registered on the context.

FUNCTIONS
      get_shell()
              Shell object in its own context

      help([member])
            Provides help about this class and it's members

#@<OUT> ShellContextWrapper.globals
NAME
      globals - Gives access to the global objects registered on the context.

SYNTAX
      <ShellContextWrapper>.globals

#@<OUT> ShellContextWrapper.get_shell
NAME
      get_shell -   Shell object in its own context

SYNTAX
      <ShellContextWrapper>.get_shell()

RETURNS
      Shell

DESCRIPTION
        This function returns shell object that has its own scope.

