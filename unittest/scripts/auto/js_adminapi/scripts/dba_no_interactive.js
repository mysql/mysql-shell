// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>

//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
// ensure my.cnf file is saved/restored for replay in recording mode
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);

testutil.createFile("mybad.cnf", "[mysqld]\ngtid_mode = OFF\n");

shell.options.useWizards=false;

shell.connect("mysql://root:root@localhost:" + __mysql_sandbox_port1);

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
dba.createCluster('1invalidN4me');

//@ Dba: create cluster with memberSslMode AUTO succeed
var c1 = dba.createCluster("devCluster", {memberSslMode: 'AUTO'});
c1

//@ Dba: dissolve cluster created with memberSslMode AUTO
c1.dissolve({force:true});

//@ Dba: createCluster success
var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED'});
print(c1)

//@# Dba: createCluster already exist
var c1 = dba.createCluster('devCluster');

//@# Dba: checkInstanceConfiguration errors
dba.checkInstanceConfiguration('root@localhost:' + __mysql_sandbox_port1);
dba.checkInstanceConfiguration('sample@localhost:' + __mysql_sandbox_port1);

//@ Dba: checkInstanceConfiguration ok1
var uri2 = 'root:root@localhost:' + __mysql_sandbox_port2;
dba.checkInstanceConfiguration(uri2);

//@ Dba: checkInstanceConfiguration ok2
dba.checkInstanceConfiguration('root:root@localhost:' + __mysql_sandbox_port2);

//@<OUT> Dba: checkInstanceConfiguration report with errors
dba.checkInstanceConfiguration(uri2, {mycnfPath:'mybad.cnf'});

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
// Drop user
session.runSql("SET sql_log_bin = 0");
session.runSql("DROP user 'no_global_grant'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

shell.connect(__sandbox_uri1);

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

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
