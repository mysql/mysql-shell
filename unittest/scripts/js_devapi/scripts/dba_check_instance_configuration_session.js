// Assumptions: smart deployment routines available

//@ Check Instance Configuration must work without a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user:'root', password:'root'});

//@ Check Instance Configuration should fail if there's no session nor parameters provided
dba.checkInstanceConfiguration();

//@ First Sandbox
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@ Deploy 2nd sandbox with invalid (empty) report_host value.
// Regression for BUG#28285389: TARGET MEMBER HOSTNAME SHOULD BE TAKEN FROM HOSTNAME OR REPORT_HOST SYSVARS
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": ""});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@ Check Instance Configuration ok with a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user:'root', password:'root'});

//@ Create account with all privileges but remove one of the necessary privileges
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'test_user'@'%'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'test_user'@'%' WITH GRANT OPTION");
session.runSql("REVOKE SELECT on *.* FROM 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");

//@ Error: user has no privileges to run the checkInstanceConfiguration command (BUG#26609909)
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user: 'test_user', password:''});
session.close();

//@ Check instance configuration using a non existing user that authenticates as another user that does not have enough privileges (BUG#26979375)
shell.connect({scheme:'mysql', host: hostname, port: __mysql_sandbox_port1, user: 'test_user', password: ''});
dba.checkInstanceConfiguration({host: hostname, port: __mysql_sandbox_port1, user: 'test_user', password:''});

//@ Check instance configuration using a non existing user that authenticates as another user that has all privileges (BUG#26979375)
// Grant the missing privilege again
session.close();
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET sql_log_bin = 0");
session.runSql("GRANT SELECT on *.* TO 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

shell.connect({scheme:'mysql', host: hostname, port: __mysql_sandbox_port1, user: 'test_user', password: ''});
dba.checkInstanceConfiguration({host: hostname, port: __mysql_sandbox_port1, user: 'test_user', password:''});
session.close();

// Drop user
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP user 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");

session.close();

//@ Check if all missing privileges are reported for user with no privileges
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

// Create account with no privileges
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'no_privileges'@'%'");
session.runSql("SET sql_log_bin = 1");

// Test
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user: 'no_privileges', password:''});

// Drop user
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP user 'no_privileges'@'%'");
session.runSql("SET sql_log_bin = 1");

session.close();

//@ Create a user with the root role {VER(>=8.0.0)}
// Regression for BUG#28236922: AdminAPI does not recognize privileges if assigned with a role.
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'admin_test'@'localhost' IDENTIFIED BY 'adminpass'");
session.runSql("GRANT 'root'@'localhost' TO 'admin_test'@'localhost'");
session.runSql("SET DEFAULT ROLE 'root'@'localhost' to 'admin_test'@'localhost'");
session.runSql("SET sql_log_bin = 1");

//@ Check instance using user with a root role as parameter {VER(>=8.0.0)}
// Regression for BUG#28236922: AdminAPI does not recognize privileges if assigned with a role.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user: 'admin_test', password:'adminpass'});

//@ Connect using admin user {VER(>=8.0.0)}
// Regression for BUG#28236922: AdminAPI does not recognize privileges if assigned with a role.
session.close();
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'admin_test', password: 'adminpass'});

//@ Check instance using session user with a root role {VER(>=8.0.0)}
// Regression for BUG#28236922: AdminAPI does not recognize privileges if assigned with a role.
dba.checkInstanceConfiguration();

//@ Create role with missing privileges and grant to admin user {VER(>=8.0.0)}
// Regression for BUG#28236922: AdminAPI does not recognize privileges if assigned with a role.
session.close();
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET sql_log_bin = 0");
// Remove previous root role from admin_test user
session.runSql("REVOKE 'root'@'localhost' FROM 'admin_test'@'localhost';");
// Create admin_role with missing privileges and grant it to admin_test user
session.runSql("CREATE ROLE 'admin_role'@'localhost'");
session.runSql("GRANT ALL ON mysql.* TO 'admin_role'@'localhost'");
session.runSql("GRANT ALL ON sys.* TO 'admin_role'@'localhost'");
session.runSql("GRANT 'admin_role'@'localhost' TO 'admin_test'@'localhost'");
session.runSql("SET DEFAULT ROLE 'admin_role'@'localhost' to 'admin_test'@'localhost'");
session.runSql("SET sql_log_bin = 1");

//@ Check instance using user with admin role as parameter, missing privileges {VER(>=8.0.0)}
// Regression for BUG#28236922: AdminAPI does not recognize privileges if assigned with a role.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user: 'admin_test', password:'adminpass'});

//@ Connect using admin user again {VER(>=8.0.0)}
// Regression for BUG#28236922: AdminAPI does not recognize privileges if assigned with a role.
session.close();
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'admin_test', password: 'adminpass'});

//@ Check instance using session user with admin role, missing privileges {VER(>=8.0.0)}
// Regression for BUG#28236922: AdminAPI does not recognize privileges if assigned with a role.
dba.checkInstanceConfiguration();

//@ Drop user with role {VER(>=8.0.0)}
// Regression for BUG#28236922: AdminAPI does not recognize privileges if assigned with a role.
session.close();
shell.connect({host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP USER 'admin_test'@'localhost'");
session.runSql("DROP ROLE 'admin_role'@'localhost'");
session.runSql("SET sql_log_bin = 1");
session.close();

//@ Check instance must fail if report_host is defined but empty.
// Regression for BUG#28285389: TARGET MEMBER HOSTNAME SHOULD BE TAKEN FROM HOSTNAME OR REPORT_HOST SYSVARS
shell.connect({host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
dba.checkInstanceConfiguration();

//@ Configure instance must fail if report_host is defined but empty.
// Regression for BUG#28285389: TARGET MEMBER HOSTNAME SHOULD BE TAKEN FROM HOSTNAME OR REPORT_HOST SYSVARS
dba.configureInstance();

//@ Create cluster must fail if report_host is defined but empty.
// Regression for BUG#28285389: TARGET MEMBER HOSTNAME SHOULD BE TAKEN FROM HOSTNAME OR REPORT_HOST SYSVARS
dba.createCluster('error');

//@ Remove the sandboxes
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// Regression test for BUG#25867733: CHECKINSTANCECONFIGURATION SUCCESS BUT CREATE CLUSTER FAILING IF PFS DISABLED
//@ Deploy instances (setting performance_schema value).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"performance_schema": "off"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"performance_schema": "on"});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@ checkInstanceConfiguration error with performance_schema=off
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user:'root', password:'root'});

// checkInstanceConfiguration no error with performance_schema=on
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port2, user:'root', password:'root'});

//@ Remove the sandboxes (final)
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
