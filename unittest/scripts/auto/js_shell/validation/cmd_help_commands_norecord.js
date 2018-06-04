//@<OUT> Multiline Command
NAME
      \ - Start multi-line input when in SQL mode.

SYNTAX
      \

//@<OUT> Connect Command
NAME
      \connect - Connects the shell to a MySQL server and assigns the global
      session.

SYNTAX
      \connect [<TYPE>] <URI>

      \c [<TYPE>] <URI>

DESCRIPTION
      TYPE is an optional parameter to specify the session type. Accepts the
      following values:

      - -mc, --mysql: create a classic MySQL protocol session (default port
        3306)
      - -mx, --mysqlx: create an X protocol session (default port 33060)
      - -ma: attempt to create a session using automatic detection of the
        protocol type

      If TYPE is omitted, -ma is assumed by default, unless the protocol is
      given in the URI.

      URI format is: [user[:password]@]hostname[:port]

EXAMPLE
      \connect -mx root@localhost
            Creates a global session using the X protocol to the indicated URI.

//@<OUT> Exit Command
NAME
      \exit - Exits the MySQL Shell, same as \quit.

SYNTAX
      \exit

//@<OUT> Help Command
NAME
      \help - Prints help information about a specific topic.

SYNTAX
      \help [pattern]

      \h [pattern]

      \? [pattern]

DESCRIPTION
      The Shell Help is organized in categories and topics. To get help for a
specific category or topic use: \? <pattern>

The <pattern> argument should be the name of a category or a topic.

The pattern is a filter to identify topics for which help is required, it can
use the following wildcards:

- ? matches any single charecter.
- * matches any character sequence.

The following are the main help categories:

 - AdminAPI       Introduces to the dba global object and the InnoDB cluster
                  administration API.
 - Shell Commands Provides details about the available built-in shell commands.
 - ShellAPI       Contains information about the shell and util global objects
                  as well as the mysql module that enables executing SQL on
                  MySQL Servers.
 - SQL Syntax     Entry point to retrieve syntax help on SQL statements.
 - X DevAPI       Details the mysqlx module as well as the capabilities of the
                  X DevAPI which enable working with MySQL as a Document Store

EXAMPLES
      \help
            With no parameters this command prints the general shell help.

      \? contents
            Describes information about the help organization.

      \h cluster
            Prints the information available for the Cluster class.

      \? *sandbox*
            List the available functions for sandbox operations.

//@<OUT> History Command
NAME
      \history - View and edit command line history.

SYNTAX
      \history [options].

DESCRIPTION
      The operation done by this command depends on the given options. Valid
      options are:

      - del <num>[-<num>] Deletes entry/entries from history.
      - clear             Clear history.
      - save              Save history to file.

      If no options are given the command will display the history entries.

      NOTE: The history.autoSave shell option must be set to true to
      automatically save the contents of the command history when MySQL Shell
      exits.

EXAMPLES
      \history
            Displays the entire history.

      \history del 123
            Deletes entry number 123 from the history.

      \history del 10-20
            Deletes range of entries from number 10 to 20 from the history.

      \history del 10-
            Deletes entries from number 10 and ahead from the history.

//@<OUT> JavaScript Command
NAME
      \js - Switches to JavaScript processing mode.

SYNTAX
      \js

//@<OUT> Nowarnings Command
NAME
      \nowarnings - Don't show warnings after every statement.

SYNTAX
      \nowarnings

      \w

//@<OUT> Option Command
NAME
      \option - Allows working with the available shell options.

SYNTAX
      \option [args]

DESCRIPTION
      The given [args] define the operation to be done by this command, the
      following values are accepted

      - -h, --help [<filter>]: print help for the shell options matching
        filter.
      - -l, --list [--show-origin]: list all the shell options.
      - <shell_option>: print value of the shell option.
      - <shell_option> [=] <value> sets the value for the shell option.
      - --persist causes an option to be stored on the configuration file
      - --unset resets an option value to the default value.

//@<OUT> Python Command
NAME
      \py - Switches to Python processing mode.

SYNTAX
      \py

//@<OUT> Quit Command
NAME
      \quit - Exits the MySQL Shell.

SYNTAX
      \quit

//@<OUT> Reconnect Command
NAME
      \reconnect - Reconnects the global session.

SYNTAX
      \reconnect

//@<OUT> Rehash Command
NAME
      \rehash - Refresh the autocompletion cache.

SYNTAX
      \rehash

DESCRIPTION
      Populate or refresh the schema object name cache used for SQL
      auto-completion and the DevAPI schema object.

      A rehash is automatically done whenever the 'use' command is executed,
      unless the shell is started with --no-name-cache.

      This may take a long time if you have many schemas or many objects in the
      default schema.

//@<OUT> Source Command
NAME
      \source - Loads and executes a script from a file.

SYNTAX
      \source <path>

      \. <path>

DESCRIPTION
      Executes a script from a file, the following languages are supported:

      - JavaScript
      - Python
      - SQL

      The file will be loaded and executed using the active language.

EXAMPLES
      \source /home/me/sakila.sql

      \. /home/me/sakila.sql

//@<OUT> SQL Command
NAME
      \sql - Switches to SQL processing mode.

SYNTAX
      \sql

//@<OUT> Status Command
NAME
      \status - Print information about the current global session.

SYNTAX
      \status

      \s

//@<OUT> Use Command
NAME
      \use - Sets the active schema.

SYNTAX
      \use <schema>

      \u <schema>

DESCRIPTION
      Uses the global session to set the active schema.

      When this command is used:

      - The active schema in SQL mode will be updated.
      - The db global variable available on the scripting modes will be
        updated.

EXAMPLES
      \use mysql

      \u mysql

//@<OUT> Warnings Command
NAME
      \warnings - Show warnings after every statement.

SYNTAX
      \warnings

      \W

