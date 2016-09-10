// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMember and validateNotMember are defined on the setup script
var ClusterPassword = 'testing';

//@ Session: validating members
var members = dir(dba);

print("Session Members:", members.length);
validateMember(members, 'createCluster');
validateMember(members, 'deleteLocalInstance');
validateMember(members, 'deployLocalInstance');
validateMember(members, 'dropCluster');
validateMember(members, 'dropMetadataSchema');
validateMember(members, 'getCluster');
validateMember(members, 'help');
validateMember(members, 'killLocalInstance');
validateMember(members, 'resetSession');
validateMember(members, 'startLocalInstance');
validateMember(members, 'validateInstance');
validateMember(members, 'stopLocalInstance');

//@# Dba: createCluster errors
var c1 = dba.createCluster();
var c1 = dba.createCluster(5);
var c1 = dba.createCluster('', 5);
var c1 = dba.createCluster('devCluster');
var c1 = dba.createCluster('devCluster', ClusterPassword);
var c1 = dba.createCluster('devCluster', ClusterPassword);

//@ Dba: createCluster
print(c1)

//@# Dba: getCluster errors
var c2 = dba.getCluster();
var c2 = dba.getCluster(5);
var c2 = dba.getCluster('', 5);
var c2 = dba.getCluster('');
var c2 = dba.getCluster('devCluster');
var c2 = dba.getCluster('devCluster', {masterKey:ClusterPassword});

//@ Dba: getCluster
print(c2);

//@# Dba: dropCluster errors
dba.dropCluster();
dba.dropCluster(5);
dba.dropCluster('');
dba.dropCluster('sample', 5);
dba.dropCluster('sample', {}, 5);
dba.dropCluster('sample');
dba.dropCluster('devCluster');
