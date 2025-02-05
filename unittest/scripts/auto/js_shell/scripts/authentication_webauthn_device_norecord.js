//@ {hasAuthEnvironment('FIDO') && __version_num >= 80200}

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

//@<> WL16770#FR1 wrong type value {fido_available}
EXPECT_THROWS(function(){
    shell.connect(`webauthn_test:mypwd@localhost:${__mysql_sandbox_port1}?plugin-authentication-webauthn-device=testvalue`);
}, "Argument #1: Invalid URI: The value of 'plugin-authentication-webauthn-device' must be an integer larger than 0.");
WIPE_OUTPUT();

//@<> WL16770#FR1 negative value {fido_available}
EXPECT_THROWS(function(){
    shell.connect(`webauthn_test:mypwd@localhost:${__mysql_sandbox_port1}?plugin-authentication-webauthn-device=-2`);
}, "Argument #1: Invalid URI: The value of 'plugin-authentication-webauthn-device' must be an integer larger than 0.");
WIPE_OUTPUT();

//@<> Drops the sandbox
testutil.destroySandbox(__mysql_sandbox_port1);
