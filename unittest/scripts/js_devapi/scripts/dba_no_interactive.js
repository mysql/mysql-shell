// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
dba.dropMetadataSchema({enforce:true});

//@ Session: validating members
var members = dir(dba);

print("Session Members:", members.length);
validateMember(members, 'defaultFarm');
validateMember(members, 'getDefaultFarm');
validateMember(members, 'createFarm');
validateMember(members, 'dropFarm');
validateMember(members, 'getFarm');
validateMember(members, 'dropMetadataSchema');
validateMember(members, 'resetSession');

//@# AdminSession: createFarm errors
var farm = dba.createFarm();
var farm = dba.createFarm(5);
var farm = dba.createFarm('', 5);
var farm = dba.createFarm('devFarm');
var farm = dba.createFarm('devFarm', 'password');
var farm = dba.createFarm('devFarm', 'password');

//@ AdminSession: createFarm
print (farm)

//@# AdminSession: getFarm errors
var farm = dba.getFarm();
var farm = dba.getFarm(5);
var farm = dba.getFarm('', 5);
var farm = dba.getFarm('');
var farm = dba.getFarm('devFarm');

//@ AdminSession: getFarm
print(farm);

//@# AdminSession: dropFarm errors
// Need a node to reproduce the not empty error
farm.addSeedInstance(__host_port);
var farm = dba.dropFarm();
var farm = dba.dropFarm(5);
var farm = dba.dropFarm('');
var farm = dba.dropFarm('sample', 5);
var farm = dba.dropFarm('sample', {}, 5);
var farm = dba.dropFarm('sample');
var farm = dba.dropFarm('devFarm');

//@ AdminSession: dropFarm
dba.dropFarm('devFarm', {dropDefaultReplicaSet: true});
