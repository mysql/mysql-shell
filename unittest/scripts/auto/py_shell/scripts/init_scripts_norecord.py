#@<> {__os_type == 'windows'}
import os
program_data = os.getenv("PROGRAMDATA")
mysql_dir = os.path.join(program_data, "MySQL")
mysqlsh_dir = os.path.join(mysql_dir, "mysqlsh")
init_py = os.path.join(mysqlsh_dir, "mysqlshrc.py")
init_js = os.path.join(mysqlsh_dir, "mysqlshrc.js")

mysql_dir_exists = os.path.exists(mysql_dir)
mysqlsh_dir_exists = os.path.exists(mysqlsh_dir)
init_py_exists = os.path.exists(init_py)
init_js_exists = os.path.exists(init_js)

#@<> Setup
if not mysql_dir_exists:
    os.mkdir(mysql_dir)

if not mysqlsh_dir_exists:
    os.mkdir(mysqlsh_dir)

# Ensures a startup file for JS and PY exist
if not init_js_exists:
    testutil.create_file(init_js, "print('Loaded JS Startup File')")
if not init_py_exists:
    testutil.create_file(init_py, "print('Loaded PY Startup File')")

# Create test files to verify both startup and mode switch on each case
testutil.create_file('test.js', "\\py")
testutil.create_file('test.py', "\\js")

#@<> Tests initialization and switch to python
WIPE_OUTPUT()
testutil.call_mysqlsh(["--interactive=full", "-f", 'test.js'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS(f"WARNING: Global startup scripts are no longer supported, '{init_js}' will be ignored.")
EXPECT_STDOUT_CONTAINS(f"WARNING: Global startup scripts are no longer supported, '{init_py}' will be ignored.")
EXPECT_STDOUT_NOT_CONTAINS('Loaded JS Startup File')
EXPECT_STDOUT_NOT_CONTAINS('Loaded PY Startup File')

#@<> Tests initialization and switch to javascript
WIPE_OUTPUT()
testutil.call_mysqlsh(["--interactive=full", "-f", 'test.js'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS(f"WARNING: Global startup scripts are no longer supported, '{init_js}' will be ignored.")
EXPECT_STDOUT_CONTAINS(f"WARNING: Global startup scripts are no longer supported, '{init_py}' will be ignored.")
EXPECT_STDOUT_NOT_CONTAINS('Loaded JS Startup File')
EXPECT_STDOUT_NOT_CONTAINS('Loaded PY Startup File')

#@<> Cleanup
os.remove('test.js')
os.remove('test.py')

# Deletes the initialization files if were created by the test
if not init_js_exists:
    os.remove(init_js)
if not init_py_exists:
    os.remove(init_py)

if not mysqlsh_dir_exists:
    os.rmdir(mysqlsh_dir)

if not mysql_dir_exists:
    os.rmdir(mysql_dir)

