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
validateMember(members, 'addSeedInstance');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');
validateMember(members, 'getReplicaSet');

//@ Cluster: addSeedInstance
// Added this to enable addInstance, full testing of addSeedInstance is needed
Cluster.addSeedInstance(ClusterPassword, {host: __host, port:__mysql_port}, __pwd);

//@# Cluster: addInstance errors
Cluster.addInstance()
Cluster.addInstance(5,6,7,1)
Cluster.addInstance(5, 5)
Cluster.addInstance('', 5)
Cluster.addInstance(ClusterPassword, 5)
Cluster.addInstance(ClusterPassword, {host: __host, schema: 'abs', user:"sample", authMethod:56});
Cluster.addInstance(ClusterPassword, {port: __port});
Cluster.addInstance(ClusterPassword, {host: __host, port:__mysql_port}, __pwd);

// Cleanup
dba.dropCluster('devCluster', {dropDefaultReplicaSet: true});
