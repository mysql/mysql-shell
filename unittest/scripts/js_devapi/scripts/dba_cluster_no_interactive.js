// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({enforce:true});
var ClusterPassword = 'testing';
//@ Cluster: validating members
var Cluster = dba.createCluster('devCluster', ClusterPassword);

var members = dir(Cluster);

print("Cluster Members:", members.length);
validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'adminType');
validateMember(members, 'getAdminType');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');
validateMember(members, 'getReplicaSet');
validateMember(members, 'rejoinInstance');
validateMember(members, 'describe');
validateMember(members, 'status');

//@ Cluster: removeInstance
Cluster.removeInstance({host: "127.0.0.1", port:__mysql_sandbox_port1});

//@ Cluster: addInstance
Cluster.addInstance({dbUser: "root", host: "127.0.0.1", port:__mysql_sandbox_port1}, "root");

//@# Cluster: addInstance errors
Cluster.addInstance()
Cluster.addInstance(5,6,7,1)
Cluster.addInstance(5, 5)
Cluster.addInstance('', 5)
Cluster.addInstance( 5)
Cluster.addInstance({host: "127.0.0.1", schema: 'abs', user:"sample", authMethod:56});
Cluster.addInstance({port: __mysql_sandbox_port1});
Cluster.addInstance({host: "127.0.0.1", port:__mysql_sandbox_port1}, "root");

// Cleanup
dba.dropCluster('devCluster', {dropDefaultReplicaSet: true});
