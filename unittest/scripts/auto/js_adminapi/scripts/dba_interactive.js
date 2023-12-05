// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>

//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
// ensure my.cnf file is saved/restored for replay in recording mode
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

testutil.createFile("mybad.cnf", "[mysqld]\ngtid_mode = OFF\n");

shell.options.useWizards=true;

shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});

//@<> Dba: validating members
validateMembers(dba, [
    'checkInstanceConfiguration',
    'configureInstance',
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

var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED'});
print(c1);

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

//@ Dba: create cluster using a non existing user that authenticates as another user (BUG#26979375) {enable_bug_26979375}
// Clear super read_only
session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'test_user'@'%'");
session.runSql("GRANT ALL PRIVILEGES ON *.* to 'test_user'@'%' WITH GRANT OPTION");
session.runSql("SET sql_log_bin = 1");
session.close();

shell.connect({scheme:'mysql', host: "127.0.0.1", port: __mysql_sandbox_port1, user: 'test_user', password: ''});
c1 = dba.createCluster("devCluster");
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
var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED'});

//@ Dba: checkInstanceConfiguration in a cluster member
testutil.expectPassword("*", "root");
dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port1);

//@<OUT> Dba: checkInstanceConfiguration ok 1
testutil.expectPassword("*", "root");
dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port2);

//@<OUT> Dba: checkInstanceConfiguration ok 2
testutil.expectPassword("*", "root");
dba.checkInstanceConfiguration('root:root@localhost:' + __mysql_sandbox_port2);

//@<OUT> Dba: checkInstanceConfiguration report with errors
var uri2 = 'root@localhost:' + __mysql_sandbox_port2;
var res = dba.checkInstanceConfiguration(uri2, {mycnfPath:'mybad.cnf'});

session.close();

//@ createCluster() should fail if user does not have global GRANT OPTION
shell.connect(__sandbox_uri2);

session.runSql("SET sql_log_bin = 0");
session.runSql("CREATE USER 'no_global_grant'@'%'");
session.runSql("GRANT ALL PRIVILEGES on *.* to 'no_global_grant'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'no_global_grant', password: ''});
dba.createCluster("cluster");
session.close();

shell.connect(__sandbox_uri2);
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP user 'no_global_grant'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

shell.connect(__sandbox_uri1);

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
//@<> Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
