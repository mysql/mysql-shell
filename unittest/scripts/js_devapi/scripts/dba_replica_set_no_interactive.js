// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({enforce:true});

var farmPassword = 'testing';
//@ Farm: validating members
var farm = dba.createFarm('devFarm', farmPassword);
farm.addSeedInstance(farmPassword, {host: __host, port:__mysql_port}, __pwd);
var rset = farm.getReplicaSet();

var members = dir(rset);

print("Replica Set Members:", members.length);
validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');

//@# Farm: addInstance errors
rset.addInstance()
rset.addInstance(5,6,7,1)
rset.addInstance(5, 5)
rset.addInstance('', 5)
rset.addInstance(farmPassword, 5)
rset.addInstance(farmPassword, {host: __host, schema: 'abs', user:"sample", authMethod:56});
rset.addInstance(farmPassword, {port: __port});
rset.addInstance(farmPassword, {host: __host, port:__mysql_port}, __pwd);

// Cleanup
dba.dropFarm('devFarm', {dropDefaultReplicaSet: true});
