// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMember and validateNotMember are defined on the setup script

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
var c1 = dba.createCluster(5);
var c1 = dba.createCluster('', 5);
var c1 = dba.createCluster('devCluster', 'bla');
var c1 = dba.createCluster('devCluster', {invalid:1, another:2});
var c1 = dba.createCluster('devCluster', {memberSslMode: 'foo'});
var c1 = dba.createCluster('devCluster', {memberSslMode: ''});
var c1 = dba.createCluster('devCluster', {adoptFromGR: true, memberSslMode: 'AUTO'});
var c1 = dba.createCluster('devCluster', {adoptFromGR: true, memberSslMode: 'REQUIRED'});
var c1 = dba.createCluster('devCluster', {adoptFromGR: true, memberSslMode: 'DISABLED'});
var c1 = dba.createCluster('devCluster', {ipWhitelist: " "});


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
    var c1 = dba.createCluster('devCluster');
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

//@ Dba: createCluster success
if (__have_ssl)
  var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED'})
else
  var c1 = dba.createCluster('devCluster')
print(c1)

//@# Dba: createCluster already exist
var c1 = dba.createCluster('devCluster');

// TODO: add multi-master unit-tests

//@# Dba: checkInstanceConfiguration errors
dba.checkInstanceConfiguration('localhost:' + __mysql_sandbox_port1);
dba.checkInstanceConfiguration('sample@localhost:' + __mysql_sandbox_port1);
var result = dba.checkInstanceConfiguration('root:root@localhost:' + __mysql_sandbox_port1);

//@ Dba: checkInstanceConfiguration ok1
var uri2 = 'root:root@localhost:' + __mysql_sandbox_port2;
var result = dba.checkInstanceConfiguration(uri2);
print (result.status)

//@ Dba: checkInstanceConfiguration ok2
var result = dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port2, {password:'root'});
print (result.status)

//@ Dba: checkInstanceConfiguration ok3
var result = dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port2, {dbPassword:'root'});
print (result.status)

//@<OUT> Dba: checkInstanceConfiguration report with errors
dba.checkInstanceConfiguration(uri2, {mycnfPath:'mybad.cnf'});

//@# Dba: configureLocalInstance errors
dba.configureLocalInstance('someotherhost:' + __mysql_sandbox_port1);
dba.configureLocalInstance('localhost:' + __mysql_sandbox_port1);
dba.configureLocalInstance('sample@localhost:' + __mysql_sandbox_port1);
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1, {password:'root'});

//@<OUT> Dba: configureLocalInstance updating config file
dba.configureLocalInstance(uri2, {mycnfPath:'mybad.cnf'});

//@ Dba: configureLocalInstance report fixed 1
var result = dba.configureLocalInstance(uri2, {mycnfPath:'mybad.cnf'});
print (result.status)

//@ Dba: configureLocalInstance report fixed 2
var result = dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf', password:'root'});
print (result.status)

//@ Dba: configureLocalInstance report fixed 3
var result = dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf', dbPassword:'root'});
print (result.status)

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

//@ Dba: configureLocalInstance not enough privileges
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

//@<OUT> Dba: configureLocalInstance create existing invalid admin user
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

connect_to_sandbox([__mysql_sandbox_port1]);

//@# Dba: getCluster errors
var c2 = dba.getCluster();
var c2 = dba.getCluster(5);
var c2 = dba.getCluster('', 5);
var c2 = dba.getCluster('');
var c2 = dba.getCluster('#');
var c2 = dba.getCluster("over40chars_12345678901234567890123456789");
var c2 = dba.getCluster('devCluster');
var c2 = dba.getCluster('devCluster');

//@ Dba: getCluster
print(c2);
