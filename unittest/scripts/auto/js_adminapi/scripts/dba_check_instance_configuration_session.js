// Assumptions: smart deployment routines available

//@ Check Instance Configuration must work without a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user:'root', password:'root'});

//@ Check Instance Configuration should fail if there's no session nor parameters provided
dba.checkInstanceConfiguration();

//@<> First Sandbox
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@<> Deploy 2nd sandbox with invalid (empty) report_host value.
// Regression for BUG#28285389: TARGET MEMBER HOSTNAME SHOULD BE TAKEN FROM HOSTNAME OR REPORT_HOST SYSVARS
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": ""});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@ Check Instance Configuration ok with a session
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user:'root', password:'root'});

//@<> Create account with all privileges but remove one of the necessary privileges
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'test_user'@'%'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'test_user'@'%' WITH GRANT OPTION");
session.runSql("REVOKE SELECT on *.* FROM 'test_user'@'%'");
//NOTE: SELECT privileges required to access the metadata (version), otherwise another error is issued.
session.runSql("GRANT SELECT ON `mysql_innodb_cluster_metadata`.* TO 'test_user'@'%'");

// NOTE: This privilege is required or the the generated error will be:
//Dba.checkInstanceConfiguration: Unable to detect target instance state. Please check account privileges.
//Dba.checkInstanceConfiguration: Unable to detect state for instance 'me7nw6-mac:3316'. Please check account privileges.
session.runSql("GRANT SELECT ON `performance_schema`.* TO 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");

//TODO: Improve error message (more details for missing metadata privileges) and add test here for this error message.

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
//NOTE: SELECT privileges required to access the metadata (version), otherwise another error is issued.
session.runSql("GRANT SELECT ON `mysql_innodb_cluster_metadata`.* TO 'no_privileges'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_group_members` TO 'no_privileges'@'%'");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'no_privileges'@'%' WITH GRANT OPTION;");
session.runSql("GRANT SELECT ON `performance_schema`.`threads` TO 'no_privileges'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_connection_configuration` TO 'no_privileges'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_connection_status` TO 'no_privileges'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_applier_status` TO 'no_privileges'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_applier_status_by_worker` TO 'no_privileges'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_applier_status_by_coordinator` TO 'no_privileges'@'%'");

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
session.runSql("CREATE USER 'admin_test'@'%' IDENTIFIED BY 'adminpass'");
session.runSql("GRANT 'root'@'%' TO 'admin_test'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_group_members` TO 'admin_test'@'%'");
session.runSql("SET DEFAULT ROLE 'root'@'%' to 'admin_test'@'%'");
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
session.runSql("REVOKE 'root'@'%' FROM 'admin_test'@'%';");
// Create admin_role with missing privileges and grant it to admin_test user
session.runSql("CREATE ROLE 'admin_role'@'%'");
session.runSql("GRANT ALL ON mysql.* TO 'admin_role'@'%'");
session.runSql("GRANT ALL ON sys.* TO 'admin_role'@'%'");
session.runSql("GRANT SELECT ON `mysql_innodb_cluster_metadata`.* TO 'admin_role'@'%'");
session.runSql("GRANT 'admin_role'@'%' TO 'admin_test'@'%'");
session.runSql("SET DEFAULT ROLE 'admin_role'@'%' to 'admin_test'@'%'");
session.runSql("SET sql_log_bin = 1");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'admin_test'@'%' WITH GRANT OPTION;");
session.runSql("GRANT SELECT ON `performance_schema`.`threads` TO 'admin_test'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_connection_configuration` TO 'admin_test'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_connection_status` TO 'admin_test'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_applier_status` TO 'admin_test'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_applier_status_by_worker` TO 'admin_test'@'%'");
session.runSql("GRANT SELECT ON `performance_schema`.`replication_applier_status_by_coordinator` TO 'admin_test'@'%'");

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
session.runSql("DROP USER 'admin_test'@'%'");
session.runSql("DROP ROLE 'admin_role'@'%'");
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

// Regression test for BUG#25867733: CHECKINSTANCECONFIGURATION SUCCESS BUT CREATE CLUSTER FAILING IF PFS DISABLED
//@ Deploy instances (setting performance_schema value).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"performance_schema": "off", "report_host": hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
// ensure performance_schema=on, which is the default
EXPECT_EQ(mysql.getSession("root:root@localhost:"+__mysql_sandbox_port2).runSql("select @@performance_schema").fetchOne()[0], 1);

//@ checkInstanceConfiguration error with performance_schema=off
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user:'root', password:'root'});

// checkInstanceConfiguration no error with performance_schema=on
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port2, user:'root', password:'root'});

//@ Remove the sandboxes (final)
testutil.destroySandbox(__mysql_sandbox_port1);

//Regression test for BUG#29018457: CHECKINSTANCECONFIGURATION() DOES NOT VALIDATE IF THE ACCOUNT EXISTS ON THE HOST
//@ BUG29018457 - Deploy instance.
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname});

//@ BUG29018457 - Create a admin_local user only for localhost.
shell.connect(__sandbox_uri1);
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'admin_local'@'localhost' IDENTIFIED BY 'local_pass'");
session.runSql("GRANT ALL PRIVILEGES ON *.* TO 'admin_local'@'localhost' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");
session.close();

//@<> BUG29018457 - check instance fails because only a localhost account is available.
dba.checkInstanceConfiguration({host: localhost, port: __mysql_sandbox_port1, user:'admin_local', password:'local_pass'});

//@ BUG29018457 - Create a admin_host user only for report_host.
shell.connect(__sandbox_uri1);
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'admin_host'@'" + hostname_ip + "' IDENTIFIED BY 'host_pass'");
session.runSql("GRANT ALL PRIVILEGES ON *.* TO 'admin_host'@'" + hostname_ip + "' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");
session.close();

//@<> BUG29018457 - check instance fails because only report_host account is available.
dba.checkInstanceConfiguration({host: hostname_ip, port: __mysql_sandbox_port1, user:'admin_host', password:'host_pass'});

//@ BUG29018457 - clean-up (destroy sandbox).
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
