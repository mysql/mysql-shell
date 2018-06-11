#@<OUT> Shell Options Help
NAME
      options - Gives access to options impacting shell behavior.

SYNTAX
      shell.options

DESCRIPTION
      The options object acts as a dictionary, it may contain the following
      attributes:

      - batchContinueOnError: read-only, boolean value to indicate if the
        execution of an SQL script in batch mode shall continue if errors occur
      - interactive: read-only, boolean value that indicates if the shell is
        running in interactive mode
      - outputFormat: controls the type of output produced for SQL results.
      - sandboxDir: default path where the new sandbox instances for InnoDB
        cluster will be deployed
      - showWarnings: boolean value to indicate whether warnings shall be
        included when printing an SQL result
      - useWizards: read-only, boolean value to indicate if the Shell is using
        the interactive wrappers (wizard mode)
      - history.maxSize: number of entries to keep in command history
      - history.autoSave: true to save command history when exiting the shell
      - history.sql.ignorePattern: colon separated list of glob patterns to
        filter out of the command history in SQL mode

      The outputFormat option supports the following values:

      - table: displays the output in table format (default)
      - json: displays the output in JSON format
      - json/raw: displays the output in a JSON format but in a single line
      - vertical: displays the outputs vertically, one line per column value
      - autocomplete.nameCache: true if auto-refresh of DB object name cache is
        enabled. The \rehash command can be used for manual refresh

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

#@<OUT> Help on help
NAME
      help - Provides help about this object and it's members

SYNTAX
      options.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> Help on set
NAME
      set - Sets value of an option.

SYNTAX
      options.set(optionName, value)

WHERE
      optionName: name of the option to set.
      value: new value for the option.

#@<OUT> Help on set_persist
NAME
      set_persist - Sets value of an option and stores it in the configuration
                    file.

SYNTAX
      options.set_persist(optionName, value)

WHERE
      optionName: name of the option to set.
      value: new value for the option.

#@<OUT> Help on unset
NAME
      unset - Resets value of an option to default.

SYNTAX
      options.unset(optionName)

WHERE
      optionName: name of the option to reset.

#@<OUT> Help on unset_persist
NAME
      unset_persist - Resets value of an option to default and removes it from
                      the configuration file.

SYNTAX
      options.unset_persist(optionName)

WHERE
      optionName: name of the option to reset.
