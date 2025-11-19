#@<> Initialization
import os

user_path = testutil.get_user_config_path()
plugins_path = os.path.join(user_path, "plugins")
plugin_folder_path = os.path.join(plugins_path, "custom_processor")
plugin_path = os.path.join(plugin_folder_path, "init.py")
sql_file_path = os.path.join(user_path, "sample.sql")
testutil.mkdir(plugin_folder_path, True)


def call_mysqlsh(command_line_args):
    testutil.call_mysqlsh(["--disable-builtin-plugins"] + command_line_args, "", [
                          "MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Empty list of SQL Handlers
call_mysqlsh([__mysqluripwd, "--", "shell", "list-sql-handlers"])
EXPECT_OUTPUT_CONTAINS('[]')

#@<> SQL Handler Registration Errors
EXPECT_THROWS(lambda: shell.register_sql_handler(5, 5,
    45, None), "Argument #1 is expected to be a string")
EXPECT_THROWS(lambda: shell.register_sql_handler("", 5,
    45, None), "Argument #2 is expected to be a string")
EXPECT_THROWS(lambda: shell.register_sql_handler("", "",
    45, None), "Argument #3 is expected to be an array")
EXPECT_THROWS(lambda: shell.register_sql_handler("sample", "sample description", 
    ["SAMPLE"], 45), "Argument #4 is expected to be a function")
EXPECT_THROWS(lambda: shell.register_sql_handler("", "", [], None),
              "An empty name is not allowed")
EXPECT_THROWS(lambda: shell.register_sql_handler("sample", "", [], None),
              "An empty description is not allowed")
EXPECT_THROWS(lambda: shell.register_sql_handler("sample", "sample description", [], None),
              "At least one prefix must be specified")
EXPECT_THROWS(lambda: shell.register_sql_handler("sample", "sample description", 
    [45], None), "Argument #3 is expected to be an array of strings")
EXPECT_THROWS(lambda: shell.register_sql_handler("sample", "sample description", 
    [""], None), "Empty or blank prefixes are not allowed.")
EXPECT_THROWS(lambda: shell.register_sql_handler("sample", "sample description", 
    ["SAMPLE", "   "], None), "Empty or blank prefixes are not allowed.")
EXPECT_THROWS(lambda: shell.register_sql_handler("sample", "sample description", 
    ["SAMPLE"], None), "The callback function must be defined.")

#Error registering handler with same name
def one():
    pass

shell.register_sql_handler("pyOne", "one description", ["pyOne", "pysome other"], one)
EXPECT_THROWS(lambda: shell.register_sql_handler("pyOne", "one description",
    ["  pyone", "some other"], one), "An SQL Handler named 'pyOne' already exists.")
EXPECT_THROWS(lambda: shell.register_sql_handler("two", "one description",
    ["PYONE"], one), "The prefix 'PYONE' is shadowed by an existing prefix defined at the 'pyOne' SQL Handler")
EXPECT_THROWS(lambda: shell.register_sql_handler("two", "one description",
    ["pyone piece"], one), "The prefix 'pyone piece' is shadowed by an existing prefix defined at the 'pyOne' SQL Handler")
EXPECT_THROWS(lambda: shell.register_sql_handler("two", "two",
    ["pysome"], one), "The prefix 'pysome' will shadow an existing prefix defined at the 'pyOne' SQL Handler")

#@<> SQL Handler Runtime Errors (function does not met expected syntax)
plugin_code = """
def sample():
    pass

shell.register_sql_handler("sample1", "sample description", ["my bad callback syntax"], sample)


def invalid_return_data(session, sql):
    if sql.endswith("number"):
        return 45
    if sql.endswith("session"):
        return session


shell.register_sql_handler("sample2", "sample description", ["my bad callback return"], invalid_return_data)
"""
testutil.create_file(plugin_path, plugin_code)

call_mysqlsh([__mysqluripwd, "--sql", "-e", "my bad callback syntax"])
EXPECT_STDOUT_CONTAINS("sample() takes 0 positional arguments but 2 were given")
WIPE_OUTPUT()

call_mysqlsh([__mysqluripwd, "--sql", "-e", "my bad callback return number"])
EXPECT_STDOUT_CONTAINS("Invalid data returned by the SQL handler, if any, the returned data should be a result object.")
WIPE_OUTPUT()

call_mysqlsh([__mysqluripwd, "--sql", "-e", "my bad callback return session"])
EXPECT_STDOUT_CONTAINS("Invalid data returned by the SQL handler, if any, the returned data should be a result object.")
WIPE_OUTPUT()

#@<> Simple logging SQL handler
plugin_code = """
import mysqlsh

def sql_show_logger(session, sql):
    print(f"====> SQL LOGGER: {sql}")

mysqlsh.globals.shell.register_sql_handler("showHandler", "Prints SHOW Statements", ["SHOW Databases", "show tables"], sql_show_logger)

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
import mysqlsh

def first_handler(session, sql):
    print(f"====> FIRST HANDLER: {sql}")

    #On this specific case will return a result
    if sql == "show databases tweaked":
        return mysqlsh.globals.shell.create_result()

def second_handler(session, sql):
    print(f"====> SECOND HANDLER: {sql}")

mysqlsh.globals.shell.register_sql_handler("databaseShow", "Handler for SHOW DATABASE", ["SHOW DATABASES"], first_handler)
mysqlsh.globals.shell.register_sql_handler("tableShow", "Handler for SHOW TABLES", ["SHOW TABLES"], second_handler)

"""
testutil.create_file(plugin_path, plugin_code)

call_mysqlsh([__mysqluripwd, "--", "shell", "list-sql-handlers"])
EXPECT_STDOUT_CONTAINS_MULTILINE("""
[
    {
        "description": "Handler for SHOW DATABASE",
        "name": "databaseShow"
    },
    {
        "description": "Handler for SHOW TABLES",
        "name": "tableShow"
    }
]""")

call_mysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS("""====> FIRST HANDLER: show databases
Database""")
EXPECT_STDOUT_NOT_CONTAINS("====> SECOND HANDLER:")
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

#Since processinig is aborted neither the otherhandler or the shell session process the sql
EXPECT_STDOUT_NOT_CONTAINS("""====> SECOND HANDLER:""")

EXPECT_STDOUT_NOT_CONTAINS("====> THIRD HANDLER:")
WIPE_OUTPUT()


#@<> Execution of nested extended SQL is forbidden
plugin_code = """
import mysqlsh

def my_handler(session, sql):
    print(f"====> SQL HANDLER: {sql}")

    if sql == "showme":
        sql = "showme tables"

    return session.run_sql(sql)

mysqlsh.globals.shell.register_sql_handler("showHandler", "Handler for SHOW statements", ["SHOWME"], my_handler)
"""
testutil.create_file(plugin_path, plugin_code)

call_mysqlsh([__mysqluripwd, "-i", "--tabbed", "--py",
             "-e", "session.run_sql('showme')"])
EXPECT_STDOUT_CONTAINS("RuntimeError: LogicError: Unable to execute a sql handler while another is being executed. Executing SQL: showme. Unable to execute: showme tables")
EXPECT_STDOUT_NOT_CONTAINS("====> SQL HANDLER: showme tables")


#@<> Execution of standard SQL while executing extended SQL is allowed
plugin_code = """
import mysqlsh

def my_handler(session, sql):
    print(f"====> SQL HANDLER: {sql}")

    if sql == "showme":
        sql = "show databases"

    return session.run_sql(sql)

mysqlsh.globals.shell.register_sql_handler("showHandler", "Handler for SHOW statements", ["SHOWME"], my_handler)
"""
testutil.create_file(plugin_path, plugin_code)

# Classic
call_mysqlsh([__mysqluripwd, "-i", "--tabbed", "--py",
             "-e", "session.run_sql('showme')"])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: showme
Database
""")
EXPECT_STDOUT_NOT_CONTAINS("====> SQL HANDLER: show databases")
WIPE_OUTPUT()

# X
call_mysqlsh([__uripwd, "-i", "--tabbed", "--py",
             "-e", "session.run_sql('showme')"])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: showme
Database
""")
EXPECT_STDOUT_NOT_CONTAINS("====> SQL HANDLER: show databases")
WIPE_OUTPUT()

# -e
call_mysqlsh([__uripwd, "-e", "showme"])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: showme
Database
""")
EXPECT_STDOUT_NOT_CONTAINS("====> SQL HANDLER: show databases")
WIPE_OUTPUT()

# -f interactive
testutil.create_file(sql_file_path, "showme;")

call_mysqlsh([__uripwd, "-i", "-f", sql_file_path])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: showme""")
EXPECT_STDOUT_CONTAINS("| Database")
EXPECT_STDOUT_NOT_CONTAINS("====> SQL HANDLER: show databases")
WIPE_OUTPUT()

# -f batch
call_mysqlsh([__uripwd, "-f", sql_file_path])
EXPECT_STDOUT_CONTAINS("""====> SQL HANDLER: showme
Database
""")
EXPECT_STDOUT_NOT_CONTAINS("====> SQL HANDLER: show databases")
WIPE_OUTPUT()

testutil.rmfile(sql_file_path)

#@<> Handlers are disabled of --disable-plugins is used
WIPE_OUTPUT()
call_mysqlsh([__mysqluripwd, "--py", "--disable-plugins", "-i", "--tabbed", "-e", "session.run_sql('showme')"])
EXPECT_STDOUT_CONTAINS('You have an error in your SQL syntax;')
EXPECT_STDOUT_NOT_CONTAINS('====> SQL HANDLER:')


#@<> BUG#37196079 - SQL handler callbacks received SQL query still containing the placeholders
plugin_code = """
def my_handler(session, sql):
    print("====> SQL HANDLER:", sql)

shell.register_sql_handler("placeholdersHandler", "Handler with placeholders", ["SELECT "], my_handler)
"""
testutil.create_file(plugin_path, plugin_code)

call_mysqlsh([__mysqluripwd, "--py", "-e", "session.run_sql('select 1', [])"])
EXPECT_STDOUT_CONTAINS("""
====> SQL HANDLER: select 1
""")
WIPE_OUTPUT()

call_mysqlsh([__mysqluripwd, "--py", "-e", "session.run_sql('select ? from dual', [1])"])
EXPECT_STDOUT_CONTAINS("""
====> SQL HANDLER: select 1 from dual
""")
WIPE_OUTPUT()

call_mysqlsh([__mysqluripwd, "--py", "-e", "session.run_sql('select ?,?, ? from DUAL', ['a', 2, 'c'])"])
EXPECT_STDOUT_CONTAINS("""
====> SQL HANDLER: select 'a',2, 'c' from DUAL
""")
WIPE_OUTPUT()

call_mysqlsh([__mysqluripwd, "--py", "-e", "session.run_sql('select ? from dual', [])"])
EXPECT_STDOUT_CONTAINS("""
ValueError: Insufficient number of values for placeholders in query
""")
WIPE_OUTPUT()


#@<> Finalization
testutil.rmdir(plugins_path, True)
