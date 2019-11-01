// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
dba.dropMetadataSchema({force:true});

//@<> ReplicaSet: validating members
var Cluster = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED'})

var rset = Cluster.getReplicaSet();

validatemembers(rset, [
  'name',
  'getName',
  'addInstance',
  'removeInstance',
  'help',
  'rejoinInstance'])

//@# ReplicaSet: addInstance errors
rset.addInstance()
rset.addInstance(5,6,7,1)
rset.addInstance(5, 5)
rset.addInstance('', 5)
rset.addInstance( 5)
rset.addInstance({host: "127.0.0.1", schema: 'abs', user:"sample", "auth-method":56});
rset.addInstance({port: __mysql_sandbox_port1});
rset.addInstance({host: "127.0.0.1", port:__mysql_sandbox_port1}, "root");

// Cleanup
dba.dropCluster('devCluster', {dropDefaultReplicaSet: true});
