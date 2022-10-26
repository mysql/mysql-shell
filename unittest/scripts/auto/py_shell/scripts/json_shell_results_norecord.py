#@<> Preparation
import json
import base64

shell.connect(__mysqluripwd)


session.run_sql("drop schema if exists json_shell")
session.run_sql("create schema json_shell")
session.run_sql("create table json_shell.sample(id int, data varchar(30))")
session.run_sql("insert into json_shell.sample values (1, ?)", ["john doe"])
session.run_sql("insert into json_shell.sample values (2, ?)", ["jane doe"])

def mysqlsh(args):
    testutil.call_mysqlsh(["--interactive=full","--quiet-start=2"] + args, "", ["MYSQLSH_JSON_SHELL=1", "MYSQLSH_TERM_COLOR_MODE=nocolor"])


#@<> Validating result from SQL execution in SQL mode
mysqlsh(["--sql", "-e", '{"execute":"SELECT * FROM json_shell.sample"}', __mysqluripwd])
EXPECT_STDOUT_CONTAINS('{"Field 1":{"Name":"`id`"')
EXPECT_STDOUT_CONTAINS(',"Field 2":{"Name":"`data`"')
EXPECT_STDOUT_CONTAINS('{"hasData":true,"rows":[{"id":1,"data":"john doe"},{"id":2,"data":"jane doe"}]')

#@<> Validating result from SQL execution through the session object JS
mysqlsh(["-e", '{"execute":"session.runSql(\'SELECT * FROM json_shell.sample\');"}', __mysqluripwd])
EXPECT_STDOUT_CONTAINS('{"Field 1":{"Name":"`id`"')
EXPECT_STDOUT_CONTAINS(',"Field 2":{"Name":"`data`"')
EXPECT_STDOUT_CONTAINS('{"hasData":true,"rows":[{"id":1,"data":"john doe"},{"id":2,"data":"jane doe"}]')

#@<> Validating result from SQL execution through the session object PY
mysqlsh(["--py", "-e", '{"execute":"session.run_sql(\'SELECT * FROM json_shell.sample\');"}', __mysqluripwd])
EXPECT_STDOUT_CONTAINS('{"Field 1":{"Name":"`id`"')
EXPECT_STDOUT_CONTAINS(',"Field 2":{"Name":"`data`"')
EXPECT_STDOUT_CONTAINS('{"hasData":true,"rows":[{"id":1,"data":"john doe"},{"id":2,"data":"jane doe"}]')

#@<> Validating result from shell dumpRows
mysqlsh(["-e", '{"execute":"let res = session.runSql(\'SELECT * FROM json_shell.sample\'); shell.dumpRows(res);"}', __mysqluripwd])
EXPECT_STDOUT_CONTAINS('{"Field 1":{"Name":"`id`"')
EXPECT_STDOUT_CONTAINS(',"Field 2":{"Name":"`data`"')
EXPECT_STDOUT_CONTAINS('{"hasData":true,"rows":[{"id":1,"data":"john doe"},{"id":2,"data":"jane doe"}]')


#@<> Validating result from \show report
mysqlsh(["-e", '{"execute":"\\\\show threads"}', __mysqluripwd])
EXPECT_STDOUT_CONTAINS('{"Field 1":{"Name":"`tid`"')
EXPECT_STDOUT_CONTAINS(',"Field 2":{"Name":"`cid`"')
EXPECT_STDOUT_CONTAINS(',"Field 3":{"Name":"`user`"')
EXPECT_STDOUT_CONTAINS('{"hasData":true,"rows":[{"tid"')


#@<> Validating result from SQL in json/raw format
# NOTE: The result data is JSON wrapped inside JSON so it is properly processed by the GUI
mysqlsh(["--result-format=json/raw", "--sql", "-e", '{"execute":"SELECT * FROM json_shell.sample"}', __mysqluripwd])
EXPECT_STDOUT_NOT_CONTAINS('{"Field 1":{"Name":"`id`"')
EXPECT_STDOUT_CONTAINS('{"info":"{\\\"hasData\\\":true,\\\"rows\\\":[{\\\"id\\\":1,\\\"data\\\":\\\"john doe\\\"}')


#@<> Validating result from SQL in json/raw format, including column type info
# NOTE: The column metadata and the result data is JSON wrapped inside JSON so it is properly processed by the GUI
mysqlsh(["--column-type-info", "--result-format=json/raw", "--sql", "-e", '{"execute":"SELECT * FROM json_shell.sample"}', __mysqluripwd])
EXPECT_STDOUT_CONTAINS('{"info":"{\\\"Field 1\\\":{\\\"Name\\\":\\\"`id`\\\"')
EXPECT_STDOUT_CONTAINS('{"info":"{\\\"hasData\\\":true,\\\"rows\\\":[{\\\"id\\\":1,\\\"data\\\":\\\"john doe\\\"}')

#@<> Cleanup
session.run_sql("DROP SCHEMA json_shell")
shell.disconnect()