// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
var mysqlx = require('mysqlx');

//@ Farm: validating members
var myAdmin = mysqlx.getAdminSession(__uripwd);
var farm = myAdmin.createFarm('devFarm');
farm.addSeedInstance({host: '192.168.1.1'});
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
rset.addInstance({host: '192.168.1.1', schema: 'abs'});
rset.addInstance({host: '192.168.1.1', user: 'abs'});
rset.addInstance({host: '192.168.1.1', password: 'abs'});
rset.addInstance({host: '192.168.1.1', authMethod: 'abs'});
rset.addInstance({port: 33060});
rset.addInstance('');

//@# Farm: addInstance
rset.addInstance('192.168.1.1:33060');
rset.addInstance({host: '192.168.1.1', port: 1234});

// Cleanup
myAdmin.dropFarm('devFarm', {dropDefaultReplicaSet: true});
myAdmin.close();
