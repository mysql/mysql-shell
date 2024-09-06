//@<OUT> Shell Options Help
NAME
      options - Gives access to options impacting shell behavior.

SYNTAX
      shell.options

DESCRIPTION
      The options object acts as a dictionary, it may contain the following
      attributes:

      - autocomplete.nameCache: true if auto-refresh of DB object name cache is
        enabled. The \rehash command can be used for manual refresh.
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
      - dba.connectivityChecks: bool, checks SSL settings and network
        connectivity between instances when creating a cluster, replicaset or
        clusterset, or adding an instance to one
      - dba.gtidWaitTimeout: timeout value in seconds to wait for GTIDs to be
        synchronized
      - dba.logSql: 0..2, log SQL statements executed by AdminAPI operations: 0
        - logging disabled; 1 - log statements other than SELECT and SHOW; 2 -
        log all statements. Option takes precedence over --log-sql in Dba.*
        context, if enabled.
      - dba.restartWaitTimeout: timeout in seconds to wait for MySQL server to
        come back after a restart during clone recovery
      - dba.versionCompatibilityChecks: bool, checks version compatibility for
        asynchronous replication when managing a ReplicaSet, ClusterSet, or a
        Cluster with Read-Replicas.
      - defaultCompress: Enable compression in client/server protocol by
        default in global shell sessions.
      - defaultMode: shell mode to use when shell is started, allowed values:
        "js", "py", "sql" or "none"
      - devapi.dbObjectHandles: true to enable schema collection and table name
        aliases in the db object, for DevAPI operations
      - history.autoSave: true to save command history when exiting the shell
      - history.maxSize: number of entries to keep in command history
      - history.sql.ignorePattern: colon separated list of glob patterns to
        filter out of the command history in SQL mode
      - history.sql.syslog: true to log filtered interactive commands to the
        system log, filtering of commands depends on the value of
        history.sql.ignorePattern
      - interactive: read-only, boolean value that indicates if the shell is
        running in interactive mode
      - logFile: read-only, path to the log file. Use --log-file to change the
        location.
      - logLevel: current log level, allowed values are integers between 1 and
        8, or one of: "none", "internal", "error", "warning", "info", "debug",
        "debug2", "debug3". If value is prefixed with '@', log messages are
        also written to the stderr.
      - logSql: Log SQL statements: off - none of SQL statements will be
        logged; error (default) - SQL statement with error message will be
        logged only when error occurs; on - all SQL statements will be logged
        except these which match any of logSql.ignorePattern and
        logSql.ignorePatternUnsafe glob pattern; all - all SQL statements will
        be logged except these which match any of logSql.ignorePatternUnsafe
        glob pattern; unfiltered - all SQL statements will be logged
      - logSql.ignorePattern: Colon separated list of glob patterns to filter
        out SQL queries to be logged when logSql is set to "on". Default:
        *SELECT*:*SHOW*
      - logSql.ignorePatternUnsafe: Colon separated list of glob patterns to
        filter out SQL queries to be logged when logSql is set to "all".
        Default: *IDENTIFIED*:*PASSWORD*
      - mysqlPluginDir: Directory for client-side authentication plugins
      - oci.configFile: Path to OCI (Oracle Cloud Infrastructure) configuration
        file
      - oci.profile: Specify which section in oci.configFile will be used as
        profile settings
      - outputFormat: Deprecated, use resultFormat instead
      - pager: string which specifies the external command which is going to be
        used to display the paged output
      - passwordsFromStdin: boolean value that indicates if the shell should
        read passwords from stdin instead of the tty
      - resultFormat: controls the type of output produced for SQL results
      - sandboxDir: default path where the new sandbox instances for InnoDB
        cluster will be deployed
      - showColumnTypeInfo: display column type information in SQL mode. Please
        be aware that output may depend on the protocol you are using to
        connect to the server, e.g. DbType field is approximated when using X
        protocol.
      - showWarnings: boolean value to indicate whether warnings shall be
        included when printing a SQL result
      - ssh.bufferSize integer, default 10240 bytes, used for tunnel data
        transfer
      - ssh.configFile string, default empty, custom path for SSH
        configuration. If not defined, the standard SSH paths will be used
        (~/.ssh/config).
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

//@<OUT> Help on help
NAME
      help - Provides help about this object and it's members

SYNTAX
      shell.options.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on set
NAME
      set - Sets value of an option.

SYNTAX
      shell.options.set(optionName, value)

WHERE
      optionName: name of the option to set.
      value: new value for the option.

//@<OUT> Help on setPersist
NAME
      setPersist - Sets value of an option and stores it in the configuration
                   file.

SYNTAX
      shell.options.setPersist(optionName, value)

WHERE
      optionName: name of the option to set.
      value: new value for the option.

//@<OUT> Help on unset
NAME
      unset - Resets value of an option to default.

SYNTAX
      shell.options.unset(optionName)

WHERE
      optionName: name of the option to reset.

//@<OUT> Help on unsetPersist
NAME
      unsetPersist - Resets value of an option to default and removes it from
                     the configuration file.

SYNTAX
      shell.options.unsetPersist(optionName)

WHERE
      optionName: name of the option to reset.
