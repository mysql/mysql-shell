
// Various negative test cases for createCluster()

//@ Initialize
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);

//@# create_cluster_with_cluster_admin_type
dba.createCluster("dev", {"clusterAdminType": "local"});

//@<> createCluster() with memberSslMode DISABLED but require_secure_transport enabled
session.runSql("SET GLOBAL require_secure_transport = TRUE");

EXPECT_THROWS_TYPE(function(){dba.createCluster("dev", {memberSslMode: "DISABLED"});}, `The instance '127.0.0.1:${__mysql_sandbox_port1}' requires secure connections, to create the cluster either turn off require_secure_transport or use the memberSslMode option with 'REQUIRED', 'VERIFY_CA' or 'VERIFY_IDENTITY' value.`, "ArgumentError");

//@<> createCluster() with memberSslMode VERIFY_CA but CA options not set {VER(>=8.0.0)}
session.runSql("SET GLOBAL ssl_ca = DEFAULT");
session.runSql("SET GLOBAL ssl_capath = DEFAULT");

EXPECT_THROWS_TYPE(function(){dba.createCluster("dev", {memberSslMode: "VERIFY_CA"});}, "memberSslMode 'VERIFY_CA' requires Certificate Authority (CA) certificates to be supplied.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS("ERROR: CA certificates options not set. ssl_ca or ssl_capath are required, to supply a CA certificate that matches the one used by the server.");

//@<> createCluster() with memberSslMode VERIFY_IDENTITY but CA options not set {VER(>=8.0.0)}
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

EXPECT_THROWS_TYPE(function(){dba.createCluster("dev", {memberSslMode: "REQUIRED"});}, `The instance '127.0.0.1:${__mysql_sandbox_port1}' does not have SSL enabled, to create the cluster either use an instance with SSL enabled, remove the memberSslMode option or use it with any of 'AUTO' or 'DISABLED'.`, "ArgumentError");

//@ Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
