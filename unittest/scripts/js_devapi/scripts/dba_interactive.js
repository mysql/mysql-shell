// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");
// ensure my.cnf file is saved/restored for replay in recording mode
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

// Tests below are expecting root@% to already exit
shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port2});
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("CREATE USER root@'%' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION");
session.runSql("SET SQL_LOG_BIN=1");

shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("CREATE USER root@'%' IDENTIFIED BY 'root'");
session.runSql("GRANT ALL ON *.* to root@'%' WITH GRANT OPTION");
session.runSql("SET SQL_LOG_BIN=1");

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
    var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED', clearReadOnly: true});
else
    var c1 = dba.createCluster('devCluster', {memberSslMode: 'DISABLED', clearReadOnly: true});
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

//@ Dba: create cluster using a non existing user that authenticates as another user (BUG#26979375)
// Clear super read_only
session.runSql("set GLOBAL super_read_only = 0");
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'test_user'@'%'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'test_user'@'%' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");
session.close();

shell.connect({scheme:'mysql', host: "127.0.0.1", port: __mysql_sandbox_port1, user: 'test_user', password: ''});
c1 = dba.createCluster("devCluster", {clearReadOnly: true});
c1

//@ Dba: dissolve cluster created using a non existing user that authenticates as another user (BUG#26979375)
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
if (__have_ssl)
  var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED', clearReadOnly: true});
else
  var c1 = dba.createCluster('devCluster', {memberSslMode: 'DISABLED', clearReadOnly: true});

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

//@ Dba: configureLocalInstance create existing invalid admin user
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
var c2 = dba.getCluster('x', 5, 6);
var c2 = dba.getCluster(null, 5);
var c2 = dba.getCluster('');
var c2 = dba.getCluster('#');
var c2 = dba.getCluster("over40chars_12345678901234567890123456789");

//@<OUT> Dba: getCluster with interaction
var c2 = dba.getCluster('devCluster');
c2;

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
