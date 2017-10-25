// Assumptions: smart deployment rountines available

//@ Check Instance Configuration must work without a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, password:'root'});

//@ First Sandbox
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@ Check Instance Configuration ok with a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, password:'root'});

//@ Error: user has no privileges to run the checkInstanceConfiguration command (BUG#26609909)
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
// Create account with all privileges but remove one of the necessary privileges
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'test_user'@'%'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'test_user'@'%' WITH GRANT OPTION");
session.runSql("REVOKE SELECT on *.* FROM 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user: 'test_user', password:''});
session.close()

//@ Check instance configuration using a non existing user that authenticates as another user that does not have enough privileges (BUG#26979375)
shell.connect({scheme:'mysql', host: hostname, port: __mysql_sandbox_port1, user: 'test_user', password: ''});
dba.checkInstanceConfiguration({host: hostname, port: __mysql_sandbox_port1, user: 'test_user', password:''});
session.close()

//@ Check instance configuration using a non existing user that authenticates as another user that has all privileges (BUG#26979375)
// Grant the missing privilege again
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET sql_log_bin = 0");
session.runSql("GRANT SELECT on *.* TO 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close()

shell.connect({scheme:'mysql', host: hostname, port: __mysql_sandbox_port1, user: 'test_user', password: ''});
dba.checkInstanceConfiguration({host: hostname, port: __mysql_sandbox_port1, user: 'test_user', password:''});
session.close()

// Drop user
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP user 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");

session.close();

// Remove the sandbox
testutil.destroySandbox(__mysql_sandbox_port1);
