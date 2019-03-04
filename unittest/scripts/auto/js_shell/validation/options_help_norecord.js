//@<OUT> Shell Options Help
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

