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
farm.addSeedInstance(__host_port);

//@# Farm: addInstance errors
farm.addInstance();
farm.addInstance(5,6);
farm.addInstance(5);
farm.addInstance({host: __host, schema: 'abs'});
farm.addInstance({host: __host, user: 'abs'});
farm.addInstance({host: __host, password: 'abs'});
farm.addInstance({host: __host, authMethod: 'abs'});
farm.addInstance({port: __port});
farm.addInstance('');

//@# Farm: addInstance
farm.addInstance(__host_port);
farm.addInstance({host: __host, port: __port});

// Cleanup
dba.dropFarm('devFarm', {dropDefaultReplicaSet: true});
