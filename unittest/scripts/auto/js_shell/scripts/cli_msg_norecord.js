//@<>Initialization
shell.connect(__mysqluripwd)
session.runSql(`create user 'test_cli'@'%' identified by '';`);
session.runSql(`grant select on mysql.* to 'test_cli'@'%';`);

//@<> CLI error code presence (BUG#36156164)
testutil.callMysqlsh(['test_cli:@'+__host+':'+__mysql_port, '--save-passwords=never', '--', 'util', 'dump_instance', 'test_out'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("ERROR: MYSQLSH 52002: ")

//@<>CleanUp
session.runSql(`drop user 'test_cli'@'%';`);
