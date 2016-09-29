// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMember and validateNotMember are defined on the setup script
var ClusterPassword = 'testing';

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
validateMember(members, 'verbose');

//@# Dba: createCluster errors
var c1 = dba.createCluster();
var c1 = dba.createCluster(5);
var c1 = dba.createCluster('', 5);
var c1 = dba.createCluster('devCluster');

//@# Dba: createCluster succeed
var c1 = dba.createCluster('devCluster', ClusterPassword);
print(c1)

//@# Dba: createCluster already exist
var c1 = dba.createCluster('devCluster', ClusterPassword);

//@# Dba: getCluster errors
var c2 = dba.getCluster();
var c2 = dba.getCluster(5);
var c2 = dba.getCluster('', 5);
var c2 = dba.getCluster('');
var c2 = dba.getCluster('devCluster');
var c2 = dba.getCluster('devCluster', {masterKey:ClusterPassword});

//@ Dba: getCluster
print(c2);
