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

//@<> BUG#29305551: Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

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
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
