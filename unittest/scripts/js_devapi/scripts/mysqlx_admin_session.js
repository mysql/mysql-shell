// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
var mysqlx = require('mysqlx').mysqlx;

//@ Session: validating members
var myAdmin = mysqlx.getAdminSession(__uripwd);
var members = dir(myAdmin);

print("Session Members:", members.length);
validateMember(members, 'uri');
validateMember(members, 'defaultFarm');
validateMember(members, 'getUri');
validateMember(members, 'getDefaultFarm');
validateMember(members, 'createFarm');
validateMember(members, 'getFarm');
validateMember(members, 'close');

// Cleanup
myAdmin.close();