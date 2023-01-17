//@ {hasAuthEnvironment('LDAP_KERBEROS') && __version_num >= 80020}

//@<> Setup
if (debugAuthPlugins()) {
  testutil.setenv("AUTHENTICATION_LDAP_CLIENT_LOG", "5");
}

var ldap_kerberos_available = isAuthMethodSupported('LDAP_KERBEROS');
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', getAuthServerConfig('LDAP_KERBEROS'));

try {
    shell.connect(__sandbox_uri1);

    if (ldap_kerberos_available) {
        session.runSql("CREATE DATABASE test_user_db");
        session.runSql(`CREATE USER '${LDAP_KERBEROS_USER}@MYSQL.LOCAL' IDENTIFIED WITH authentication_ldap_sasl BY "${LDAP_KERBEROS_AUTH_STRING}"`);
        session.runSql("CREATE USER 'invalid_user' IDENTIFIED WITH authentication_ldap_sasl");
        session.runSql("CREATE USER 'mysql_engineering'");
        session.runSql("GRANT ALL PRIVILEGES ON test_user_db.* TO 'mysql_engineering'");
        session.runSql("GRANT ALL PRIVILEGES ON test_user_db.* TO 'invalid_user'");
        session.runSql(`GRANT PROXY on 'mysql_engineering' TO '${LDAP_KERBEROS_USER}@MYSQL.LOCAL'`);
    } else {
        session.runSql("create user ldap_kerberos_test identified by 'mypwd'");
    }

    session.close();
} catch (error) {
    println(testutil.catFile(testutil.getSandboxLogPath(__mysql_sandbox_port1)));
    throw error;
}

var test_list = {
    "SELECT current_user()": "mysql_engineering@%",
    "SELECT user()": `${LDAP_KERBEROS_USER}@MYSQL.LOCAL@localhost`,
    "SELECT @@local.proxy_user": `'${LDAP_KERBEROS_USER}@MYSQL.LOCAL'@'%'`,
    "SELECT @@local.external_user": "mysql_engineering"
};

// Cleans the Kerberos cache
testutil.callMysqlsh(["--py", "-i", "-e", `import os;os.system('${__os_type == 'windows' ? 'klist purge' : 'kdestroy'}')`]);

//@<> WL14553-TSFR_8_2 - Kerberos session with no user/password, should fail as there's no cached TGT {ldap_kerberos_available}
args = ["--mysql", "--host=localhost",
    `--port=${__mysql_sandbox_port1}`,
    '--schema=test_user_db',
    '--auth-method=authentication_ldap_sasl_client',
    "--credential-store-helper=plaintext"]

// WL14553-TSFR_9_4 - No user/password provided
testutil.callMysqlsh(args.concat(["-i", "-e", "SELECT 1"]));

// System user is not automatically added to the connection data
EXPECT_OUTPUT_CONTAINS(`Creating a Classic session to 'localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`)

// The client library will pick the system user anyway
EXPECT_OUTPUT_CONTAINS(`SSO user not found, Please perform SSO authentication using kerberos.`)
WIPE_OUTPUT()

//@<> WL14553-TSFR_9_4 - User given but no password {ldap_kerberos_available}
testutil.callMysqlsh(args.concat(["-i", "-e", "SELECT 1", `--user=${LDAP_KERBEROS_USER}@MYSQL.LOCAL`]));

// System user is not automatically added to the connection data
EXPECT_OUTPUT_CONTAINS(`Creating a Classic session to '${LDAP_KERBEROS_USER}%40MYSQL.LOCAL@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`)

// The client library will pick the system user anyway
// NOTE: This is the error reported by the server on this scenario rather than a clear message
//       about wrong/missing password for the kerberos account
EXPECT_OUTPUT_CONTAINS('Unknown MySQL error')
WIPE_OUTPUT()

args.push("--quiet-start=2")
args.push("--sql")
args.push("-e")

cli_variants = []

// WL14553-TSFR_9_1 - Full credentials will cause the TGT to be created
cli_variants.push([`--user=${LDAP_KERBEROS_USER}@MYSQL.LOCAL`, `--password=${LDAP_KERBEROS_PWD}`])

// The rest of the variants fall back to the cached TGT
// WL14553-TSFR_9_2 - No User/Password
cli_variants.push([])

// User but no password
cli_variants.push([`--user=${LDAP_KERBEROS_USER}@MYSQL.LOCAL`])

// WL14553-TSFR_8_1 - Password but no user
cli_variants.push([`--password=whatever`])

// User and wrong password
cli_variants.push([`--user=${LDAP_KERBEROS_USER}@MYSQL.LOCAL`, `--password=whatever`])

//@<> Test TGT with independent shell instances {ldap_kerberos_available}
for (variant_index in cli_variants) {
    for (query in test_list) {
        testutil.callMysqlsh(args.concat([`${query}`]).concat(cli_variants[variant_index]));
        EXPECT_OUTPUT_CONTAINS(test_list[query] ? test_list[query] : "NULL");
        WIPE_OUTPUT();
    }
}

//@<> Test TGT with interactive shell connections {ldap_kerberos_available}
ok_variants = []
// Full credentials will cause the TGT to be created
ok_variants.push(function () {
    shell.connect(`mysql://${LDAP_KERBEROS_USER}%40MYSQL.LOCAL:${LDAP_KERBEROS_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`);
});

// Repeats using Shell API
ok_variants.push(function () {
    session = mysql.getSession(`mysql://${LDAP_KERBEROS_USER}%40MYSQL.LOCAL:${LDAP_KERBEROS_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`);
});

// No user/password, falls back to cached TGT
ok_variants.push(function () {
    shell.connect(`mysql://localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`);
});

// Repeats using the Shell API
ok_variants.push(function () {
    session = mysql.getSession(`mysql://localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`);
});


// User and wrong password, ignores password and uses cached TGT
ok_variants.push(function () {
    shell.connect(`mysql://${LDAP_KERBEROS_USER}%40MYSQL.LOCAL:fakepwd@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`);
});

// Repeats using the Shell API
ok_variants.push(function () {
    session = mysql.getSession(`mysql://${LDAP_KERBEROS_USER}%40MYSQL.LOCAL:fakepwd@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`);
});

for (variant_index in ok_variants) {
    // Creates the session
    ok_variants[variant_index]();

    // Test queries
    for (query in test_list) {
        var result = session.runSql(query);
        var row = result.fetchOne();
        EXPECT_EQ(test_list[query], row[0]);
    }

    // Closes the session
    session.close()
}

//@<> Test loading the plugin {!ldap_kerberos_available}
WIPE_SHELL_LOG();
testutil.callMysqlsh([`ldap_kerberos_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--log-level=8", "--json=raw", "--auth-method=authentication_ldap_sasl_client", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('{"user()":"ldap_kerberos_test@localhost"}');
EXPECT_SHELL_LOG_CONTAINS('authentication_ldap_sasl_client.AUTHENTICATE.AUTH_PLUGIN');

//@<> Cleanup
if (debugAuthPlugins()) {
    println(testutil.catFile(testutil.getSandboxLogPath(__mysql_sandbox_port1)));
}

testutil.destroySandbox(__mysql_sandbox_port1);
