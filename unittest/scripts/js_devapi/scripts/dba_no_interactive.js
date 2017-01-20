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
validateMember(members, 'configLocalInstance');
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

//@# Dba: createCluster succeed
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

//@# Dba: configLocalInstance errors
dba.configLocalInstance('someotherhost:' + __mysql_sandbox_port1);
dba.configLocalInstance('localhost:' + __mysql_sandbox_port1);
dba.configLocalInstance('sample@localhost:' + __mysql_sandbox_port1);
dba.configLocalInstance('root@localhost:' + __mysql_sandbox_port1, {password:'root'});
dba.configLocalInstance('root@localhost:' + __mysql_sandbox_port1, {password:'root', mycnfPath:'mybad.cnf'});

//@<OUT> Dba: configLocalInstance updating config file
dba.configLocalInstance(uri2, {mycnfPath:'mybad.cnf'});

//@<OUT> Dba: configLocalInstance report fixed 1
var result = dba.configLocalInstance(uri2, {mycnfPath:'mybad.cnf'});
print (result.status)

//@<OUT> Dba: configLocalInstance report fixed 2
var result = dba.configLocalInstance('root@localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf', password:'root'});
print (result.status)

//@<OUT> Dba: configLocalInstance report fixed 3
var result = dba.configLocalInstance('root@localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf', dbPassword:'root'});
print (result.status)

//@# Dba: getCluster errors
var c2 = dba.getCluster();
var c2 = dba.getCluster(5);
var c2 = dba.getCluster('', 5);
var c2 = dba.getCluster('');
var c2 = dba.getCluster('devCluster');
var c2 = dba.getCluster('devCluster');

//@ Dba: getCluster
print(c2);
