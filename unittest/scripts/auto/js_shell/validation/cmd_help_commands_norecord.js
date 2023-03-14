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

      - --mc, --mysql: create a classic MySQL protocol session (default port
        3306)
      - --mx, --mysqlx: create an X protocol session (default port 33060)
      - --ssh <SSHURI>: create an SSH tunnel to use as a gateway for db
        connection. This requires that db port is specified in advance.

      If TYPE is omitted, automatic protocol detection is done, unless the
      protocol is given in the URI.

      URI and SSHURI format is: [user[:password]@]hostname[:port]

EXAMPLE
      \connect --mx root@localhost
            Creates a global session using the X protocol to the indicated URI.

//@<OUT> Edit Command
NAME
      \edit - Launch a system editor to edit a command to be executed.

SYNTAX
      \edit [arguments...]
      \e [arguments...]

DESCRIPTION
      The system editor is selected using the EDITOR and VISUAL environment
      variables.

?{__os_type == 'windows'}
      If these are not set, falls back to notepad.exe.
?{}
?{__os_type != 'windows'}
      If these are not set, falls back to vi.
?{}

      It is also possible to invoke this command by pressing CTRL-X CTRL-E.

EXAMPLES
      \edit
            Allows to edit the last command in history.

            If there are no commands in history, editor will be blank.

      \e print('hello world!')
            Allows to edit the commands given as arguments.

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

- ? matches any single character.
- * matches any character sequence.

The following are the main help categories:

 - AdminAPI       The AdminAPI is an API that enables configuring and managing
                  InnoDB Clusters, ReplicaSets, ClusterSets, among other
                  things.
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

      - del range         Deletes entry/entries from history.
      - clear             Clear history.
      - save              Save history to file.

      If no options are given the command will display the history entries.

      Range in the delete operation can be given in one of the following forms:

      - num single number identifying entry to delete.
      - num-num numbers specifying lower and upper bounds of the range.
      - num- range from num till the end of history.
      - -num last num entries.

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

      \history del -5
            Deletes last 5 entries from the history.

//@<OUT> JavaScript Command
NAME
      \js - Switches to JavaScript processing mode.

SYNTAX
      \js

//@<OUT> nopager Command
NAME
      \nopager - Disables the current pager.

SYNTAX
      \nopager

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
      - --persist causes an option to be stored on the configuration file.
      - --unset resets an option value to the default value, removes the option
        from configuration file when used together with --persist option.

EXAMPLES
      \option --persist defaultMode sql

      \option --unset --persist defaultMode

//@<OUT> pager Command
NAME
      \pager - Sets the current pager.

SYNTAX
      \pager [command]
      \P [command]

DESCRIPTION
      The current pager will be automatically used to:

      - display results of statements executed in SQL mode,
      - display text output of \help command,
      - display text output in scripting mode, after shell.enablePager() has
        been called,

      Pager is going to be used only if shell is running in interactive mode.

EXAMPLES
      \pager
            With no parameters this command restores the initial pager.

      \pager ""
            Restores the initial pager.

      \pager more
            Sets pager to "more".

      \pager "more -10"
            Sets pager to "more -10".

      \pager more -10
            Sets pager to "more -10".

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

//@<OUT> Show Command
NAME
      \show - Executes the given report with provided options and arguments.

SYNTAX
      \show <report_name> [options] [arguments]

DESCRIPTION
      The report name accepted by the \show command is case-insensitive, '-'
      and '_' characters can be used interchangeably.

      Common options:

      - --help - Display help of the given report.
      - --vertical, -E - For 'list' type reports, display records vertically.

      The output format of \show command depends on the type of report:

      - 'list' - displays records in tabular form (or vertically, if --vertical
        is used),
      - 'report' - displays a YAML text report,
      - 'print' - does not display anything, report is responsible for text
        output.

      If executed without the report name, lists available reports.

      Note: user-defined reports can be registered with shell.registerReport()
      method.

EXAMPLES
      \show
            Lists available reports, both built-in and user-defined.

      \show query show session status like 'Uptime%'
            Executes 'query' report with the provided SQL statement.

      \show query --vertical show session status like 'Uptime%'
            As above, but results are displayed in vertical form.

      \show query --help
            Displays help for the 'query' report.

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

//@<OUT> SQL Command
NAME
      \sql - Executes SQL statement or switches to SQL processing mode when no
      statement is given.

SYNTAX
      \sql [statement]

//@<OUT> Status Command
NAME
      \status - Print information about the current global session.

SYNTAX
      \status
      \s

//@<OUT> System Command
NAME
      \system - Execute a system shell command.

SYNTAX
      \system [command [arguments...]]
      \! [command [arguments...]]

EXAMPLES
      \system
            With no arguments, this command displays this help.

      \system ls
            Executes 'ls' in the current working directory and displays the
            result.

      \! ls > list.txt
            Executes 'ls' in the current working directory, storing the result
            in the 'list.txt' file.

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

//@<OUT> Watch Command
NAME
      \watch - Executes the given report with provided options and arguments in
      a loop.

SYNTAX
      \watch <report_name> [options] [arguments]

DESCRIPTION
      This command behaves like \show command, but the given report is executed
      repeatedly, refreshing the screen every 2 seconds until CTRL-C is
      pressed.

      In addition to \show command options, following are also supported:

      - --interval=float, -i float - Number of seconds to wait between
        refreshes. Default 2. Allowed values are in range [0.1, 86400].
      - --nocls - Don't clear the screen between refreshes.

      If executed without the report name, lists available reports.

      For more information see \show command.

EXAMPLES
      \watch
            Lists available reports, both built-in and user-defined.

      \watch query --interval=1 show session status like 'Uptime%'
            Executes the 'query' report refreshing the screen every second.

      \watch query --nocls show session status like 'Uptime%'
            As above, but screen is not cleared, results are displayed one
            after another.
