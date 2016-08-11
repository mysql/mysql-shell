// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({enforce:true});

//@ Farm: validating members
var farm = dba.createFarm('devFarm', 'testing');

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
farm.addSeedInstance({host: __host, port:__mysql_port}, __pwd);

//@# Farm: addInstance errors
farm.addInstance()
farm.addInstance(5,6,7)
farm.addInstance(5)
farm.addInstance({host: __host, schema: 'abs', user:"sample", authMethod:56});
farm.addInstance({port: __port});
farm.addInstance({host: __host, port:__mysql_port}, __pwd);

// Cleanup
dba.dropFarm('devFarm', {dropDefaultReplicaSet: true});
