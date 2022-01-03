#@<> Setup test
shell.connect(__mysql_uri)
\option logSql = "unfiltered"

testutil.create_file("multi.sql", "select 1; select 2; SELECT 3; select 4; select 5; select 6;\nselect 'a'; select 'b';\nselect 'c'; select 'd'; select 'e';\n")

#@<> Multi SQL statement
WIPE_SHELL_LOG()
\sql
\. multi.sql
\py

EXPECT_SHELL_LOG_CONTAINS_COUNT("select 1", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("select 2", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("SELECT 3", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("select 4", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("select 5", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("select 6", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("select 'a'", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("select 'b'", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("select 'c'", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("select 'd'", 1)
EXPECT_SHELL_LOG_CONTAINS_COUNT("select 'e'", 1)

#@<> Context origin
WIPE_SHELL_LOG()
mysql.get_session(__mysql_uri).run_sql("select 1 /* ClassicSession.run_sql */;")
EXPECT_SHELL_LOG_CONTAINS("Info: ClassicSession.run_sql: tid=")

#@<> Clean-up
testutil.rmfile("multi.sql")
\option logSql = "error"
session.close()
