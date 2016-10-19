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
validateMember(members, 'validateInstance');
validateMember(members, 'stopSandboxInstance');
validateMember(members, 'configureLocalInstance');
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

//@# Dba: validateInstance errors
dba.validateInstance('localhost:' + __mysql_sandbox_port1);
dba.validateInstance('sample@localhost:' + __mysql_sandbox_port1);
var result = dba.validateInstance('root:root@localhost:' + __mysql_sandbox_port1);

//@ Dba: validateInstance ok1
var uri2 = 'root:root@localhost:' + __mysql_sandbox_port2;
var result = dba.validateInstance(uri2);
print (result.status)

//@ Dba: validateInstance ok2
var result = dba.validateInstance('root@localhost:' + __mysql_sandbox_port2, {password:'root'});
print (result.status)

//@ Dba: validateInstance ok3
var result = dba.validateInstance('root@localhost:' + __mysql_sandbox_port2, {dbPassword:'root'});
print (result.status)

//@<OUT> Dba: validateInstance report with errors
dba.validateInstance(uri2, {mycnfPath:'mybad.cnf'});

//@# Dba: configureLocalInstance errors
dba.configureLocalInstance('someotherhost:' + __mysql_sandbox_port1);
dba.configureLocalInstance('localhost:' + __mysql_sandbox_port1);
dba.configureLocalInstance('sample@localhost:' + __mysql_sandbox_port1);
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1, {password:'root'});
dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port1, {password:'root', mycnfPath:'mybad.cnf'});

//@<OUT> Dba: configureLocalInstance updating config file
dba.configureLocalInstance(uri2, {mycnfPath:'mybad.cnf'});

//@<OUT> Dba: configureLocalInstance report fixed 1
var result = dba.configureLocalInstance(uri2, {mycnfPath:'mybad.cnf'});
print (result.status)

//@<OUT> Dba: configureLocalInstance report fixed 2
var result = dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf', password:'root'});
print (result.status)

//@<OUT> Dba: configureLocalInstance report fixed 3
var result = dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port2, {mycnfPath:'mybad.cnf', dbPassword:'root'});
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
