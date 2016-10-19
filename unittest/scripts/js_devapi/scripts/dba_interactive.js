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
validateMember(members, 'validateInstance');
validateMember(members, 'stopSandboxInstance');
validateMember(members, 'configureLocalInstance');
validateMember(members, 'verbose');

//@# Dba: createCluster errors
var c1 = dba.createCluster();
var c1 = dba.createCluster(1,2,3,4);
var c1 = dba.createCluster(5);
var c1 = dba.createCluster('');
var c1 = dba.createCluster('devCluster', {invalid:1, another:2});

//@<OUT> Dba: createCluster with interaction
var c1 = dba.createCluster('devCluster');

//@ Dba: validateInstance error
dba.validateInstance('localhost:' + __mysql_sandbox_port1);

//@<OUT> Dba: validateInstance ok 1
dba.validateInstance('localhost:' + __mysql_sandbox_port2);

//@<OUT> Dba: validateInstance ok 2
dba.validateInstance('localhost:' + __mysql_sandbox_port2, {password:'root'});

//@<OUT> Dba: validateInstance report with errors
var uri2 = 'localhost:' + __mysql_sandbox_port2;
var res = dba.validateInstance(uri2, {mycnfPath:'mybad.cnf'});

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
