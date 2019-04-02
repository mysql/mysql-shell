// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMember and validateNotMember are defined on the setup script
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
// ensure my.cnf file is saved/restored for replay in recording mode
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

shell.connect("mysql://root:root@localhost:" + __mysql_sandbox_port1);

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
validateMember(members, 'configureInstance');
validateMember(members, 'configureLocalInstance');
validateMember(members, 'verbose');
validateMember(members, 'rebootClusterFromCompleteOutage');

//@# Dba: createCluster errors
dba.createCluster();
dba.createCluster(5);
dba.createCluster('');
dba.createCluster('', 5);
dba.createCluster('devCluster', 'bla');
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
dba.createCluster('devCluster', {ipWhitelist: " "});
dba.createCluster('1invalidN4me');


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

//@ Dba: create cluster with memberSslMode AUTO succeed
var c1 = dba.createCluster("devCluster", {memberSslMode: 'AUTO', clearReadOnly: true});
c1

//@ Dba: dissolve cluster created with memberSslMode AUTO
c1.dissolve({force:true});

//@ Dba: createCluster success
if (__have_ssl)
  var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED', clearReadOnly: true});
else
  var c1 = dba.createCluster('devCluster', {memberSslMode: 'DISABLED', clearReadOnly: true});
print(c1)

//@# Dba: createCluster already exist
var c1 = dba.createCluster('devCluster');

//@# Dba: checkInstanceConfiguration errors
dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port1);
dba.checkInstanceConfiguration('sample@localhost:' + __mysql_sandbox_port1);
dba.checkInstanceConfiguration('root:root@localhost:' + __mysql_sandbox_port1);

//@ Dba: checkInstanceConfiguration ok1
var uri2 = 'root:root@localhost:' + __mysql_sandbox_port2;
dba.checkInstanceConfiguration(uri2);

//@ Dba: checkInstanceConfiguration ok2
dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port2, {PASSWORD:'root'});

//@<OUT> Dba: checkInstanceConfiguration report with errors
dba.checkInstanceConfiguration(uri2, {mycnfPath:'mybad.cnf'});

// Delete the config file so that configureLocal doesn't detect it as a sandbox and auto-picks the config file
// This test should actually be rewritten to use raw sandboxes, but no time for that at this moment
var mycnf1_path = testutil.getSandboxConfPath(__mysql_sandbox_port1);
testutil.cpfile(mycnf1_path, mycnf1_path+".bak");
testutil.rmfile(mycnf1_path);

//@# Dba: configureLocalInstance errors
// TODO: This test needs an actual remote instance
//dba.configureLocalInstance('sample:@someotherhost:' + __mysql_sandbox_port1);
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1);
dba.configureLocalInstance('sample@localhost:' + __mysql_sandbox_port1);
//@# Dba: configureLocalInstance errors 5.7 {VER(<8.0.11)}
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1, {password:'root'});
//@# Dba: configureLocalInstance errors 8.0 {VER(>=8.0.11)}
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1, {password:'root'});

// Restore config file
testutil.cpfile(mycnf1_path + ".bak", mycnf1_path);
testutil.rmfile(mycnf1_path + ".bak");

//@<OUT> Dba: configureLocalInstance updating config file
dba.configureLocalInstance(uri2, {mycnfPath:'mybad.cnf'});

//@ Dba: configureLocalInstance report fixed 1
dba.configureLocalInstance(uri2, {mycnfPath:'mybad.cnf'});

//@ Dba: configureLocalInstance report fixed 2
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf', password:'root'});

//@ Dba: Create user without all necessary privileges
// create user that has all permissions to admin a cluster but doesn't have
// the grant privileges for them, so it cannot be used to create viable accounts
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER

connect_to_sandbox([__mysql_sandbox_port2]);
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("CREATE USER missingprivileges@localhost");
session.runSql("GRANT SUPER, CREATE USER ON *.* TO missingprivileges@localhost");
session.runSql("GRANT SELECT ON `performance_schema`.* TO missingprivileges@localhost WITH GRANT OPTION");
session.runSql("SET SQL_LOG_BIN=1");
var result = session.runSql("select COUNT(*) from mysql.user where user='missingprivileges' and host='localhost'");
var row = result.fetchOne();
print("Number of accounts: "+ row[0] + "\n");
session.close();

//@# Dba: configureLocalInstance not enough privileges
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER
dba.configureLocalInstance('missingprivileges:@localhost:' + __mysql_sandbox_port2,
    {clusterAdmin: "missingprivileges", clusterAdminPassword:"",
     mycnfPath:__sandbox_dir + __mysql_sandbox_port2 + __path_splitter + 'my.cnf'});

//@ Dba: Show list of users to make sure the user missingprivileges@% was not created
// Regression for BUG#25614855 : CONFIGURELOCALINSTANCE URI USER WITHOUT
// PERMISSIONS, CREATES A WRONG NEW USER
connect_to_sandbox([__mysql_sandbox_port2]);
var result = session.runSql("select COUNT(*) from mysql.user where user='missingprivileges' and host='%'");
var row = result.fetchOne();
print("Number of accounts: "+ row[0] + "\n");
session.close();

//@ Dba: Delete created user and reconnect to previous sandbox
connect_to_sandbox([__mysql_sandbox_port2]);
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("DROP USER missingprivileges@localhost");
session.runSql("SET SQL_LOG_BIN=1");
var result = session.runSql("select COUNT(*) from mysql.user where user='missingprivileges' and host='localhost'");
var row = result.fetchOne();
print("Number of accounts: "+ row[0] + "\n");
session.close();

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
dba.configureLocalInstance('mydba:@localhost:' + __mysql_sandbox_port2,
    {clusterAdmin: "dba_test", clusterAdminPassword:"",
        mycnfPath:__sandbox_dir + __mysql_sandbox_port2 + __path_splitter + 'my.cnf'});

//@<OUT> Dba: configureLocalInstance create existing valid admin user
// Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configureLocalInstance('mydba:@localhost:' + __mysql_sandbox_port2,
    {clusterAdmin: "dba_test", clusterAdminPassword:"",
        mycnfPath:__sandbox_dir + __mysql_sandbox_port2 + __path_splitter + 'my.cnf'});

//@ Dba: remove needed privilege (REPLICATION SLAVE) from created admin user
connect_to_sandbox([__mysql_sandbox_port2]);
session.runSql("SET SQL_LOG_BIN=0");
session.runSql("REVOKE REPLICATION SLAVE ON *.* FROM 'dba_test'@'%'");
session.runSql("SET SQL_LOG_BIN=1");
session.close();

//@# Dba: configureLocalInstance create existing invalid admin user
// Regression for BUG#25519190 : CONFIGURELOCALINSTANCE() FAILS UNGRACEFUL IF CALLED TWICE
dba.configureLocalInstance('mydba:@localhost:' + __mysql_sandbox_port2,
    {clusterAdmin: "dba_test", clusterAdminPassword:"",
        mycnfPath:__sandbox_dir + __mysql_sandbox_port2 + __path_splitter + 'my.cnf'});

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

connect_to_sandbox([__mysql_sandbox_port2]);
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
var c2 = dba.getCluster();
var c2 = dba.getCluster(5);
var c2 = dba.getCluster('', {}, 5);
var c2 = dba.getCluster('', 5);
var c2 = dba.getCluster('');
var c2 = dba.getCluster('#');
var c2 = dba.getCluster("over40chars_12345678901234567890123456789");
var c2 = dba.getCluster('devCluster');

//@ Dba: getCluster
print(c2);

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
