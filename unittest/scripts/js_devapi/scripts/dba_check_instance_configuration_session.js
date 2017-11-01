// Assumptions: smart deployment routines available

//@ Check Instance Configuration must work without a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, password:'root'});

//@ First Sandbox
var deployed_here = reset_or_deploy_sandbox(__mysql_sandbox_port1);

//@ Check Instance Configuration ok with a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, password:'root'});

//@ Error: user has no privileges to run the checkInstanceConfiguration command (BUG#26609909)
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
// Create account with all privileges but remove one of the necessary privileges
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'test_user'@'%'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'test_user'@'%' WITH GRANT OPTION");
session.runSql("REVOKE SELECT on *.* FROM 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user: 'test_user', password:''});
// Drop user
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP user 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");

session.close();

// Remove the sandbox
if (deployed_here)
	cleanup_sandbox(__mysql_sandbox_port1);
