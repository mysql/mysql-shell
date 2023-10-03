// Tests handling for WebauthN authentication.
// These tests require usage of the FIDO device
//
// Execute as follows:
//          mysqlshrec -f <thisfile>, from the build dir.
//
// Making sure the path to the mysql server to use is on PATH

// Deploys a server with the authentication plugin enabled
var port=3331;
testutil.deployRawSandbox(port, "root", {"plugin-load-add":"authentication_webauthn.so", "authentication-webauthn-rp-id":"mysql.com"});

// Create a user with the authentication plugin
shell.connect(`root:root@localhost:${port}`);
session.runSql("create user webauthntest identified by 'mypwd' and identified with authentication_webauthn");

// Attempt to execute a query
// EXPECTED FAILURE: Authentication plugin requires registration. Please refer ALTER USER syntax or set --fido-register-factor command line option to do registration.
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--sql", "-e", 'select user()'])

// Registers the FIDO device and executes the query.
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED OUTPUT:
// user()
// webauthntest@localhost
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--register-factor=2", "--sql", "-e", 'select user()'])

// Repeats the above operation, should fail as the FIDO device is already registered
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED FAILURE: The registration for 2 factor authentication method is already completed. You cannot perform registration multiple times.
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--register-factor=2", "--sql", "-e", 'select user()'])

// Attempt to execute a query using normal authentication, succeeds this time
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED OUTPUT:
// user()
// webauthntest@localhost
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--sql", "-e", 'select user()'])

// Recreate the account to verify simultaneous usage of --fido-register-factor and --register-factor
// EXPECT: WARNING: The --fido-register-factor option was deprecated, please use --register-factor instead. (Option has been processed as --register-factor=3).
// EXPECT: Please insert FIDO device and follow the instruction.Depending on the device, you may have to perform gesture action multiple times.
//         1. Perform gesture action (Skip this step if you are prompted to enter device PIN).
// EXPECTED OUTPUT:
// user()
// webauthntest@localhost
shell.connect(`root:root@localhost:${port}`);
session.runSql("drop user webauthntest@'%'")
session.runSql("create user webauthntest identified by 'mypwd' and identified with authentication_webauthn");
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--fido-register-factor=3", "--register-factor=2", "--sql", "-e", 'select user()'])

// Recreate the account to verify simultaneous usage of -register-factor and --fido-register-factor
// The deprecation warning will be shown, but the last value provided is used
shell.connect(`root:root@localhost:${port}`);
session.runSql("drop user webauthntest@'%'")
session.runSql("create user webauthntest identified by 'mypwd' and identified with authentication_webauthn");
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--register-factor=3", "--fido-register-factor=2", "--sql", "-e", 'select user()'])


// Verify the --plugin-authentication-webauthn-client-preserve-privacy is properly set to 1
// EXPECT: The following should be added to the shell log: Debug3: Using authentication_webauthn_client_preserve_privacy =  1
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--log-level=debug3", "--plugin-authentication-webauthn-client-preserve-privacy", "--sql", "-e", 'select user()'])
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--log-level=debug3", "--plugin-authentication-webauthn-client-preserve-privacy=1", "--sql", "-e", 'select user()'])
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--log-level=debug3", "--plugin-authentication-webauthn-client-preserve-privacy=true", "--sql", "-e", 'select user()'])

// Verify the --plugin-authentication-webauthn-client-preserve-privacy is properly set to 0
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--log-level=debug3", "--plugin-authentication-webauthn-client-preserve-privacy=0", "--sql", "-e", 'select user()'])
testutil.callMysqlsh([`webauthntest:mypwd@localhost:${port}`, "--log-level=debug3", "--plugin-authentication-webauthn-client-preserve-privacy=false", "--sql", "-e", 'select user()'])

// Drops the sandbox
testutil.destroySandbox(port);