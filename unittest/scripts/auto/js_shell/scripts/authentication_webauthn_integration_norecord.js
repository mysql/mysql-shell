//@ {hasAuthEnvironment('FIDO') && __version_num >= 80200}

// This test is focused on testing ONLY the integration points of the MySQL Shell with the
// FIDO Authentication, not the FIDO authentication itself.
// There are 2 integration points:
//
// - Handling of the --register-factor option: which would trigger an attempt to read the FIDO device
//   to do the registration.
// - Usage of a print callback: which will avoid the WEBAUTHN plugin from printing to stderr directly.
//

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", getAuthServerConfig('WEBAUTHN'));
var fido_available = isAuthMethodSupported('WEBAUTHN', __sandbox_uri1);

// Create a user with the authentication plugin
try {
  shell.connect(__sandbox_uri1);

  if (fido_available) {
    session.runSql("create user webauthn_test identified by 'mypwd' and identified with authentication_webauthn");
  } else {
    session.runSql("create user webauthn_test identified by 'mypwd'");
  }

  session.close();
} catch (error) {
  println(testutil.catFile(testutil.getSandboxLogPath(__mysql_sandbox_port1)));
  throw error;
}

//@<> Test registration {fido_available}
// Attempt to use the user without the device being registered, this test ensures:
// The authentication plugin is used as expected, and the corresponding error message is generated.
testutil.callMysqlsh([`webauthn_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--json=raw", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('{"error":{"code":4055,"line":1,"message":"Authentication plugin requires registration. Please refer ALTER USER syntax or set --register-factor command line option to do registration.","state":"HY000","type":"MySQL Error"}}');

//@<> Test registration using X protocol {fido_available && __os_type != 'windows'}
testutil.callMysqlsh([`webauthn_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--mysqlx", "--json=raw", "--register-factor=2", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('Option --register-factor is only available for MySQL protocol connections.');

//@<> Test the preserve privacy option using X protocol {fido_available && __os_type != 'windows'}
// NOTE: the appearance of the error indicates the value was accepted (see next test for invalid value)
allowed_values = [
  "--plugin-authentication-webauthn-client-preserve-privacy",
  "--plugin-authentication-webauthn-client-preserve-privacy=1",
  "--plugin-authentication-webauthn-client-preserve-privacy=0",
  "--plugin-authentication-webauthn-client-preserve-privacy=true",
  "--plugin-authentication-webauthn-client-preserve-privacy=false",
]

for (option of allowed_values) {
  testutil.callMysqlsh([`webauthn_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--mysqlx", "--json=raw", option, "--sql", "-e", 'select user()']);
  EXPECT_OUTPUT_CONTAINS('Option --plugin-authentication-webauthn-client-preserve-privacy is only available for MySQL protocol connections.');
  WIPE_OUTPUT()
}

//@<> Using an invalid value {fido_available && __os_type != 'windows'}
testutil.callMysqlsh([`webauthn_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--mysqlx", "--json=raw", "--plugin-authentication-webauthn-client-preserve-privacy=whatever", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('Incorrect option value.');
WIPE_OUTPUT()

//@<> Test registration using non webauthn account {fido_available}
// Attempt to register a decide using an account not using webauthn
// The authentication plugin is used as expected, and the corresponding error message is generated.
testutil.callMysqlsh([__sandbox_uri1, "--json=raw", "--register-factor=2", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS("MySQL Error 4057 (HY000): 2 factor authentication method doesn't exist. Please do ALTER USER... ADD 2 factor... before doing this operation.");

//@<> Test authentication {fido_available && __os_type != 'windows'}
// This currently crashes on Windows, see: BUG#34918044
// Attempt registering the FIDO device (assuming there's none)
// This test confirms the print callback is properly set on the FIDO authentication client plugin as
// a JSON document is generated for the error coming from the plugin, instead of raw text in stderr
testutil.callMysqlsh([`webauthn_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--json=raw", "--register-factor=2", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('{"info":"No FIDO device available on client host.\\n"}');

//@<> Drops the sandbox
testutil.destroySandbox(__mysql_sandbox_port1);
