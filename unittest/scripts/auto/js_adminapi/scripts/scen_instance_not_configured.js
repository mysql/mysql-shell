//@ {false && (VER(>5.7.21) || __os_type != 'macos')}
// This test doesn't work in macos with server 5.7 because of bug #89123
// Disabled because mysqlprovision needs hostname to be resolvable

// Purpose:
// Ensure proper handling (abort operation with error msg) when an instance
// that is not yet configured is used for createCluster or addInstance

// Common expectations (to be moved to a common file to be sourced)

function EXPECT_BEGIN() {
  // clear any leftover output from previous tests
  while (testutil.fetchCapturedStdout(true)) {}
}

function EXPECT_VALIDATING_LOCAL_INSTANCE(port) {
  var line = testutil.fetchCapturedStdout(true);
  while (line == "\n") {
    line = testutil.fetchCapturedStdout(true);
  }
  EXPECT_CONTAINS("Validating local MySQL instance listening at port "+port+" for use in an InnoDB cluster...", line);
}

function EXPECT_VALIDATING_INSTANCE(address) {
  var line = testutil.fetchCapturedStdout(true);
  while (line == "\n") {
    line = testutil.fetchCapturedStdout(true);
  }
  EXPECT_CONTAINS("Validating MySQL instance at " + address + " for use in an InnoDB cluster...", line);
}

// Check that the return value of a checkInstance() matches the expected one for a raw, non-configured instance
function EXPECT_RETVAL_CHECK_INSTANCE_RAW_INSTANCE(result) {
  if (__version_num > 80000) {  // MySQL 5.7 had different defaults
    var errors = {"config_errors": [{"action": "server_update", "current": "CRC32", "option": "binlog_checksum", "required": "NONE"}, {"action": "restart", "current": "<no value>", "option": "disabled_storage_engines", "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"}, {"action": "restart", "current": "OFF", "option": "enforce_gtid_consistency", "required": "ON"}, {"action": "restart", "current": "OFF", "option": "gtid_mode", "required": "ON"}, {"action": "restart", "current": "1", "option": "server_id", "required": "<unique ID>"}], "status": "error"};
  } else {
    var errors = {"config_errors": [{"action": "server_update", "current": "CRC32", "option": "binlog_checksum", "required": "NONE"}, {"action": "restart", "current": "<no value>", "option": "disabled_storage_engines", "required": "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE"}, {"action": "restart", "current": "OFF", "option": "enforce_gtid_consistency", "required": "ON"}, {"action": "restart", "current": "OFF", "option": "gtid_mode", "required": "ON"}, {"action": "restart", "current": "0", "option": "log_bin", "required": "1"}, {"action": "restart", "current": "0", "option": "log_slave_updates", "required": "ON"}, {"action": "restart", "current": "FILE", "option": "master_info_repository", "required": "TABLE"}, {"action": "restart", "current": "FILE", "option": "relay_log_info_repository", "required": "TABLE"}, {"action": "restart", "current": "0", "option": "server_id", "required": "<unique ID>"}, {"action": "restart", "current": "OFF", "option": "transaction_write_set_extraction", "required": "XXHASH64"}], "status": "error"};
  }

  EXPECT_EQ(errors, result);
}

function EXPECT_REPORTED_HOST() {
  var line = testutil.fetchCapturedStdout(true);
  while (line == "\n") {
    line = testutil.fetchCapturedStdout(true);
  }
  EXPECT_CONTAINS("This instance reports its own address as " + real_hostname, line);
}

function EXPECT_REPORTED_HOST_VERBOSE() {
  var line = testutil.fetchCapturedStdout(true);
  while (line == "\n") {
    line = testutil.fetchCapturedStdout(true);
  }
  EXPECT_CONTAINS("This instance reports its own address as " + real_hostname, line);
  EXPECT_CONTAINS("Clients and other cluster members will communicate", testutil.fetchCapturedStdout(true));
}

function EXPECT_LOOPBACK_ERROR() {
  EXPECT_NEXT_OUTPUT("ERROR: " + real_hostname + " resolves to a loopback address.");
  EXPECT_NEXT_OUTPUT("Because that address will be used by other cluster members to connect to it, it must resolve to an externally reachable address.");
  EXPECT_NEXT_OUTPUT("This address was determined through the report_host or hostname MySQL system variable.");
}

function EXPECT_SCHEMA_OK() {
  EXPECT_NEXT_OUTPUT("Checking whether existing tables comply with Group Replication requirements...");
  EXPECT_NEXT_OUTPUT("No incompatible tables detected");
}

function EXPECT_SCHEMA_BROKEN(bad_engine_tables, bad_key_tables) {
  EXPECT_NEXT_OUTPUT("Checking whether existing tables comply with Group Replication requirements...");
  if (bad_engine_tables.length > 0) {
    EXPECT_NEXT_OUTPUT("WARNING: The following tables use a storage engine that are not supported by Group Replication:")
    var line = testutil.fetchCapturedStdout(true);
    var broken = line.trim().split(", ").sort();
    EXPECT_EQ(bad_engine_tables, broken);
  }
  if (bad_key_tables.length > 0) {
    EXPECT_NEXT_OUTPUT("WARNING: The following tables do not have a Primary Key or equivalent column:")
    var line = testutil.fetchCapturedStdout(true);
    var broken = line.trim().split(", ").sort();
    EXPECT_EQ(bad_key_tables, broken);
  }
  EXPECT_NEXT_OUTPUT("Group Replication requires tables to use InnoDB and have a PRIMARY KEY or PRIMARY ");
}

function EXPECT_CONFIGURATION_ISSUES_RAW_INSTANCE(silent) {
  if (__version_num > 80000) {  // MySQL 5.7 had different defaults
    var base_config_issues = ["binlog_checksum", "disabled_storage_engines", "enforce_gtid_consistency", "gtid_mode", "server_id"];
  } else {
    var base_config_issues = ["binlog_checksum", "disabled_storage_engines", "enforce_gtid_consistency", "gtid_mode", "server_id", "log_bin", "log_slave_updates", "master_info_repository", "relay_log_info_repository", "transaction_write_set_extraction"];
  }
  var options = base_config_issues.slice();


  var line = testutil.fetchCapturedStdout(true);
  while (line == "\n") line = testutil.fetchCapturedStdout(true);
  if (!silent)
    EXPECT_CONTAINS("Checking instance configuration...", line);
  // Search the issues table
  while (line.indexOf("| Variable") < 0 && line != "") line = testutil.fetchCapturedStdout(true);
  if (line == "")
    testutil.fail("Expected output missing (configuration issues table)");
  testutil.fetchCapturedStdout(true);  // skip separator
  line = testutil.fetchCapturedStdout(true);
  while (line.indexOf("+-----------") < 0 && line != "") {
    var opt = line.split("|")[1].trim();
    var i = options.indexOf(opt);
    if (i >= 0) {
      options.splice(i,1);
    } else {
      testutil.fail("Unexpected option '" + opt + "' in configuration issues list");
    }
    line = testutil.fetchCapturedStdout(true);
  }
  if (options.length > 0) {
    testutil.fail("Options " + repr(options) + " missing from output");
  }
  if (line == "")
    testutil.fail("Expected output missing (end of configuration issues table)");
}

function EXPECT_REQUIRE_RESTART() {

}

function EXPECT_REQUIRE_CONFIGURE() {
  var line = testutil.fetchCapturedStdout(true);
  while (line == "\n") line = testutil.fetchCapturedStdout(true);
  EXPECT_CONTAINS("Please use the dba.configureInstance() command to repair these issues.", line);
}

function EXPECT_REQUIRE_CHECK() {

}

function EXPECT_END() {
  var line = testutil.fetchCapturedStdout(true);
  while (line != "") {
    if (line != "\n")
      testutil.fail("Unexpected output: <red>" + line + testutil.fetchCapturedStdout() + "</red>");
    line = testutil.fetchCapturedStdout(true);
  }
}

// Scenario setup flags

// raw instance with nothing configured
var SCEN_RAW = 1;
// log_bin is disabled
var SCEN_MYCNF_CHANGE_REQUIRED = 2;
// sysvar change required
var SCEN_SYSVAR_CHANGE_REQUIRED = 4;
// restart required
var SCEN_RESTART_REQUIRED = 8;
// incompatible schema
var SCEN_BAD_SCHEMA = 16;
// root from any OK
var SCEN_ROOT_FROM_ANY = 32;
// root from any OK
var SCEN_REPORT_HOST = 64;

var deployed = false;

function prepare_cluster_seed(sandbox, server_flags) {
  const allowed_flags = SCEN_ROOT_FROM_ANY;

  if (server_flags & ~allowed_flags) {
    testutil.fail("TEST ERROR: prepare_cluster_seed() called with invalid flags");
  }

  var port = SANDBOX_PORTS[sandbox - 1];
  var luri = SANDBOX_LOCAL_URIS[sandbox - 1];
  var huri = SANDBOX_URIS[sandbox - 1];

  testutil.deploySandbox(port, 'root', {report_host: hostname});

  if (server_flags & SCEN_ROOT_FROM_ANY) {
    var s = mysql.getSession(luri);
    s.runSql("SET sql_log_bin = 0");
    s.runSql("CREATE USER root@'%' IDENTIFIED BY 'root'");
    s.runSql("GRANT ALL ON *.* TO root@'%' WITH GRANT OPTION");
    s.runSql("SET sql_log_bin = 1");
    s.close();
  }

  shell.connect(huri);
  var cluster = dba.createCluster("mycluster");
  session.close();
  return cluster;
}

function prepare_instance(sandbox, server_flags) {
  var port = SANDBOX_PORTS[sandbox - 1];
  var luri = SANDBOX_LOCAL_URIS[sandbox - 1];
  var huri = SANDBOX_URIS[sandbox - 1];

  if (deployed)
    testutil.destroySandbox(port);

  var s = null;
  if (server_flags & SCEN_RAW) {
    testutil.deployRawSandbox(port, 'root', {report_host: hostname});

    s = mysql.getSession(luri);
  } else {
    testutil.deploySandbox(port, 'root', {report_host: hostname});
    s = mysql.getSession(luri);

    var restart = false;
    if (server_flags & SCEN_MYCNF_CHANGE_REQUIRED) {
      testutil.changeSandboxConf(port, "log_bin", "0");
      restart = true;
    }
    if (server_flags & SCEN_SYSVAR_CHANGE_REQUIRED) {
      testutil.changeSandboxConf(port, "binlog_format", "mixed");
      restart = true;
    }

    if (server_flags & SCEN_RESTART_REQUIRED) {
      testutil.changeSandboxConf(port, "binlog_checksum", "CRC32");
      restart = true;
    }

    if (server_flags & SCEN_REPORT_HOST) {
      testutil.changeSandboxConf(port, "report_host", hostname);
      restart = true;
    }

    if (server_flags & SCEN_BAD_SCHEMA) {
      //testutil.changeSandboxConf(port, "disabled_storage_engines", "BLACKHOLE");
      restart = true;
    }

    if (restart) {
      testutil.restartSandbox(port);
    }

    if (server_flags & SCEN_RESTART_REQUIRED) {
      // for 5.7 server checks
      testutil.changeSandboxConf(port, "binlog_checksum", "none");
      // for 8.0.11 server checks
      if (__version_num >= 80005)
        testutil.runSql("set persist binlog_checksum=none");
    }
  }

  s.runSql("SELECT @@disabled_storage_engines").fetchAll();
  s.runSql("CREATE SCHEMA mydb");
  s.runSql("CREATE TABLE mydb.okpk (a int primary key, b varchar(32))");
  s.runSql("CREATE TABLE mydb.okuk (a int, b varchar(32) unique not null)");
  if (server_flags & SCEN_BAD_SCHEMA) {
    s.runSql("CREATE TABLE mydb.bad_engine (a int primary key) engine=MyISAM");
    s.runSql("CREATE TABLE mydb.badpk (a int, b varchar(32))");
    s.runSql("CREATE TABLE mydb.baduk (a int, b varchar(32) unique)");
  }

  if (server_flags & SCEN_ROOT_FROM_ANY) {
    s.runSql("SET sql_log_bin = 0");
    s.runSql("CREATE USER root@'%' IDENTIFIED BY 'root'");
    s.runSql("GRANT ALL ON *.* TO root@'%' WITH GRANT OPTION");
    s.runSql("SET sql_log_bin = 1");
  }

  s.close();

  if ((server_flags & (SCEN_RAW|SCEN_BAD_SCHEMA)) == SCEN_BAD_SCHEMA) {
    testutil.changeSandboxConf(port, "disabled_storage_engines", "MyISAM,BLACKHOLE,FEDERATED,CSV,ARCHIVE");
    testutil.restartSandbox(port);
  }

  deployed = true;
}

//@ Initialization
// Deploy test cluster with seed instance only
cluster = prepare_cluster_seed(1, SCEN_ROOT_FROM_ANY);

// All tests use interactive by default
shell.options["useWizards"] = true;

//------------------------------------------------------------------------------------------
//@ Prepare raw, nothing configured, no schema issues
prepare_instance(2, SCEN_RAW|SCEN_ROOT_FROM_ANY);

shell.connect(__hostname_uri2);

//@<> Info for manual sanity check
println("** hostname:", hostname);
println("** real_hostname:", real_hostname);

var res = session.runSql("select @@hostname, @@report_host").fetchOne();
println("** @@hostname:", res[0]);
println("** @@report_host:", res[1]);

//@<> Raw, new instance, checkInstance
EXPECT_BEGIN();

var res = dba.checkInstanceConfiguration();

EXPECT_RETVAL_CHECK_INSTANCE_RAW_INSTANCE(res);

EXPECT_VALIDATING_LOCAL_INSTANCE(__mysql_sandbox_port2);

EXPECT_REPORTED_HOST_VERBOSE();

// TODO(alfredo) split a new line of testing for hosts like ubuntu to check specifically
// the loopback detection. other tests should be using the loopback workaround, so the
// error should never happen
if (real_host_is_loopback) EXPECT_LOOPBACK_ERROR();
EXPECT_SCHEMA_OK();
EXPECT_CONFIGURATION_ISSUES_RAW_INSTANCE();

EXPECT_REQUIRE_CONFIGURE()
EXPECT_END();

//@<> Raw, new instance, createCluster
EXPECT_BEGIN();

EXPECT_THROWS(function() {dba.createCluster("c")}, "Instance check failed");

EXPECT_NEXT_OUTPUT("Validating instance at "+hostname+":"+__mysql_sandbox_port2+"...");
EXPECT_REPORTED_HOST();
// createCluster and addInstance don't do schema check
EXPECT_CONFIGURATION_ISSUES_RAW_INSTANCE(true);

EXPECT_REQUIRE_CONFIGURE();
EXPECT_NEXT_OUTPUT("ERROR: Instance must be configured and validated with dba.checkInstanceConfiguration() and dba.configureInstance() before it can be used in an InnoDB cluster.");

EXPECT_END();

//@<> Raw, new instance, no-interactive, createCluster
// interactive option doesn't exist for createCluster yet
shell.options["useWizards"] = false;
EXPECT_BEGIN();

EXPECT_THROWS(function() {dba.createCluster("c")}, "Instance check failed");

EXPECT_NEXT_OUTPUT("Validating instance at "+hostname+":"+__mysql_sandbox_port2+"...");
EXPECT_REPORTED_HOST();
EXPECT_CONFIGURATION_ISSUES_RAW_INSTANCE(true);

EXPECT_REQUIRE_CONFIGURE();
EXPECT_NEXT_OUTPUT("ERROR: Instance must be configured and validated with dba.checkInstanceConfiguration() and dba.configureInstance() before it can be used in an InnoDB cluster.");
EXPECT_END();
shell.options["useWizards"] = true;
//@<> Raw, new instance, addInstance
EXPECT_BEGIN();

EXPECT_THROWS(function() {cluster.addInstance(__hostname_uri2)}, "Instance check failed");
EXPECT_NEXT_OUTPUT("Validating instance at "+hostname+":"+__mysql_sandbox_port2+"...");
EXPECT_REPORTED_HOST();
EXPECT_CONFIGURATION_ISSUES_RAW_INSTANCE(true);

EXPECT_REQUIRE_CONFIGURE();
EXPECT_NEXT_OUTPUT("ERROR: Instance must be configured and validated with dba.checkInstanceConfiguration() and dba.configureInstance() before it can be used in an InnoDB cluster.");
EXPECT_END();

//@<> Raw, new instance, no-interactive, addInstance
shell.options["useWizards"] = false;
EXPECT_BEGIN();

EXPECT_THROWS(function() {cluster.addInstance(__hostname_uri2)}, "Instance check failed");

EXPECT_NEXT_OUTPUT("Validating instance at "+hostname+":"+__mysql_sandbox_port2+"...");
EXPECT_REPORTED_HOST();
EXPECT_CONFIGURATION_ISSUES_RAW_INSTANCE(true);

EXPECT_REQUIRE_CONFIGURE();
EXPECT_NEXT_OUTPUT("ERROR: Instance must be configured and validated with dba.checkInstanceConfiguration() and dba.configureInstance() before it can be used in an InnoDB cluster.");
EXPECT_END();
shell.options["useWizards"] = true;

session.close();

//------------------------------------------------------------------------------------------
//@ Prepare raw, nothing configured, with schema issues
prepare_instance(2, SCEN_RAW|SCEN_ROOT_FROM_ANY|SCEN_BAD_SCHEMA);

shell.connect(__hostname_uri2);

//@<> Raw with schema issues, new instance, checkInstance
EXPECT_BEGIN();

var res = dba.checkInstanceConfiguration();

EXPECT_RETVAL_CHECK_INSTANCE_RAW_INSTANCE(res);

EXPECT_VALIDATING_LOCAL_INSTANCE(__mysql_sandbox_port2);
EXPECT_REPORTED_HOST_VERBOSE();

if (real_host_is_loopback) EXPECT_LOOPBACK_ERROR();

EXPECT_SCHEMA_BROKEN(["mydb.bad_engine"], ["mydb.badpk", "mydb.baduk"]);
EXPECT_CONFIGURATION_ISSUES_RAW_INSTANCE();

EXPECT_REQUIRE_CONFIGURE()
EXPECT_END();


//@<> Raw with schema issues, new instance, createCluster
EXPECT_BEGIN();

EXPECT_THROWS(function() {dba.createCluster("c")}, "Instance check failed");

EXPECT_NEXT_OUTPUT("Validating instance at "+hostname+":"+__mysql_sandbox_port2+"...");
EXPECT_REPORTED_HOST();
EXPECT_CONFIGURATION_ISSUES_RAW_INSTANCE(true);

EXPECT_REQUIRE_CONFIGURE();
EXPECT_NEXT_OUTPUT("ERROR: Instance must be configured and validated with dba.checkInstanceConfiguration() and dba.configureInstance() before it can be used in an InnoDB cluster.");
EXPECT_END();

//@<> Raw with schema issues, new instance, addInstance
EXPECT_BEGIN();

EXPECT_THROWS(function() {cluster.addInstance(__hostname_uri2)}, "Instance check failed");

EXPECT_NEXT_OUTPUT("Validating instance at "+hostname+":"+__mysql_sandbox_port2+"...");
EXPECT_REPORTED_HOST();

if (real_host_is_loopback)
  EXPECT_LOOPBACK_ERROR();

EXPECT_CONFIGURATION_ISSUES_RAW_INSTANCE(true);

EXPECT_REQUIRE_CONFIGURE();
EXPECT_NEXT_OUTPUT("ERROR: Instance must be configured and validated with dba.checkInstanceConfiguration() and dba.configureInstance() before it can be used in an InnoDB cluster.");
EXPECT_END();

session.close();

//------------------------------------------------------------------------------------------
//@ Prepare raw, config OK but schema issues
//prepare_instance(2, SCEN_ROOT_FROM_ANY|SCEN_BAD_SCHEMA);




//------------------------------------------------------------------------------------------
// TODO - more test scenarios: remote instance with 5.7 (no access to config file), user without privs to run the checks, hostname that resolves to a loopback

//------------------------------------------------------------------------------------------
//@ Cleanup
cluster.disconnect();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
