// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
var mysqlx = require('mysqlx');

//@ Session: validating members
var myAdmin = mysqlx.getAdminSession(__uripwd);
var members = dir(myAdmin);

print("Session Members:", members.length);
validateMember(members, 'uri');
validateMember(members, 'defaultFarm');
validateMember(members, 'getUri');
validateMember(members, 'getDefaultFarm');
validateMember(members, 'isOpen');
validateMember(members, 'createFarm');
validateMember(members, 'dropFarm');
validateMember(members, 'getFarm');
validateMember(members, 'close');

//@# AdminSession: createFarm errors
var farm = myAdmin.createFarm();
var farm = myAdmin.createFarm(5);
var farm = myAdmin.createFarm('', 5);
var farm = myAdmin.createFarm('');
var farm = myAdmin.createFarm('devFarm', 'password');
var farm = myAdmin.createFarm('devFarm', 'password');

//@ AdminSession: createFarm
print (farm)

//@# AdminSession: getFarm errors
var farm = myAdmin.getFarm();
var farm = myAdmin.getFarm(5);
var farm = myAdmin.getFarm('', 5);
var farm = myAdmin.getFarm('');
var farm = myAdmin.getFarm('devFarm');

//@ AdminSession: getFarm
print(farm);

//@# AdminSession: dropFarm errors
// Need a node to reproduce the not empty error
farm.addSeedInstance('192.168.1.1:33060');
var farm = myAdmin.dropFarm();
var farm = myAdmin.dropFarm(5);
var farm = myAdmin.dropFarm('');
var farm = myAdmin.dropFarm('sample', 5);
var farm = myAdmin.dropFarm('sample', {}, 5);
var farm = myAdmin.dropFarm('sample');
var farm = myAdmin.dropFarm('devFarm');

//@ AdminSession: dropFarm
myAdmin.dropFarm('devFarm', {dropDefaultReplicaSet: true});

// Cleanup
myAdmin.close();
