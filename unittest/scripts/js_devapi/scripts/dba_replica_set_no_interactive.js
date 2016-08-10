// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({enforce:true});
//@ Farm: validating members
var farm = dba.createFarm('devFarm', 'testing');
farm.addSeedInstance({host: __host});
var rset = farm.getReplicaSet();

var members = dir(rset);

print("Replica Set Members:", members.length);
validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');

//@# Farm: addInstance errors
rset.addInstance()
rset.addInstance(5,6)
rset.addInstance(5)
rset.addInstance({host: __host, schema: 'abs'});
rset.addInstance({host: __host, user: 'abs'});
rset.addInstance({host: __host, password: 'abs'});
rset.addInstance({host: __host, authMethod: 'abs'});
rset.addInstance({port: __port});
rset.addInstance('');

//@# Farm: addInstance
rset.addInstance(__host_port);

// Cleanup
dba.dropFarm('devFarm', {dropDefaultReplicaSet: true});
