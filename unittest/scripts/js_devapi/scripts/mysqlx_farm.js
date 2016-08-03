// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
var mysqlx = require('mysqlx');

//@ Farm: validating members
var myAdmin = mysqlx.getAdminSession(__uripwd);
var farm = myAdmin.createFarm('devFarm');

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
farm.addSeedInstance({host: '192.168.1.1'});

//@# Farm: addInstance errors
farm.addInstance();
farm.addInstance(5,6);
farm.addInstance(5);
farm.addInstance({host: '192.168.1.1', schema: 'abs'});
farm.addInstance({host: '192.168.1.1', user: 'abs'});
farm.addInstance({host: '192.168.1.1', password: 'abs'});
farm.addInstance({host: '192.168.1.1', authMethod: 'abs'});
farm.addInstance({port: 33060});
farm.addInstance('');

//@# Farm: addInstance
farm.addInstance('192.168.1.1:33060');
farm.addInstance({host: '192.168.1.1', port: 1234});

// Cleanup
myAdmin.dropFarm('devFarm', {dropDefaultReplicaSet: true});
myAdmin.close();
