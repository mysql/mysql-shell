//@<>Initialization
user_name = "test_cli"
schema_name = "bug_36156164"

shell.connect(__mysqluripwd)

session.runSql(`create schema if not exists ${schema_name}`);
session.runSql(`create table if not exists ${schema_name}.t (a int)`);

// create an account which has a limited set of privileges
session.runSql(`create user '${user_name}'@'%' identified by ''`);
session.runSql(`grant lock tables, select on mysql.* to '${user_name}'@'%'`);
session.runSql(`grant select on ${schema_name}.* to '${user_name}'@'%'`);

//@<> CLI error code presence (BUG#36156164)
// a consistent dump will fail because user cannot execute FTWRL, and fall-back
// to LOCK TABLES is not viable, because account cannot lock tables in
// bug_36156164 schema
testutil.callMysqlsh([`${user_name}:@${__host}:${__mysql_port}`, '--save-passwords=never', '--', 'util', 'dump_instance', 'test_out'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("ERROR: MYSQLSH 52002: ")

//@<>CleanUp
session.runSql(`drop user '${user_name}'@'%'`);
session.runSql(`drop schema ${schema_name}`);
