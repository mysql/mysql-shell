// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({enforce:true});

var ClusterPassword = 'testing';
//@ Cluster: validating members
var Cluster = dba.createCluster('devCluster', ClusterPassword);
Cluster.addSeedInstance(ClusterPassword, {host: __host, port:__mysql_port}, __pwd);
var rset = Cluster.getReplicaSet();

var members = dir(rset);

print("Replica Set Members:", members.length);
validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');

//@# Cluster: addInstance errors
rset.addInstance()
rset.addInstance(5,6,7,1)
rset.addInstance(5, 5)
rset.addInstance('', 5)
rset.addInstance(ClusterPassword, 5)
rset.addInstance(ClusterPassword, {host: __host, schema: 'abs', user:"sample", authMethod:56});
rset.addInstance(ClusterPassword, {port: __port});
rset.addInstance(ClusterPassword, {host: __host, port:__mysql_port}, __pwd);

// Cleanup
dba.dropCluster('devCluster', {dropDefaultReplicaSet: true});
