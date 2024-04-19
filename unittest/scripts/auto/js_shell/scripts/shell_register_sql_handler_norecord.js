// TODO(rennox): Review callbacks going out of context in JavaScript

//@<> Initialization
var user_path = testutil.getUserConfigPath()
var plugins_path = os.path.join(user_path, "plugins")
var plugin_folder_path = os.path.join(plugins_path, "cli_tester")
var plugin_path =  os.path.join(plugin_folder_path, "init.js")
testutil.mkdir(plugin_folder_path, true)

function callMysqlsh(command_line_args) {
    testutil.callMysqlsh(command_line_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
}

//@<> Empty list of SQL Handlers
callMysqlsh([__mysqluripwd, "--", "shell", "list-sql-handlers"])
EXPECT_OUTPUT_CONTAINS('[]')

//@<> SQL Handler Registration Errors
EXPECT_THROWS(function() {shell.registerSqlHandler(5, 5, 45, null)}, "Shell.registerSqlHandler: Argument #1 is expected to be a string")
EXPECT_THROWS(function() {shell.registerSqlHandler("", 5, 45, null)}, "Shell.registerSqlHandler: Argument #2 is expected to be a string")
EXPECT_THROWS(function() {shell.registerSqlHandler("", "", 45, null)}, "Shell.registerSqlHandler: Argument #3 is expected to be an array")
EXPECT_THROWS(function() {shell.registerSqlHandler("sample", "description", ["SAMPLE"], 45)}, "Shell.registerSqlHandler: Argument #4 is expected to be a function")
EXPECT_THROWS(function() {shell.registerSqlHandler("", "", [], null)}, "Shell.registerSqlHandler: An empty name is not allowed")
EXPECT_THROWS(function() {shell.registerSqlHandler("sample", "", [], null)}, "Shell.registerSqlHandler: An empty description is not allowed")
EXPECT_THROWS(function() {shell.registerSqlHandler("sample", "description", [], null)}, "Shell.registerSqlHandler: At least one prefix must be specified")
EXPECT_THROWS(function() {shell.registerSqlHandler("sample", "description", [45], null)}, "Shell.registerSqlHandler: Argument #3 is expected to be an array of strings")
EXPECT_THROWS(function() {shell.registerSqlHandler("sample", "description", [""], null)}, "Shell.registerSqlHandler: Empty or blank prefixes are not allowed")
EXPECT_THROWS(function() {shell.registerSqlHandler("sample", "description", ["SAMPLE", "   "], null)}, "Shell.registerSqlHandler: Empty or blank prefixes are not allowed")
EXPECT_THROWS(function() {shell.registerSqlHandler("sample", "description", ["SAMPLE"], null)}, "The callback function must be defined.")

// Error registering handler with same name
function one() {}

shell.registerSqlHandler("jsOne", "one description", ["jsOne", "some other"], one)
EXPECT_THROWS(function() { shell.registerSqlHandler("jsOne", "one description", ["  one", "some other"], one)}, "An SQL Handler named 'jsOne' already exists.")
EXPECT_THROWS(function() { shell.registerSqlHandler("two", "one description", ["JSONE"], one)}, "The prefix 'JSONE' is shadowed by an existing prefix defined at the 'jsOne' SQL Handler")
EXPECT_THROWS(function() { shell.registerSqlHandler("two", "one description", ["jsone piece"], one)}, "The prefix 'jsone piece' is shadowed by an existing prefix defined at the 'jsOne' SQL Handler")
EXPECT_THROWS(function() { shell.registerSqlHandler("two", "two", ["some"], one)}, "The prefix 'some' will shadow an existing prefix defined at the 'jsOne' SQL Handler")


//@<> SQL Handler Runtime Errors (function does not met expected syntax)
plugin_code = `
function invalidReturnData(session, sql) {
    if (sql == "my bad callback return number"){
        return 45;
    }
    if (sql == "my bad callback return session"){
        return session;
    }
}

shell.registerSqlHandler("sample1", "description", ["my bad callback return"], invalidReturnData)
`

testutil.createFile(plugin_path, plugin_code)

callMysqlsh([__mysqluripwd, "--sql", "-e", "my bad callback return number"])
EXPECT_STDOUT_CONTAINS(`Invalid data returned by the SQL handler, if any, the returned data should be a result object.`)
WIPE_OUTPUT()

callMysqlsh([__mysqluripwd, "--sql", "-e", "my bad callback return session"])
EXPECT_STDOUT_CONTAINS(`Invalid data returned by the SQL handler, if any, the returned data should be a result object.`)
WIPE_OUTPUT()


//@<> Simple logging SQL handler
plugin_code = `
function sql_show_logger(session, sql) {
    println("====> SQL LOGGER:",sql);
}

shell.registerSqlHandler("showHandler", "Prints SHOW Statements", ["SHOW Databases", "show tables"], sql_show_logger)
`

testutil.createFile(plugin_path, plugin_code)

callMysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS(`====> SQL LOGGER: show databases
Database`)
WIPE_OUTPUT()

callMysqlsh([__mysqluripwd, "--sql", "-e", "show tables from mysql"])
EXPECT_STDOUT_CONTAINS(`====> SQL LOGGER: show tables from mysql
Tables_in_mysql`)
WIPE_OUTPUT()

callMysqlsh([__mysqluripwd, "--sql", "-e", "SHOW PLUGINS"])
EXPECT_STDOUT_NOT_CONTAINS("====> SQL LOGGER:")
WIPE_OUTPUT()


//@<> Multiple SQL Handlers, executed in registration order
plugin_code = `
function first_handler(session, sql) {
    println("====> FIRST HANDLER:", sql)

    // On this specific case will return a result
    if (sql.trim() == "show databases tweaked") {
        return shell.createResult()
    }
}

function second_handler(session, sql) {
    println("====> SECOND HANDLER:", sql)
}

shell.registerSqlHandler("databaseShow", "Handler for SHOW DATABASE", ["SHOW DATABASES"], first_handler)
shell.registerSqlHandler("tableShow", "Handler for SHOW TABLES", ["SHOW TABLES"], second_handler)
`
testutil.createFile(plugin_path, plugin_code)

callMysqlsh([__mysqluripwd, "--", "shell", "list-sql-handlers"])
EXPECT_OUTPUT_CONTAINS_MULTILINE(`
[
    {
        "description": "Handler for SHOW DATABASE", 
        "name": "databaseShow"
    }, 
    {
        "description": "Handler for SHOW TABLES", 
        "name": "tableShow"
    }
]`)

callMysqlsh([__mysqluripwd, "--sql", "-e", "show databases"])
EXPECT_STDOUT_CONTAINS(`====> FIRST HANDLER: show databases
Database`)
EXPECT_STDOUT_NOT_CONTAINS("====> THIRD HANDLER:")
WIPE_OUTPUT()

callMysqlsh([__mysqluripwd, "--sql", "-e", "show tables from mysql"])
EXPECT_STDOUT_CONTAINS(`====> SECOND HANDLER: show tables from mysql
Tables_in_mysql`)
EXPECT_STDOUT_NOT_CONTAINS("====> FIRST HANDLER:")
WIPE_OUTPUT()

callMysqlsh([__mysqluripwd, "--js", "-i", "-e", "session.runSql('show databases tweaked')"])
EXPECT_STDOUT_CONTAINS(`====> FIRST HANDLER: show databases tweaked
Query OK, 0 rows affected (0.0000 sec)`)

// Since processinig is aborted neither the otherhandler or the shell session process the sql
EXPECT_STDOUT_NOT_CONTAINS(`====> SECOND HANDLER:`)
WIPE_OUTPUT()

//@<> Execution of nested extended SQL is forbidden
plugin_code = `

function my_handler(session, sql) {
    println("====> SQL HANDLER:", sql)

    if (sql == "showme") {
        sql = "showme databases"
    }

    return session.runSql(sql)
}

shell.registerSqlHandler("showHandler", "Handler for SHOW statements", ["SHOWME"], my_handler)
`
testutil.createFile(plugin_path, plugin_code)

callMysqlsh([__mysqluripwd, "--js", "-i", "--tabbed", "-e", "session.runSql('showme')"])
EXPECT_STDOUT_CONTAINS_MULTILINE(`ClassicSession.runSql: ClassicSession.runSql: Unable to execute a sql handler while another is being executed.
Executing SQL: showme
Unable to execute: showme databases (LogicError)`)

//@<> Execution of standard SQL while executing extended SQL is allowed
plugin_code = `

function my_handler(session, sql) {
    println("====> SQL HANDLER:", sql)

    if (sql == "showme") {
        sql = "show databases"
    }

    return session.runSql(sql)
}

shell.registerSqlHandler("showHandler", "Handler for SHOW statements", ["SHOWME"], my_handler)
`
testutil.createFile(plugin_path, plugin_code)

// Classic
callMysqlsh([__mysqluripwd, "--js", "-i", "--tabbed", "-e", "session.runSql('showme')"])
EXPECT_STDOUT_CONTAINS_MULTILINE(`====> SQL HANDLER: showme
Database`)
WIPE_OUTPUT()

// X
callMysqlsh([__uripwd, "--js", "-i", "--tabbed", "-e", "session.runSql('showme')"])
EXPECT_STDOUT_CONTAINS_MULTILINE(`====> SQL HANDLER: showme
Database`)
WIPE_OUTPUT()

//@<> Handlers are disabled of --disable-plugins is used
WIPE_OUTPUT()
callMysqlsh([__mysqluripwd, "--js", "--disable-plugins", "-i", "--tabbed", "-e", "session.runSql('showme')"])
EXPECT_STDOUT_CONTAINS("You have an error in your SQL syntax;")
EXPECT_STDOUT_NOT_CONTAINS(`====> SQL HANDLER:`)

//@<> Finalization
testutil.rmdir(plugins_path, true)

