// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script

dba.dropMetadataSchema({ enforce: true });
var ClusterPassword = 'testing';
var Cluster = dba.createCluster('devCluster', ClusterPassword);

//@ Cluster: validating members

var members = dir(Cluster);

print("Cluster Members:", members.length);
validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'adminType');
validateMember(members, 'getAdminType');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');
validateMember(members, 'getReplicaSet');

//@ Cluster: addInstance, no seed instance answer no
Cluster.addInstance();

//@ Cluster: addInstance, no seed instance answer yes
Cluster.addInstance(5);

//@ Cluster: addInstance, ignore invalid attributes no ignore
Cluster.addInstance({host: "127.0.0.1", data:'sample', port:__mysql_sandbox_port1, whatever:5}, "root");

//@ Cluster: addInstance, ignore invalid attributes ignore
Cluster.addInstance({host: "127.0.0.1", data:'sample', port:__mysql_sandbox_port1, whatever:5}, "root");

//@ Cluster: addSeedInstance, it already initialized, answer no
Cluster.addSeedInstance({host: "127.0.0.1", port:__mysql_sandbox_port1}, "root");

//@ Cluster: addSeedInstance, it already initialized, answer yes
Cluster.addSeedInstance({host: "127.0.0.1", port:__mysql_sandbox_port1}, "root");

// Cleanup
dba.dropCluster('devCluster', {dropDefaultReplicaSet: true, masterKey: ClusterPassword});
