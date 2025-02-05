// WL#16770 Tests selecting authentication device for webauthn plugin

// Run with mysqlshrec -f <thisfile>, from the build dir.

// This file needs to be run on a machine with at least two authentication devices to fully test the feature

var port= 3331;
var host = 'localhost'

println('Deploying sandbox with webauth plugin...');
testutil.deployRawSandbox(port, "root", {"plugin-load-add":"authentication_webauthn.so", "authentication-webauthn-rp-id":"mysql.com"});

// Create two users with the authentication plugin (one for each device)
println('Creating users...');
shell.connect(`root:root@${host}:${port}`);
session.runSql("create user webauthntest1 identified by 'mypwd' and identified with authentication_webauthn");
session.runSql("create user webauthntest2 identified by 'mypwd' and identified with authentication_webauthn");

// register each user with different device

// Device #0
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED OUTPUT:
// user()
// webauthntest1@localhost
println('Registering first user with first (#0) device.');
testutil.callMysqlsh([`webauthntest1:mypwd@${host}:${port}`, "--register-factor=2", "--plugin-authentication-webauthn-device=0", "--sql", "-e", 'select user()'])

// Device #1
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED OUTPUT:
// user()
// webauthntest2@localhost
println('Registering second user with second (#1) device.');
testutil.callMysqlsh([`webauthntest2:mypwd@${host}:${port}`, "--register-factor=2", "--plugin-authentication-webauthn-device=1", "--sql", "-e", 'select user()'])

// Attempt by second user to log in with default device - should fail.
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED FAILURE: Access denied for user 'webauthntest2'@'localhost`
println('Login attempt for second user with default (#0) device, should fail.');
testutil.callMysqlsh([`webauthntest2:mypwd@${host}:${port}`, "--sql", "-e", 'select user()'])

// Attempt by first user to log in with default device - should succeed.
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED OUTPUT:
// user()
// webauthntest2@localhost
println('Login attempt for second user with default (#0) device, should succeed.');
testutil.callMysqlsh([`webauthntest1:mypwd@${host}:${port}`, "--sql", "-e", 'select user()'])

// Attempt by first user to log in with selected second (#1) device - should fail.
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED FAILURE: Access denied for user 'webauthntest1'@'localhost`
println('Login attempt for first user with selected second (#1) device, should fail.');
testutil.callMysqlsh([`webauthntest1:mypwd@${host}:${port}`, "--plugin-authentication-webauthn-device=1", "--sql", "-e", 'select user()'])

// Attempt by second user to log in with selected second (#1) device - should succeed.
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED OUTPUT:
// user()
// webauthntest2@localhost
println('Login attempt for second user with selected second (#1) device, should succeed.');
testutil.callMysqlsh([`webauthntest2:mypwd@${host}:${port}`, "--plugin-authentication-webauthn-device=1", "--sql", "-e", 'select user()'])

// Attempt by first user to log in with selected tenth, not-existing (#9) device - should fail.
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED FAILURE: Requested FIDO device '9' not present. Please correct the device id supplied or make sure the device is present.
println('Login attempt for first user with selected tenth (#9) device (should not exist), should fail.');
testutil.callMysqlsh([`webauthntest1:mypwd@${host}:${port}`, "--plugin-authentication-webauthn-device=9", "--sql", "-e", 'select user()'])

// Drops the sandbox
println('End of tests, destroying sandbox.')
testutil.destroySandbox(port);