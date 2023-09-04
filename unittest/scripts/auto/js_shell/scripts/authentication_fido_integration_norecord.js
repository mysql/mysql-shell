//@ {hasAuthEnvironment('FIDO') && __version_num >= 80027}

// This test is focused on testing ONLY the integration points of the MySQL Shell with the
// FIDO Authentication, not the FIDO authentication itself.
// There are 2 integration points:
//
// - Handling of the --fido-register-factor option: which would trigger an attempt to read the FIDO device
//   to do the registration.
// - Usage of a print callback: which will avoid the FIDO plugin from printing to stderr directly.
//

//@<> Setup
var fido_available = isAuthMethodSupported('FIDO');
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", getAuthServerConfig('FIDO'));

// Create a user with the authentication plugin
try {
  shell.connect(__sandbox_uri1);

  if (fido_available) {
    session.runSql("create user fido_test identified by 'mypwd' and identified with authentication_fido");
  } else {
    session.runSql("create user fido_test identified by 'mypwd'");
  }

  session.close();
} catch (error) {
  println(testutil.catFile(testutil.getSandboxLogPath(__mysql_sandbox_port1)));
  throw error;
}

//@<> Test registration {fido_available}
// Attempt to use the user without the device being registered, this test ensures:
// The authentication plugin is used as expected, and the corresponding error message is generated.
testutil.callMysqlsh([`fido_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--json=raw", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('{"error":{"code":4055,"line":1,"message":"Authentication plugin requires registration. Please refer ALTER USER syntax or set --fido-register-factor command line option to do registration.","state":"HY000","type":"MySQL Error"}}');

//@<> Test authentication {fido_available && __os_type != 'windows'}
// This currently crashes on Windows, see: BUG#34918044
// Attempt registering the FIDO device (assuming there's none)
// This test confirms the print callback is properly set on the FIDO authentication client plugin as
// a JSON document is generated for the error coming from the plugin, instead of raw text in stderr
testutil.callMysqlsh([`fido_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--json=raw", "--fido-register-factor=2", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('{"info":"No FIDO device available on client host.\\n"}');

//@<> Test loading the plugin {!fido_available}
WIPE_SHELL_LOG();
testutil.callMysqlsh([`fido_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--log-level=8", "--json=raw", "--auth-method=authentication_fido_client", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('{"user()":"fido_test@localhost"}');

//@<> Drops the sandbox
testutil.destroySandbox(__mysql_sandbox_port1);
