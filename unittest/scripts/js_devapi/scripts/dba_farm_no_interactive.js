// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({enforce:true});
var farmPassword = 'testing';
//@ Farm: validating members
var farm = dba.createFarm('devFarm', farmPassword);

var members = dir(farm);

print("Farm Members:", members.length);
validateMember(members, 'name');
validateMember(members, 'getName');
validateMember(members, 'adminType');
validateMember(members, 'getAdminType');
validateMember(members, 'addSeedInstance');
validateMember(members, 'addInstance');
validateMember(members, 'removeInstance');
validateMember(members, 'getReplicaSet');

//@ Farm: addSeedInstance
// Added this to enable addInstance, full testing of addSeedInstance is needed
farm.addSeedInstance(farmPassword, {host: __host, port:__mysql_port}, __pwd);

//@# Farm: addInstance errors
farm.addInstance()
farm.addInstance(5,6,7,1)
farm.addInstance(5, 5)
farm.addInstance('', 5)
farm.addInstance(farmPassword, 5)
farm.addInstance(farmPassword, {host: __host, schema: 'abs', user:"sample", authMethod:56});
farm.addInstance(farmPassword, {port: __port});
farm.addInstance(farmPassword, {host: __host, port:__mysql_port}, __pwd);

// Cleanup
dba.dropFarm('devFarm', {dropDefaultReplicaSet: true});
