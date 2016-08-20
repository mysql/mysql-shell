// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script

dba.dropMetadataSchema({ enforce: true });
var farm = dba.createFarm('devFarm', 'testing');

//@ Farm: validating members

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

//@ Farm: addInstance, no seed instance answer no
farm.addInstance();

//@ Farm: addInstance, no seed instance answer yes
farm.addInstance(5);

//@ Farm: addInstance, ignore invalid attributes no ignore
farm.addInstance({host: __host, data:'sample', port:__mysql_port, whatever:5}, __pwd);

//@ Farm: addInstance, ignore invalid attributes ignore
farm.addInstance({host: __host, data:'sample', port:__mysql_port, whatever:5}, __pwd);

//@ Farm: addSeedInstance, it already initialized, answer no
farm.addSeedInstance({host: __host, port:__mysql_port}, __pwd);

//@ Farm: addSeedInstance, it already initialized, answer yes
farm.addSeedInstance({host: __host, port:__mysql_port}, __pwd);

// Cleanup
dba.dropFarm('devFarm', {dropDefaultReplicaSet: true});
