#@<> Initialization
import os

user_path = testutil.get_user_config_path()
plugins_path = os.path.join(user_path, "plugins")
plugin_folder_path = os.path.join(plugins_path, "custom_processor")
plugin_path = os.path.join(plugin_folder_path, "init.py")
testutil.mkdir(plugin_folder_path, True)


def call_mysqlsh(command_line_args):
    testutil.call_mysqlsh(command_line_args, "", [
                          "MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Errors Registering SQL Handler - Missing Name
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler(prefixes=["SHOW Databases", "show tables"])
def sql_show_logger(session, sql):
    "Shows calls to SHOW statements"
    print(f"====> SQL LOGGER: {sql}")
"""
testutil.create_file(plugin_path, plugin_code)

WIPE_SHELL_LOG()
call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("WARNING: Found errors loading plugins, for more details look at the log")
EXPECT_SHELL_LOG_CONTAINS("sql_handler() missing 1 required positional argument: 'name'")

#@<> Errors Registering SQL Handler - Name Not String
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler(45, prefixes=["SHOW Databases", "show tables"])
def sql_show_logger(session, sql):
    "Shows calls to SHOW statements"
    print(f"====> SQL LOGGER: {sql}")
"""
testutil.create_file(plugin_path, plugin_code)

WIPE_SHELL_LOG()
call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("WARNING: Found errors loading plugins, for more details look at the log")
EXPECT_SHELL_LOG_CONTAINS("Argument #1 is expected to be a string")


#@<> Errors Registering SQL Handler - Missing Prefixes
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler("showLogger")
def sql_show_logger(session, sql):
    "Shows calls to SHOW statements"
    print(f"====> SQL LOGGER: {sql}")
"""
testutil.create_file(plugin_path, plugin_code)

WIPE_SHELL_LOG()
call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("WARNING: Found errors loading plugins, for more details look at the log")
EXPECT_SHELL_LOG_CONTAINS("sql_handler() missing 1 required positional argument: 'prefixes'")

#@<> Errors Registering SQL Handler - Invalid Prefixes Type
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler("showLogger", prefixes=45)
def sql_show_logger(session, sql):
    "Shows calls to SHOW statements"
    print(f"====> SQL LOGGER: {sql}")
"""
testutil.create_file(plugin_path, plugin_code)

WIPE_SHELL_LOG()
call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("WARNING: Found errors loading plugins, for more details look at the log")
EXPECT_SHELL_LOG_CONTAINS("sql_handler() argument 'prefixes' must be a list of strings")

#@<> Errors Registering SQL Handler - Invalid Prefixes Element Type
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler("showLogger", prefixes=["one", 45])
def sql_show_logger(session, sql):
    "Shows calls to SHOW statements"
    print(f"====> SQL LOGGER: {sql}")
"""
testutil.create_file(plugin_path, plugin_code)

WIPE_SHELL_LOG()
call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("WARNING: Found errors loading plugins, for more details look at the log")
EXPECT_SHELL_LOG_CONTAINS("sql_handler() argument 'prefixes' must be a list of strings")

#@<> Errors Registering SQL Handler - Empty Prefixes
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler("showLogger", prefixes=[])
def sql_show_logger(session, sql):
    "Shows calls to SHOW statements"
    print(f"====> SQL LOGGER: {sql}")
"""
testutil.create_file(plugin_path, plugin_code)

WIPE_SHELL_LOG()
call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("WARNING: Found errors loading plugins, for more details look at the log")
EXPECT_SHELL_LOG_CONTAINS("sql_handler() argument 'prefixes' can not be empty")

#@<> Errors Registering SQL Handler - Missing DocString
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler("showLogger", prefixes=["one"])
def sql_show_logger(session, sql):
    print(f"====> SQL LOGGER: {sql}")
"""
testutil.create_file(plugin_path, plugin_code)

WIPE_SHELL_LOG()
call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("WARNING: Found errors loading plugins, for more details look at the log")
EXPECT_SHELL_LOG_CONTAINS("sql_handler() function 'sql_show_logger' is missing a description")

#@<> Simple logging SQL handler
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler("showLogger", prefixes=["SHOW Databases", "show tables"])
def sql_show_logger(session, sql):
    "Shows calls to SHOW statements"
    print(f"====> SQL LOGGER: {sql}")
"""
testutil.create_file(plugin_path, plugin_code)

call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("""====> SQL LOGGER: show databases
Database""")
WIPE_OUTPUT()

call_mysqlsh([__mysqluripwd, "--sql", "-e", "show tables from mysql"])
EXPECT_STDOUT_CONTAINS("""====> SQL LOGGER: show tables from mysql
Tables_in_mysql""")
WIPE_OUTPUT()

call_mysqlsh([__mysqluripwd, "--sql", "-e", "SHOW PLUGINS"])
EXPECT_STDOUT_NOT_CONTAINS("====> SQL LOGGER:")
WIPE_OUTPUT()


#@<> Multiple SQL Handlers, executed in registration order
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler("globalShow", prefixes=["SHOW DATABASES"])
def first_handler(session, sql):
    "Logs any call to a SHOW statement"
    print(f"====> FIRST HANDLER: {sql}")

    # On this specific case will return a result
    if sql == "show databases tweaked":
        return mysqlsh.globals.shell.create_result()

@sql_handler("showTables", prefixes=["SHOW TABLES"])
def second_handler(session, sql):
    "Logs calls to SHOW TABLES"
    print(f"====> SECOND HANDLER: {sql}")
"""
testutil.create_file(plugin_path, plugin_code)

call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("""====> FIRST HANDLER: show databases
Database""")
EXPECT_STDOUT_NOT_CONTAINS("====> THIRD HANDLER:")
WIPE_OUTPUT()

call_mysqlsh([__mysqluripwd, "--sql", "-e", "show tables from mysql"])
EXPECT_STDOUT_CONTAINS("""====> SECOND HANDLER: show tables from mysql
Tables_in_mysql""")
EXPECT_STDOUT_NOT_CONTAINS("====> FIRST HANDLER:")
WIPE_OUTPUT()

call_mysqlsh([__mysqluripwd, "-i", "--py", "-e",
             "session.run_sql('show databases tweaked')"])
EXPECT_STDOUT_CONTAINS("""====> FIRST HANDLER: show databases tweaked
Query OK, 0 rows affected (0.0000 sec)""")

# Since processinig is aborted neither the otherhandler or the shell session process the sql
EXPECT_STDOUT_NOT_CONTAINS("""====> SECOND HANDLER:""")
WIPE_OUTPUT()


#@<> Execution of nested extended SQL is forbidden
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler("showmeHandler", prefixes=["SHOWME"])
def my_handler(session, sql):
    "Handles custom SHOWME statement"
    print(f"====> SQL HANDLER: {sql}")

    if sql == "showme":
        sql = "showme databases"

    return session.run_sql(sql)
"""
testutil.create_file(plugin_path, plugin_code)

call_mysqlsh([__mysqluripwd, "-i", "--tabbed", "--py",
             "-e", "session.run_sql('showme')"])

EXPECT_STDOUT_CONTAINS("RuntimeError: LogicError: Unable to execute a sql handler while another is being executed. Executing SQL: showme. Unable to execute: showme databases")
EXPECT_STDOUT_NOT_CONTAINS("====> SQL HANDLER: showme databases")


#@<> Execution of standard SQL while executing extended SQL is allowed
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh

@sql_handler("showmeHandler", prefixes=["SHOWME"])
def my_handler(session, sql):
    "Handles custom SHOWME statement"
    print(f"====> SQL HANDLER: {sql}")

    if sql == "showme":
        sql = "show databases"

    return session.run_sql(sql)
"""
testutil.create_file(plugin_path, plugin_code)

call_mysqlsh([__mysqluripwd, "-i", "--tabbed", "--py",
             "-e", "session.run_sql('showme')"])

EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: showme
Database
""")
EXPECT_STDOUT_NOT_CONTAINS("====> SQL HANDLER: show databases")

#@<> Handlers are disabled of --disable-plugins is used
WIPE_OUTPUT()
call_mysqlsh([__mysqluripwd, "--py", "--disable-plugins", "-i", "--tabbed",
            "-e", "session.run_sql('showme')"])
EXPECT_STDOUT_CONTAINS('You have an error in your SQL syntax;')
EXPECT_STDOUT_NOT_CONTAINS('====> SQL HANDLER:')

#@<> SQL Handler - Comment Handling
plugin_code = """
from mysqlsh.plugin_manager import sql_handler
import mysqlsh
@sql_handler("showmeHandler", prefixes=["LIST DATABASES", "LIST TABLES"])
def my_handler(session, sql):
    "Logger for list statements"
    print(f"====> SQL HANDLER: {sql}")
    return session.run_sql(sql.replace('list', 'show'))
"""
testutil.create_file(plugin_path, plugin_code)
call_mysqlsh([__mysqluripwd, "-i", "--tabbed", "--sql",
             "-e", "/* prefix comment */ list databases"])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: /* prefix comment */ list databases""")
WIPE_OUTPUT()
call_mysqlsh([__mysqluripwd, "-i", "--tabbed", "--sql",
             "-e", "list /* middle comment */ databases"])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: list /* middle comment */ databases""")
WIPE_OUTPUT()
call_mysqlsh([__mysqluripwd, "-i", "--tabbed", "--sql",
             "-e", "/* prefix comment */ list /* middle comment */ databases"])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: /* prefix comment */ list /* middle comment */ databases""")
WIPE_OUTPUT()
call_mysqlsh([__mysqluripwd, "-i", "--tabbed", "--js",
             "-e", "session.runSql('/* prefix comment */ list /* middle comment */ databases')"])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: /* prefix comment */ list /* middle comment */ databases""")
WIPE_OUTPUT()
call_mysqlsh([__uripwd, "-i", "--tabbed", "--js",
             "-e", "session.runSql('/* prefix comment */ list /* middle comment */ databases')"])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: /* prefix comment */ list /* middle comment */ databases""")
WIPE_OUTPUT()
script_path = os.path.join(user_path, "test.sql")
script_code = """
-- This script verifies the SQL Handlers work properly in the presense of comments
-- Full line comment v1
list databases like 'mysql';
# Full line comment v2
list databases like 'information_schema';
/* Delimited Comment */
list databases like 'performance_schema';
use mysql;
/* prefix comment */ list tables like 'user';
list /* middle comment */ tables like 'plugin';
/* prefix comment */ list /* and middle comment */ tables like 'db';
/* 
This is a multiline comment
*/
list tables like 'func';
"""
testutil.create_file(script_path, script_code)
call_mysqlsh([__mysqluripwd, "-i", "--tabbed", "--sql",
             "-f", script_path])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: list databases like 'mysql'
Database (mysql)
""")
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: list databases like 'information_schema'
Database (information_schema)
""")
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: /* prefix comment */ list tables like 'user'
Tables_in_mysql (user)
""")
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: list /* middle comment */ tables like 'plugin'
Tables_in_mysql (plugin)
""")
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: /* prefix comment */ list /* and middle comment */ tables like 'db'
Tables_in_mysql (db)
""")
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: /* 
This is a multiline comment
*/
list tables like 'func'
Tables_in_mysql (func)
""")

# @<> Finalization
testutil.rmfile(script_path)
testutil.rmdir(plugins_path, True)
