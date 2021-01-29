// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
// ensure my.cnf file is saved/restored for replay in recording mode
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});

//@<> Dba: validating members
validateMembers(dba, [
    'checkInstanceConfiguration',
    'configureInstance',
    'configureLocalInstance',
    'configureReplicaSetInstance',
    'createCluster',
    'createReplicaSet',
    'deleteSandboxInstance',
    'deploySandboxInstance',
    'dropMetadataSchema',
    'getCluster',
    'getReplicaSet',
    'getClusterSet',
    'help',
    'killSandboxInstance',
    'rebootClusterFromCompleteOutage',
    'startSandboxInstance',
    'stopSandboxInstance',
    'upgradeMetadata',
    'session',
    'verbose'])

//@# Dba: createCluster errors
dba.createCluster();
dba.createCluster(1,2,3,4);
dba.createCluster(5);
dba.createCluster('');
dba.createCluster('devCluster', {invalid:1, another:2});
dba.createCluster('devCluster', {memberSslMode: 'foo'});
dba.createCluster('devCluster', {memberSslMode: ''});
dba.createCluster('devCluster', {adoptFromGR: true, memberSslMode: 'AUTO'});
dba.createCluster('devCluster', {adoptFromGR: true, memberSslMode: 'REQUIRED'});
dba.createCluster('devCluster', {adoptFromGR: true, memberSslMode: 'DISABLED'});
dba.createCluster('devCluster', {adoptFromGR: true, multiPrimary: true, force: true});
dba.createCluster('devCluster', {adoptFromGR: true, multiPrimary: false});
dba.createCluster('devCluster', {adoptFromGR: true, multiMaster: true, force: true});
dba.createCluster('devCluster', {adoptFromGR: true, multiMaster: false});
dba.createCluster('devCluster', {multiPrimary:false, multiMaster: false});
dba.createCluster('devCluster', {multiPrimary:true, multiMaster: false});
dba.createCluster('devCluster', {ipWhitelist: "  "});
dba.createCluster('#');
dba.createCluster("_1234567890::_1234567890123456789012345678901234567890123456789012345678901234");
dba.createCluster("::");

//@ Dba: createCluster with ANSI_QUOTES success
// save current sql mode
var result = session.runSql("SELECT @@GLOBAL.SQL_MODE");
var row = result.fetchOne();
var original_sql_mode = row[0];
session.runSql("SET @@GLOBAL.SQL_MODE = ANSI_QUOTES");
// Check that sql mode has been changed
var result = session.runSql("SELECT @@GLOBAL.SQL_MODE");
var row = result.fetchOne();
print("Current sql_mode is: "+ row[0] + "\n");

var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED', clearReadOnly: true});
print(c1);

//@ Dba: dissolve cluster created with ansi_quotes and restore original sql_mode
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
c1.dissolve({ force: true });


// Set old_sql_mode
session.runSql("SET @@GLOBAL.SQL_MODE = '"+ original_sql_mode+ "'");
var result = session.runSql("SELECT @@GLOBAL.SQL_MODE");
var row = result.fetchOne();
var restored_sql_mode = row[0];
var was_restored = restored_sql_mode == original_sql_mode;
print("Original SQL_MODE has been restored: " + was_restored + "\n");
session.runSql("set GLOBAL super_read_only = 0");

var enable_bug_26979375 = true;
try {
  var result = session.runSql("SHOW CREATE VIEW mysql_innodb_cluster_metadata.schema_version");
  var row = result.fetchOne();
  if (row) {
    var create = row.getField("Create View");
    // Because of the addition of the metadata version precondition checks, Bug
    // 26979375 requires the schema_version view to use SECURITY INVOKER for
    // this test to succeed using SECURITY DEFINER it is not possible to read
    // the view if the creator user no longer exists.
    if (create.search("SECURITY DEFINER") != -1) {
      enable_bug_26979375 = false;
    }
  } else {
    testutil.fail("Error trying to get mysql_innodb_cluster_metadata.schema_version structure");
  }
} catch (error) {
  testutil.fail("Error trying to get mysql_innodb_cluster_metadata.schema_version structure");
}

//@ Dba: create cluster using a non existing user that authenticates as another user (BUG#26979375) {enable_bug_26979375}
// Clear super read_only
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'test_user'@'%'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'test_user'@'%' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");
session.close();

shell.connect({scheme:'mysql', host: "127.0.0.1", port: __mysql_sandbox_port1, user: 'test_user', password: ''});
c1 = dba.createCluster("devCluster", {clearReadOnly: true});
c1

//@ Dba: dissolve cluster created using a non existing user that authenticates as another user (BUG#26979375) {enable_bug_26979375}
testutil.expectPrompt("Are you sure you want to dissolve the cluster?", "y");
c1.dissolve({force:true});
session.close()
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

// drop created test_user
// Clear super read_only
session.runSql("set GLOBAL super_read_only = 0");
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP user 'test_user'@'%'");
session.runSql("SET sql_log_bin = 1");

//@<OUT> Dba: createCluster with interaction
var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED', clearReadOnly: true});

//@ Dba: checkInstanceConfiguration in a cluster member
testutil.expectPassword("*", "root");
dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port1);

//@<OUT> Dba: checkInstanceConfiguration ok 1
testutil.expectPassword("*", "root");
dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port2);

//@<OUT> Dba: checkInstanceConfiguration ok 2
testutil.expectPassword("*", "root");
dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port2, {password:'root'});

//@<OUT> Dba: checkInstanceConfiguration report with errors
var uri2 = 'root@localhost:' + __mysql_sandbox_port2;
var res = dba.checkInstanceConfiguration(uri2, {mycnfPath:'mybad.cnf'});

// TODO: This test needs an actual remote instance
//  Dba: configureLocalInstance error 1
//dba.configureLocalInstance('someotherhost:' + __mysql_sandbox_port1);

// TODO(rennox): This test case is not reliable since requires
// that no my.cnf exist on the default paths
//--@<OUT> Dba: configureLocalInstance error 2
//dba.configureLocalInstance('localhost:' + __mysql_port);

//@<OUT> Dba: configureLocalInstance error 3 {VER(<8.0.11)}
testutil.expectPassword("*", "root");
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1);

//@<OUT> Dba: configureLocalInstance error 3 bad call {VER(>=8.0.11)}
testutil.expectPassword("*", "root");
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1);

session.close();

//@ Dba: Create user without all necessary privileges
// create user that has all permissions to admin a cluster but doesn't have
// the grant privileges for them, so it cannot be used to create viable accounts
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER

connect_to_sandbox([__mysql_sandbox_port2]);
session.runSql('SET GLOBAL super_read_only = 0');
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("CREATE USER missingprivileges@localhost");
session.runSql("GRANT SUPER, CREATE USER ON *.* TO missingprivileges@localhost");
session.runSql("GRANT SELECT ON `performance_schema`.* TO missingprivileges@localhost WITH GRANT OPTION");
session.runSql("GRANT SELECT ON `mysql_innodb_cluster_metadata`.* TO missingprivileges@localhost");
session.runSql("SET SQL_LOG_BIN=1");
var result = session.runSql("select COUNT(*) from mysql.user where user='missingprivileges' and host='localhost'");
var row = result.fetchOne();
print("Number of accounts: "+ row[0] + "\n");
session.close();

//@# Dba: configureLocalInstance not enough privileges 1
testutil.expectPassword("*", "");
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER
dba.configureLocalInstance('missingprivileges@localhost:' + __mysql_sandbox_port2);

//@# Dba: configureLocalInstance not enough privileges 2
testutil.expectPassword("*", "");
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER
dba.configureLocalInstance('missingprivileges@localhost:' + __mysql_sandbox_port2,
    {clusterAdmin: "missingprivileges", clusterAdminPassword:""});

//@# Dba: configureLocalInstance not enough privileges 3
testutil.expectPassword("*", "");
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER

dba.configureLocalInstance('missingprivileges@localhost:' + __mysql_sandbox_port2);

//@ Dba: Show list of users to make sure the user missingprivileges@% was not created
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER
connect_to_sandbox([__mysql_sandbox_port2]);
var result = session.runSql("select COUNT(*) from mysql.user where user='missingprivileges' and host='%'");
var row = result.fetchOne();
print("Number of accounts: "+ row[0] + "\n");

//@ Dba: Delete created user and reconnect to previous sandbox
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("DROP USER missingprivileges@localhost");
session.runSql("SET SQL_LOG_BIN=1");
var result = session.runSql("select COUNT(*) from mysql.user where user='missingprivileges' and host='localhost'");
var row = result.fetchOne();
print("Number of accounts: "+ row[0] + "\n");
session.close();
connect_to_sandbox([__mysql_sandbox_port1]);

//@<OUT> Dba: configureLocalInstance updating config file
testutil.expectPassword("*", "root");
testutil.expectPrompt("Do you want to perform the required configuration changes? [y/n]: ", "y");
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf'});

//@ Dba: create an admin user with all needed privileges
// Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
connect_to_sandbox([__mysql_sandbox_port2]);
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("CREATE USER 'mydba'@'localhost' IDENTIFIED BY ''");
session.runSql("GRANT ALL ON *.* TO 'mydba'@'localhost' WITH GRANT OPTION");
session.runSql("SET SQL_LOG_BIN=1");
var result = session.runSql("SELECT COUNT(*) FROM mysql.user WHERE user='mydba' and host='localhost'");
var row = result.fetchOne();
print("Number of 'mydba'@'localhost' accounts: "+ row[0] + "\n");
session.close();

//@<OUT> Dba: configureLocalInstance create different admin user
testutil.expectPassword("*", "");  // Pass for mydba
testutil.expectPrompt("*", "2");   // Option (account with diff name)
testutil.expectPrompt("Account Name: ", "dba_test");  // account name
testutil.expectPassword("Password for new account: ", "");        // account pass
testutil.expectPassword("Confirm password: ", "");        // account pass confirmation

// Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configureLocalInstance('mydba@localhost:' + __mysql_sandbox_port2);

//@<OUT> Dba: configureLocalInstance create existing valid admin user
testutil.expectPassword("*", "");  // Pass for mydba
testutil.expectPrompt("*", "2");   // Option (account with diff name)
testutil.expectPrompt("Account Name: ", "dba_test");  // account name

// Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configureLocalInstance('mydba@localhost:' + __mysql_sandbox_port2);

//@ Dba: remove needed privilege (REPLICATION SLAVE) from created admin user
connect_to_sandbox([__mysql_sandbox_port2]);
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("REVOKE REPLICATION SLAVE ON *.* FROM 'dba_test'@'%'");
session.runSql("SET SQL_LOG_BIN=1");
session.close();

//@ Dba: configureLocalInstance create existing invalid admin user
testutil.expectPassword("*", "");  // Pass for mydba
testutil.expectPrompt("*", "2");   // Option (account with diff name)
testutil.expectPrompt("Account Name: ", "dba_test");  // account name
testutil.expectPassword("*", "");        // account pass
testutil.expectPassword("*", "");        // account pass confirmation

// Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configureLocalInstance('mydba@localhost:' + __mysql_sandbox_port2);

//@ Dba: Delete previously create an admin user with all needed privileges
// Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
connect_to_sandbox([__mysql_sandbox_port2]);
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("DROP USER 'mydba'@'localhost'");
session.runSql("DROP USER 'dba_test'@'%'");
session.runSql("SET SQL_LOG_BIN=1");
var result = session.runSql("SELECT COUNT(*) FROM mysql.user WHERE user='mydba' and host='localhost'");
var row = result.fetchOne();
print("Number of 'mydba'@'localhost' accounts: "+ row[0] + "\n");
session.close();

//@# Check if all missing privileges are reported for user with no privileges
connect_to_sandbox([__mysql_sandbox_port2]);
// Create account with no privileges
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'no_privileges'@'%'");
//NOTE: at least privileges to access the metadata are required, otherwise another error is issued.
session.runSql("GRANT SELECT ON `mysql_innodb_cluster_metadata`.* TO 'no_privileges'@'%'");
// NOTE: This privilege is required othe the generated error will be:
//Dba.configureLocalInstance: Unable to detect target instance state. Please check account privileges.
session.runSql("GRANT SELECT ON `performance_schema`.`replication_group_members` TO 'no_privileges'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

// Test
testutil.expectPrompt("*", "");  // press Enter to cancel
dba.configureLocalInstance({host: localhost, port: __mysql_sandbox_port2, user: 'no_privileges', password:''});

connect_to_sandbox([__mysql_sandbox_port2]);
// Drop user
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP user 'no_privileges'@'%'");
session.runSql("SET sql_log_bin = 1");

// Create account without global grant option
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'no_global_grant'@'%'");
session.runSql("GRANT ALL PRIVILEGES on *.* to 'no_global_grant'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

//@ configureLocalInstance() should fail if user does not have global GRANT OPTION
dba.configureLocalInstance({host: localhost, port: __mysql_sandbox_port2, user: 'no_global_grant', password:''});

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'no_global_grant', password: ''});

//@ createCluster() should fail if user does not have global GRANT OPTION
dba.createCluster("cluster");

session.close();

connect_to_sandbox([__mysql_sandbox_port2]);
// Drop user
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP user 'no_global_grant'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

connect_to_sandbox([__mysql_sandbox_port1]);

//@# Dba: getCluster errors
var c2 = dba.getCluster(5);
var c2 = dba.getCluster('x', 5, 6);
var c2 = dba.getCluster(null, 5);
var c2 = dba.getCluster('');
var c2 = dba.getCluster('#');
var c2 = dba.getCluster("over40chars_12345678901234567890123456789");

//@<OUT> Dba: getCluster with interaction
var c2 = dba.getCluster('devCluster');
c2;

//@ Dba: getCluser validate object serialization output - tabbed
const original_format = shell.options.resultFormat
shell.options.resultFormat = 'tabbed'
c2;

//@ Dba: getCluser validate object serialization output - table
shell.options.resultFormat = 'table'
c2;

//@ Dba: getCluser validate object serialization output - vertical
shell.options.resultFormat = 'vertical'
c2;

//@ Dba: getCluser validate object serialization output - json
shell.options.resultFormat = 'json'
c2;

//@ Dba: getCluser validate object serialization output - json/raw
shell.options.resultFormat = 'json/raw'
c2;

shell.options.resultFormat = original_format

//@<OUT> Dba: getCluster with interaction (default)
var c3 = dba.getCluster();
c3;

//@<OUT> Dba: getCluster with interaction (default null)
var c3 = dba.getCluster(null);
c3;

session.close();
//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
