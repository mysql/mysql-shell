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
validateMember(members, 'checkInstanceConfig');
validateMember(members, 'stopSandboxInstance');
validateMember(members, 'configLocalInstance');
validateMember(members, 'verbose');

//@# Dba: createCluster errors
var c1 = dba.createCluster();
var c1 = dba.createCluster(5);
var c1 = dba.createCluster('', 5);
var c1 = dba.createCluster('devCluster', 'bla');
var c1 = dba.createCluster('devCluster', {invalid:1, another:2});

//@# Dba: createCluster succeed
var c1 = dba.createCluster('devCluster');
print(c1)

//@# Dba: createCluster already exist
var c1 = dba.createCluster('devCluster');

//@# Dba: checkInstanceConfig errors
dba.checkInstanceConfig('localhost:' + __mysql_sandbox_port1);
dba.checkInstanceConfig('sample@localhost:' + __mysql_sandbox_port1);
var result = dba.checkInstanceConfig('root:root@localhost:' + __mysql_sandbox_port1);

//@ Dba: checkInstanceConfig ok1
var uri2 = 'root:root@localhost:' + __mysql_sandbox_port2;
var result = dba.checkInstanceConfig(uri2);
print (result.status)

//@ Dba: checkInstanceConfig ok2
var result = dba.checkInstanceConfig('root@localhost:' + __mysql_sandbox_port2, {password:'root'});
print (result.status)

//@ Dba: checkInstanceConfig ok3
var result = dba.checkInstanceConfig('root@localhost:' + __mysql_sandbox_port2, {dbPassword:'root'});
print (result.status)

//@<OUT> Dba: checkInstanceConfig report with errors
dba.checkInstanceConfig(uri2, {mycnfPath:'mybad.cnf'});

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
