#@ initialization
||

#@ \system command with parameter
||

#@<OUT> \system command with parameter - verification
['system']

#@ \! command with parameter
||

#@<OUT> \! command with parameter - verification
['!']

#@ \! command with quotes
||

#@<OUT> \! command with quotes - verification
['one', 'two three', 'four', 'five "six" seven']

#@ \! command with redirection
||

#@<OUT> \! command with redirection - verification
['redirection']

#@<ERR> \system command with no arguments prints help
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

#@ \system command with an error
|ERROR: System command "unknown_shell_command" returned exit code: |

#@ cleanup
||
