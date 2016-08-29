// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({ enforce: true });

//@ Session: validating members
var members = dir(dba);

print("Session Members:", members.length);
validateMember(members, 'defaultCluster');
validateMember(members, 'getDefaultCluster');
validateMember(members, 'createCluster');
validateMember(members, 'dropCluster');
validateMember(members, 'getCluster');
validateMember(members, 'dropMetadataSchema');
validateMember(members, 'resetSession');
validateMember(members, 'validateInstance');
validateMember(members, 'deployLocalInstance');

//@# Dba: createCluster errors
var Cluster = dba.createCluster();
var Cluster = dba.createCluster(5);
var Cluster = dba.createCluster('', 5);
var Cluster = dba.createCluster('devCluster');
var Cluster = dba.createCluster('devCluster', 'password');
var Cluster = dba.createCluster('devCluster', 'password');

//@ Dba: createCluster
print(Cluster)

//@# Dba: getCluster errors
var Cluster = dba.getCluster();
var Cluster = dba.getCluster(5);
var Cluster = dba.getCluster('', 5);
var Cluster = dba.getCluster('');
var Cluster = dba.getCluster('devCluster');

//@ Dba: getCluster
print(Cluster);

//@ Dba: addSeedInstance
Cluster.addSeedInstance('testing', {host: __host, port:__mysql_port}, __pwd);

//@# Dba: dropCluster errors
var Cluster = dba.dropCluster();
var Cluster = dba.dropCluster(5);
var Cluster = dba.dropCluster('');
var Cluster = dba.dropCluster('sample', 5);
var Cluster = dba.dropCluster('sample', {}, 5);
var Cluster = dba.dropCluster('sample');
var Cluster = dba.dropCluster('devCluster');

//@ Dba: dropCluster
dba.dropCluster('devCluster', { dropDefaultReplicaSet: true });
