// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script

//@ Session: validating members
var members = dir(dba);

print("Session Members:", members.length);
validateMember(members, 'createCluster');
validateMember(members, 'deleteSandboxInstance');
validateMember(members, 'deploySandboxInstance');
validateMember(members, 'dropMetadataSchema');
validateMember(members, 'getCluster');
validateMember(members, 'help');
validateMember(members, 'killSandboxInstance');
validateMember(members, 'resetSession');
validateMember(members, 'startSandboxInstance');
validateMember(members, 'checkInstanceConfiguration');
validateMember(members, 'stopSandboxInstance');
validateMember(members, 'configureLocalInstance');
validateMember(members, 'verbose');
validateMember(members, 'rebootClusterFromCompleteOutage');

//@# Dba: createCluster errors
var c1 = dba.createCluster();
var c1 = dba.createCluster(1,2,3,4);
var c1 = dba.createCluster(5);
var c1 = dba.createCluster('');
var c1 = dba.createCluster('devCluster', {invalid:1, another:2});
var c1 = dba.createCluster('devCluster', {memberSslMode: 'foo'});
var c1 = dba.createCluster('devCluster', {memberSslMode: ''});
var c1 = dba.createCluster('devCluster', {adoptFromGR: true, memberSslMode: 'AUTO'});
var c1 = dba.createCluster('devCluster', {adoptFromGR: true, memberSslMode: 'REQUIRED'});
var c1 = dba.createCluster('devCluster', {adoptFromGR: true, memberSslMode: 'DISABLED'});
var c1 = dba.createCluster('devCluster', {adoptFromGR: true, multiMaster: true, force: true});
var c1 = dba.createCluster('devCluster', {adoptFromGR: true, multiMaster: false});
var c1 = dba.createCluster('devCluster', {ipWhitelist: "  "});
var c1 = dba.createCluster('#');

// If this test is executed standalone super_read_only
// won't be enabled
session.runSql('SET GLOBAL super_read_only = 1');

//@<OUT> Dba: super-read-only error (BUG#26422638)
dba.createCluster('devCluster');

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

if (__have_ssl)
    var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED'});
else
    var c1 = dba.createCluster('devCluster', {memberSslMode: 'DISABLED'});
print(c1);

//@ Dba: dissolve cluster created with ansi_quotes and restore original sql_mode
c1.dissolve({force:true});

// Set old_sql_mode
session.runSql("SET @@GLOBAL.SQL_MODE = '"+ original_sql_mode+ "'");
var result = session.runSql("SELECT @@GLOBAL.SQL_MODE");
var row = result.fetchOne();
var restored_sql_mode = row[0];
var was_restored = restored_sql_mode == original_sql_mode;
print("Original SQL_MODE has been restored: "+ was_restored + "\n");

//@<OUT> Dba: createCluster with interaction
if (__have_ssl)
  var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED'});
else
  var c1 = dba.createCluster('devCluster', {memberSslMode: 'DISABLED'});

// TODO: add multi-master unit-tests

//@ Dba: checkInstanceConfiguration error
dba.checkInstanceConfiguration('localhost:' + __mysql_sandbox_port1);

//@<OUT> Dba: checkInstanceConfiguration ok 1
dba.checkInstanceConfiguration('localhost:' + __mysql_sandbox_port2);

//@<OUT> Dba: checkInstanceConfiguration ok 2
dba.checkInstanceConfiguration('localhost:' + __mysql_sandbox_port2, {password:'root'});

//@<OUT> Dba: checkInstanceConfiguration report with errors
var uri2 = 'localhost:' + __mysql_sandbox_port2;
var res = dba.checkInstanceConfiguration(uri2, {mycnfPath:'mybad.cnf'});

//@ Dba: configureLocalInstance error 1
dba.configureLocalInstance('someotherhost:' + __mysql_sandbox_port1);

// TODO(rennox): This test case is not reliable since requires
// that no my.cnf exist on the default paths
//--@<OUT> Dba: configureLocalInstance error 2
//dba.configureLocalInstance('localhost:' + __mysql_port);

//@<OUT> Dba: configureLocalInstance error 3
dba.configureLocalInstance('localhost:' + __mysql_sandbox_port1);

connect_to_sandbox([__mysql_sandbox_port3]);
session.runSql('SET GLOBAL super_read_only = 0');
// If this test is executed standalone the GR plugin won't be installed
// So configure will fail with:
//{
//    "errors": [
//        "An error was found trying to install the group_replication plugin: Query failed. 1290 (HY000): The MySQL server is running with the --super-read-only option so it cannot execute this statement",
//        "Error checking instance: The group_replication plugin could not be loaded in the server 'localhost:3336'"
//    ],
//    "restart_required": false,
//    "status": "error"
//}
//
// Since this is a forced behaviour, we must install the plugin first
//session.runSql('INSTALL PLUGIN group_replication SONAME \'group_replication.so\'');
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("CREATE USER 'myAdmin'@'localhost' IDENTIFIED BY ''");
session.runSql("GRANT ALL ON *.* TO 'myAdmin'@'localhost' WITH GRANT OPTION");
session.runSql("SET SQL_LOG_BIN=1");
// Enable super_read_only to test this scenario
session.runSql('SET GLOBAL super_read_only = 1');

//@<OUT> Dba.configureLocalInstance: super-read-only error (BUG#26422638)
dba.configureLocalInstance('myAdmin@localhost:' + __mysql_sandbox_port3);

//@<OUT> Dba.configureLocalInstance: clearReadOnly
dba.configureLocalInstance('myAdmin@localhost:' + __mysql_sandbox_port3);

//Delete created user and disable super_read_only
session.runSql('SET GLOBAL super_read_only = 0');
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("DROP USER 'myAdmin'@'localhost'");
session.runSql("SET SQL_LOG_BIN=1");
session.close();

//@ Dba: Create user without all necessary privileges
// create user that has all permissions to admin a cluster but doesn't have
// the grant privileges for them, so it cannot be used to create viable accounts
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER

// Disable super_read_only
connect_to_sandbox([__mysql_sandbox_port2]);
session.runSql('SET GLOBAL super_read_only = 0');
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("CREATE USER missingprivileges@localhost");
session.runSql("GRANT SUPER, CREATE USER ON *.* TO missingprivileges@localhost");
session.runSql("GRANT SELECT ON `performance_schema`.* TO missingprivileges@localhost WITH GRANT OPTION");
session.runSql("SET SQL_LOG_BIN=1");
var result = session.runSql("select COUNT(*) from mysql.user where user='missingprivileges' and host='localhost'");
var row = result.fetchOne();
print("Number of accounts: "+ row[0] + "\n");
session.close();

//@ Dba: configureLocalInstance not enough privileges 1
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER
dba.configureLocalInstance('missingprivileges@localhost:' + __mysql_sandbox_port2);

//@ Dba: configureLocalInstance not enough privileges 2
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER
dba.configureLocalInstance('missingprivileges@localhost:' + __mysql_sandbox_port2,
    {clusterAdmin: "missingprivileges", clusterAdminPassword:""});

//@ Dba: configureLocalInstance not enough privileges 3
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
dba.configureLocalInstance('localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf'});

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
// Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configureLocalInstance('mydba@localhost:' + __mysql_sandbox_port2);

//@<OUT> Dba: configureLocalInstance create existing valid admin user
// Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configureLocalInstance('mydba@localhost:' + __mysql_sandbox_port2);

//@ Dba: remove needed privilege (REPLICATION SLAVE) from created admin user
connect_to_sandbox([__mysql_sandbox_port2]);
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("REVOKE REPLICATION SLAVE ON *.* FROM 'dba_test'@'%'");
session.runSql("SET SQL_LOG_BIN=1");
session.close();

//@<OUT> Dba: configureLocalInstance create existing invalid admin user
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

connect_to_sandbox([__mysql_sandbox_port1]);

//@# Dba: getCluster errors
var c2 = dba.getCluster(5);
var c2 = dba.getCluster('', 5);
var c2 = dba.getCluster('');
var c2 = dba.getCluster('#');
var c2 = dba.getCluster("over40chars_12345678901234567890123456789");

//@<OUT> Dba: getCluster with interaction
var c2 = dba.getCluster('devCluster');
c2;

//@<OUT> Dba: getCluster with interaction (default)
var c3 = dba.getCluster();
c3;
