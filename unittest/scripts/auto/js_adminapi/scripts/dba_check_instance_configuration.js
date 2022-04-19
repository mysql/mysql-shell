// BUG#29305551: ADMINAPI FAILS TO DETECT INSTANCE IS RUNNING ASYNCHRONOUS REPLICATION
//
// dba.checkInstance() reports that a target instance which is running the Slave
// SQL and IO threads is valid to be used in an InnoDB cluster.
//
// As a consequence, the AdminAPI fails to detects that an instance has
// asynchronous replication running and both addInstance() and rejoinInstance()
// fail with useless/unfriendly errors on this scenario. There's not even
// information on how to overcome the issue.
//
// dba.checkInstanceConfiguration() shall report a warning if asynchronous
// replication is running on the target instance

//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

//@<> check if warning is returned for tables without PK BUG#29771457
shell.connect(__sandbox_uri1);
session.runSql("CREATE DATABASE test");
session.runSql("CREATE TABLE test.foo (i int)");
var resp = dba.checkInstanceConfiguration();
session.runSql("DROP DATABASE test");
EXPECT_EQ({"status":"error"}, resp)

EXPECT_STDOUT_CONTAINS("Group Replication requires tables to use InnoDB and have a PRIMARY KEY or PRIMARY KEY Equivalent (non-null unique key). Tables that do not follow these requirements will be readable but not updateable when used with Group Replication. If your applications make updates (INSERT, UPDATE or DELETE) to these tables, ensure they use the InnoDB storage engine and have a PRIMARY KEY or PRIMARY KEY Equivalent.")
EXPECT_STDOUT_CONTAINS("If you can't change the tables structure to include an extra visible key to be used as PRIMARY KEY, you can make use of the INVISIBLE COLUMN feature available since 8.0.23: https://dev.mysql.com/doc/refman/8.0/en/invisible-columns.html")

//@<> BUG#32287986: Create account without all grants
shell.connect(__sandbox_uri1);
session.runSql("CREATE USER 'dba'@'%'");
session.runSql("GRANT SELECT ON *.* TO 'dba'@'%'");
session.close()

//@<> BUG#32287986: Verify the list of missing grants displayed by checkInstanceConfiguration()
var __dba_uri = "mysql://dba:@localhost:"+__mysql_sandbox_port1;

//@<> BUG#32287986: Set the list of grants {VER(>=8.0.0)}
var __grant1 = "GRANT CLONE_ADMIN, CONNECTION_ADMIN, CREATE USER, EXECUTE, FILE, GROUP_REPLICATION_ADMIN, PERSIST_RO_VARIABLES_ADMIN, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, REPLICATION_APPLIER, REPLICATION_SLAVE_ADMIN, ROLE_ADMIN, SHUTDOWN, SYSTEM_VARIABLES_ADMIN ON *.* TO 'dba'@'%' WITH GRANT OPTION;";
var __grant2 = "GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'dba'@'%' WITH GRANT OPTION;";
var __grant3 = "GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'dba'@'%' WITH GRANT OPTION;";
var __grant4 = "GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'dba'@'%' WITH GRANT OPTION;";
var __grant5 = "GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'dba'@'%' WITH GRANT OPTION;";

//@<> BUG#32287986: Set the list of grants {VER(>=5.7.0) && VER(<8.0.0)}
var __grant1 = "GRANT CREATE USER, FILE, PROCESS, RELOAD, REPLICATION CLIENT, REPLICATION SLAVE, SHUTDOWN, SUPER ON *.* TO 'dba'@'%' WITH GRANT OPTION;";
var __grant2 = "GRANT DELETE, INSERT, UPDATE ON mysql.* TO 'dba'@'%' WITH GRANT OPTION;";
var __grant3 = "GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata.* TO 'dba'@'%' WITH GRANT OPTION;";
var __grant4 = "GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_bkp.* TO 'dba'@'%' WITH GRANT OPTION;";
var __grant5 = "GRANT ALTER, ALTER ROUTINE, CREATE, CREATE ROUTINE, CREATE TEMPORARY TABLES, CREATE VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK TABLES, REFERENCES, SHOW VIEW, TRIGGER, UPDATE ON mysql_innodb_cluster_metadata_previous.* TO 'dba'@'%' WITH GRANT OPTION;";

//@<> BUG#32287986: Run checkInstanceConfiguration()
EXPECT_THROWS(function(){dba.checkInstanceConfiguration(__dba_uri)}, "The account 'dba'@'%' is missing privileges required to manage an InnoDB cluster.");
EXPECT_STDOUT_CONTAINS(__grant1);
EXPECT_STDOUT_CONTAINS(__grant2);
EXPECT_STDOUT_CONTAINS(__grant3);
EXPECT_STDOUT_CONTAINS(__grant4);
EXPECT_STDOUT_CONTAINS(__grant5);

//@<> BUG#32287986: Grant account the grants as indicated by the cmd
shell.connect(__sandbox_uri1);
session.runSql(__grant1);
session.runSql(__grant2);
session.runSql(__grant3);
session.runSql(__grant4);
session.runSql(__grant5);
session.close();

EXPECT_NO_THROWS(function(){dba.checkInstanceConfiguration(__dba_uri)});

//@<> BUG#29305551: Setup asynchronous replication
shell.connect(__sandbox_uri1);

// Create Replication user
session.runSql("CREATE USER 'repl'@'%' IDENTIFIED BY 'password' REQUIRE SSL");
session.runSql("GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';");

// Set async channel on instance2
session.close();
shell.connect(__sandbox_uri2);

session.runSql("CHANGE MASTER TO MASTER_HOST='" + hostname + "', MASTER_PORT=" + __mysql_sandbox_port1 + ", MASTER_USER='repl', MASTER_PASSWORD='password', MASTER_AUTO_POSITION=1, MASTER_SSL=1");
session.runSql("START SLAVE");

//@ Check instance configuration async replication warning
dba.checkInstanceConfiguration(__sandbox_uri2);

// BUG#32197197: ADMINAPI DOES NOT PROPERLY CHECK FOR PRECONFIGURED REPLICATION CHANNELS
//
// Even if replication is not running but configured, the warning/error has to
// be provided as implemented in BUG#29305551
session.runSql("STOP SLAVE");

//@ Check instance configuration async replication warning with channels stopped
dba.checkInstanceConfiguration(__sandbox_uri2);

//@<> BUG#31468546: invalid path specified as mycnfPath
dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: "/this/path/is/invalid"});
EXPECT_STDOUT_CONTAINS("Configuration file /this/path/is/invalid doesn't exist.");

//@<> BUG#29305551: Finalization
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

//@<> Deploy sandbox WL#12758 IPv6 {VER(>= 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@<OUT> canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
dba.checkInstanceConfiguration(__sandbox_uri1);

//@<> Cleanup WL#12758 IPv6 {VER(>= 8.0.14)}
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Deploy sandbox WL#12758 IPv4
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "127.0.0.1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@<OUT> canonical IPv4 addresses are supported WL#12758
dba.checkInstanceConfiguration(__sandbox_uri1);

//@<OUT> canonical IPv4 addresses are supported (using IPv6 connection data) WL#12758
var ipv6_sandbox1_uri = "mysql://root:root@[::1]:"+__mysql_sandbox_port1;
dba.checkInstanceConfiguration(ipv6_sandbox1_uri);

//@<> Cleanup WL#12758 IPv4
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Initialization IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: "::1"});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

//@ IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
dba.checkInstanceConfiguration(__sandbox_uri1);

//@<> Cleanup IPv6 not supported on versions below 8.0.14 WL#12758 {VER(< 8.0.14)}
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> dba.checkInstanceConfiguration does not error if using a single clusterAdmin account with netmask BUG#31018091
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);
var ip_mask = hostname_ip.split(".").splice(0,3).join(".") + ".0";
dba.configureInstance(__sandbox_uri1, {clusterAdmin: "'admin'@'"+ ip_mask + "/255.255.255.0'", clusterAdminPassword: "pwd"});
var cluster_admin_uri= "mysql://admin:pwd@" + hostname_ip + ":" + __mysql_sandbox_port1;
shell.connect(cluster_admin_uri);
c = dba.checkInstanceConfiguration();
EXPECT_STDOUT_CONTAINS("Instance configuration is compatible with InnoDB cluster");
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> dba.checkInstanceConfiguration does not complain if server uses binlog_checksum BUG#31329024 {VER(>= 8.0.21)}
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, binlog_checksum: "CRC32"});
shell.connect(__sandbox_uri1);
c = dba.checkInstanceConfiguration();
EXPECT_STDOUT_CONTAINS("Instance configuration is compatible with InnoDB cluster");

//@<OUT> dba.checkInstanceConfiguration() must validate if parallel-appliers are enabled or not {VER(>= 8.0.23)}
session.runSql("SET GLOBAL binlog_transaction_dependency_tracking=COMMIT_ORDER");
session.runSql("SET GLOBAL slave_preserve_commit_order=OFF");
session.runSql("SET GLOBAL slave_parallel_type='DATABASE'");
session.runSql("SET GLOBAL transaction_write_set_extraction=OFF");
dba.checkInstanceConfiguration();

//@<OUT> dba.checkInstanceConfiguration() must validate if group_replication_tls_source is set to the default (mysql_main) {VER(>=8.0.21)}
ensure_plugin_enabled("group_replication", session);
session.runSql("SET GLOBAL group_replication_tls_source='mysql_admin'");
dba.checkInstanceConfiguration();

//@<OUT> dba.checkInstanceConfiguration() must validate if group_replication_tls_source is not persisted to use mysql_admin {VER(>=8.0.21)}
session.runSql("SET PERSIST group_replication_tls_source='mysql_admin'");
dba.checkInstanceConfiguration();

//@<> Clean-up {VER(>= 8.0.21)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
