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
var c1 = dba.createCluster('devCluster', {ipWhitelist: "  "});

//@<OUT> Dba: createCluster with interaction
if (__have_ssl)
  var c1 = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED'})
else
  var c1 = dba.createCluster('devCluster')

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

//@<OUT> Dba: configureLocalInstance error 2
dba.configureLocalInstance('localhost:' + __mysql_port);

//@<OUT> Dba: configureLocalInstance error 3
dba.configureLocalInstance('localhost:' + __mysql_sandbox_port1);

//@<OUT> Dba: configureLocalInstance updating config file
dba.configureLocalInstance('localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf'});

//@# Dba: getCluster errors
var c2 = dba.getCluster(5);
var c2 = dba.getCluster('', 5);
var c2 = dba.getCluster('');

//@<OUT> Dba: getCluster with interaction
var c2 = dba.getCluster('devCluster');
c2;

//@<OUT> Dba: getCluster with interaction (default)
var c3 = dba.getCluster();
c3;
