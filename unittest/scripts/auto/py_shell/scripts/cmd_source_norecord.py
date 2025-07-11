#@<> setup
import os
path=os.path.join(__test_home_path, "scripts/auto/py_shell/scripts")
orig_path=os.getcwd()
os.chdir(path)

shell.connect(__mysqluripwd)

#@ Fail test
\source cmd_source_test.js
#@ Success test
\source --js cmd_source_test.js
#@ can only override lang when sourcing interactively
\sql
\source cmd_source_test.sql
source cmd_source_test.sql
\. cmd_source_test.sql
\py

#@ from the cmdline - disallowed
testutil.call_mysqlsh([__mysqluripwd, "-f", "cmd_source_test.sql"])

#@ from the cmdline - disallowed2
testutil.call_mysqlsh([__mysqluripwd, "-f", "cmd_source_test2.sql"])

#@ from stdin - disallowed
testutil.call_mysqlsh([__mysqluripwd], open("cmd_source_test.sql").read())

#@ from stdin - disallowed2
testutil.call_mysqlsh([__mysqluripwd], open("cmd_source_test2.sql").read())

#@<> cleanup
os.chdir(orig_path)
