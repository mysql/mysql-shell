//@ {hasAuthEnvironment('FIDO') && __version_num >= 80027}
// This test is focused on testing ONLY the integration points of the MySQL Shell with the
// FIDO Authentication, not the FIDO authentication itself.
// There are 2 integration points:
//
// - Handling of the --fido-register-factor option: which would trigger an attempt to read the FIDO device
//   to do the registration.
// - Usage of a print callback: which will avoid the FIDO plugin from printing to stderr directly.
//
var port=3331;
testutil.deployRawSandbox(port, "root", {"plugin-load-add":"authentication_fido.so"});

// Create a user with the authentication plugin
shell.connect(`root:root@localhost:${port}`);
session.runSql("create user fidotest identified by 'mypwd' and identified with authentication_fido");


// Attempt to use the user without the device being registered, this test ensures:
// The authentication plugin is used as expected, and the corresponding error message is generated.
testutil.callMysqlsh([`fidotest:mypwd@localhost:${port}`, "--json=raw", `--mysql-plugin-dir=${MYSQL_PLUGIN_DIR}`, "--sql", "-e", 'select user()'])
EXPECT_OUTPUT_CONTAINS('{"error":{"code":4055,"line":1,"message":"Authentication plugin requires registration. Please refer ALTER USER syntax or set --fido-register-factor command line option to do registration.","state":"HY000","type":"MySQL Error"}}')


// Attempt registering the FIDO device (assuming there's none)
// This test confirms the print callback is properly set on the FIDO authentication client plugin as
// a JSON document is generated for the error coming from the plugin, instead of raw text in stderr
testutil.callMysqlsh([`fidotest:mypwd@localhost:${port}`, "--json=raw", `--mysql-plugin-dir=${MYSQL_PLUGIN_DIR}`, "--fido-register-factor=2", "--sql", "-e", 'select user()'])
EXPECT_OUTPUT_CONTAINS('{"info":"Failed to open FIDO device.\\n"}')

// Drops the sandbox
testutil.destroySandbox(port);
