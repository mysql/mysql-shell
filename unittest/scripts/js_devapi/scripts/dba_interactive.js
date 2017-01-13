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
validateMember(members, 'checkInstanceConfig');
validateMember(members, 'stopSandboxInstance');
validateMember(members, 'configLocalInstance');
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

//@ Dba: checkInstanceConfig error
dba.checkInstanceConfig('localhost:' + __mysql_sandbox_port1);

//@<OUT> Dba: checkInstanceConfig ok 1
dba.checkInstanceConfig('localhost:' + __mysql_sandbox_port2);

//@<OUT> Dba: checkInstanceConfig ok 2
dba.checkInstanceConfig('localhost:' + __mysql_sandbox_port2, {password:'root'});

//@<OUT> Dba: checkInstanceConfig report with errors
var uri2 = 'localhost:' + __mysql_sandbox_port2;
var res = dba.checkInstanceConfig(uri2, {mycnfPath:'mybad.cnf'});

//@ Dba: configLocalInstance error 1
dba.configLocalInstance('someotherhost:' + __mysql_sandbox_port1);

//@<OUT> Dba: configLocalInstance error 2
dba.configLocalInstance('localhost:' + __mysql_port);

//@<OUT> Dba: configLocalInstance error 3
dba.configLocalInstance('localhost:' + __mysql_sandbox_port1);

//@<OUT> Dba: configLocalInstance updating config file
dba.configLocalInstance('localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf'});

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
