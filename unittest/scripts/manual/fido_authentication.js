// Tests handling for FIDO authentication.
// These tests require usage of the FIDO device
//
// Execute as follows:
//          mysqlshrec -f <thisfile>, from the build dir.
//
// Making sure the path to the mysql server to use is on PATH

// Deploys a server with the authentication plugin enabled
var port=3331;
testutil.deployRawSandbox(port, "root", {"plugin-load-add":"authentication_fido.so"});

// Create a user with the authentication plugin
shell.connect(`root:root@localhost:${port}`);
session.runSql("create user fidotest identified by 'mypwd' and identified with authentication_fido");

// Attempt to execute a query
// EXPECTED FAILURE: Authentication plugin requires registration. Please refer ALTER USER syntax or set --fido-register-factor command line option to do registration.
testutil.callMysqlsh([`fidotest:mypwd@localhost:${port}`, "--sql", "-e", 'select user()'])

// Registers the FIDO device and executes the query.
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// PRODUCES:
// user()
// fidotest@localhost
testutil.callMysqlsh([`fidotest:mypwd@localhost:${port}`, "--fido-register-factor=2", "--sql", "-e", 'select user()'])

// Repeats the above operation, should fail as the FIDO device is already registered
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// EXPECTED FAILURE: The registration for 2 factor authentication method is already completed. You cannot perform registration multiple times.
testutil.callMysqlsh([`fidotest:mypwd@localhost:${port}`, "--fido-register-factor=2", "--sql", "-e", 'select user()'])

// Attempt to execute a query using normal authentication, succeeds this time
// EXPECTS: Please insert FIDO device and perform gesture action for registration to complete.
// PRODUCES:
// user()
// fidotest@localhost
testutil.callMysqlsh([`fidotest:mypwd@localhost:${port}`, "--sql", "-e", 'select user()'])

// Drops the sandbox
testutil.destroySandbox(port);