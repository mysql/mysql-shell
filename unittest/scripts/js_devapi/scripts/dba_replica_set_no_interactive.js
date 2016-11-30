// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({force:true});

//@ ReplicaSet: validating members
if (__have_ssl)
  var Cluster = dba.createCluster('devCluster')
else
  var Cluster = dba.createCluster('devCluster', {ssl: false})

var rset = Cluster.getReplicaSet();

var members = dir(rset);

print("Replica Set Members:", members.length);
validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');
validateMember(members, 'help');
validateMember(members, 'rejoinInstance');

//@# ReplicaSet: addInstance errors
rset.addInstance()
rset.addInstance(5,6,7,1)
rset.addInstance(5, 5)
rset.addInstance('', 5)
rset.addInstance( 5)
rset.addInstance({host: "127.0.0.1", schema: 'abs', user:"sample", authMethod:56});
rset.addInstance({port: __mysql_sandbox_port1});
rset.addInstance({host: "127.0.0.1", port:__mysql_sandbox_port1}, "root");

// Cleanup
dba.dropCluster('devCluster', {dropDefaultReplicaSet: true});
