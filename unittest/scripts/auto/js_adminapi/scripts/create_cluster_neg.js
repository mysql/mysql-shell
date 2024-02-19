//@ {VER(<8.0.24)}
// Various negative test cases for createCluster()

//@<> Initialize
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
shell.connect(__sandbox_uri1);
session2 = mysql.getSession(__sandbox_uri2);

//@<> create_cluster_with_cluster_admin_type
EXPECT_THROWS_TYPE(function(){
    dba.createCluster("dev", {"clusterAdminType": "local"});
}, "Invalid options: clusterAdminType", "ArgumentError");

//@<> createCluster() with memberSslMode DISABLED but require_secure_transport enabled
session.runSql("SET GLOBAL require_secure_transport = TRUE");
session2.runSql("SET GLOBAL require_secure_transport = TRUE");

EXPECT_THROWS_TYPE(function(){
    dba.createCluster("dev", {memberSslMode: "DISABLED"});
}, `The instance '127.0.0.1:${__mysql_sandbox_port1}' requires secure connections, to create the Cluster either turn off require_secure_transport or use the memberSslMode option with 'REQUIRED', 'VERIFY_CA' or 'VERIFY_IDENTITY' value.`, "ArgumentError");

//@<> Regression for BUG#26248116 : MYSQLPROVISION DOES NOT USE SECURE CONNECTIONS BY DEFAULT
EXPECT_NO_THROWS(function(){c = dba.createCluster("dev", {memberSslMode: "REQUIRED"});});
EXPECT_NO_THROWS(function(){c.addInstance(__sandbox_uri2, {recoveryMethod:"incremental"});});

c.removeInstance(__sandbox_uri2);
c.dissolve();

session.runSql("set global super_read_only=0");

//@<> createCluster() with memberSslMode VERIFY_CA but CA options not set
session.runSql("SET GLOBAL ssl_ca = DEFAULT");
session.runSql("SET GLOBAL ssl_capath = DEFAULT");

EXPECT_THROWS_TYPE(function(){dba.createCluster("dev", {memberSslMode: "VERIFY_CA"});}, "memberSslMode 'VERIFY_CA' requires Certificate Authority (CA) certificates to be supplied.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS("ERROR: CA certificates options not set. ssl_ca or ssl_capath are required, to supply a CA certificate that matches the one used by the server.");

//@<> createCluster() with memberSslMode VERIFY_IDENTITY but CA options not set
EXPECT_THROWS_TYPE(function(){dba.createCluster("dev", {memberSslMode: "VERIFY_IDENTITY"});}, "memberSslMode 'VERIFY_IDENTITY' requires Certificate Authority (CA) certificates to be supplied.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS("ERROR: CA certificates options not set. ssl_ca or ssl_capath are required, to supply a CA certificate that matches the one used by the server.");

//@<> createCluster() with memberSslMode REQUIRED but the target instance has SSL disabled

// Disable SSL on the instance
session.runSql("CREATE USER 'unsecure'@'%' IDENTIFIED WITH mysql_native_password BY 'root'");
session.runSql("GRANT ALL PRIVILEGES ON *.* TO 'unsecure'@'%' WITH GRANT OPTION;");
testutil.stopSandbox(__mysql_sandbox_port1);
testutil.changeSandboxConf(__mysql_sandbox_port1, "skip_ssl", "1");
testutil.changeSandboxConf(__mysql_sandbox_port1, "default_authentication_plugin", "mysql_native_password");
testutil.startSandbox(__mysql_sandbox_port1);
shell.connect({scheme: "mysql", host: localhost, port: __mysql_sandbox_port1, user: 'unsecure', password: 'root'});

EXPECT_THROWS_TYPE(function(){dba.createCluster("dev", {memberSslMode: "REQUIRED"});}, `The instance '127.0.0.1:${__mysql_sandbox_port1}' does not have TLS enabled, to create the Cluster either use an instance with TLS enabled, remove the memberSslMode option or use it with any of 'AUTO' or 'DISABLED'.`, "ArgumentError");

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
