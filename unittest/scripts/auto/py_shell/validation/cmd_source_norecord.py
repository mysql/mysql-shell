#@ Fail test
||IndentationError: unexpected indent

#@<OUT> Success test
This is JS

#@ can only override lang when sourcing interactively
||Failed to open file '--js', error: No such file or directory
||Failed to open file '--js', error: No such file or directory
||Failed to open file '--js', error: No such file or directory

#@<OUT> from the cmdline - disallowed
ERROR: 1064 (42000) at line 1: You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near '\source --js cmd_source_test.js' at line 1

#@<OUT> from the cmdline - disallowed2
Failed to open file '--js', error: No such file or directory

#@<OUT> from stdin - disallowed
ERROR: 1064 (42000) at line 1: You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near '\source --js cmd_source_test.js' at line 1

#@<OUT> from stdin - disallowed2
Failed to open file '--js', error: No such file or directory
