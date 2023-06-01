//@ {hasAuthEnvironment('KERBEROS') && __version_num >= 80026 && __os_type != 'macos'}

//@<> Setup
if (debugAuthPlugins()) {
  dba.verbose = 1;
  testutil.setenv("AUTHENTICATION_KERBEROS_CLIENT_LOG", "5");
}

var kerberos_available = isAuthMethodSupported('KERBEROS');
var server_conf = getAuthServerConfig('KERBEROS');
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', server_conf, { "timeout": 120 });

try {
    shell.connect(__sandbox_uri1);

    if (kerberos_available) {
        session.runSql("CREATE DATABASE test_user_db");
        session.runSql(`CREATE USER '${KERBEROS_USER}' IDENTIFIED WITH authentication_kerberos BY '${KERBEROS_DOMAIN}';`);
        session.runSql(`CREATE USER 'invalid_user' IDENTIFIED WITH authentication_kerberos BY '${KERBEROS_DOMAIN}';`);
        session.runSql("GRANT ALL PRIVILEGES ON test_user_db.* TO 'invalid_user'");
        session.runSql(`GRANT ALL PRIVILEGES ON *.* TO '${KERBEROS_USER}'`);
    } else {
        session.runSql("create user kerberos_test identified by 'mypwd'");
    }

    session.close();
} catch (error) {
    println(testutil.catFile(testutil.getSandboxLogPath(__mysql_sandbox_port1)));
    throw error;
}

var test_list = {
    "SELECT current_user()": `${KERBEROS_USER}@%`,
    "SELECT user()": `${KERBEROS_USER}@localhost`,
    "SELECT @@local.proxy_user": null,
    "SELECT @@local.external_user": null
};

// Cleans the Kerberos cache
testutil.callMysqlsh(["--py", "-i", "-e", `import os;os.system('${__os_type == 'windows' ? 'klist purge' : 'kdestroy'}')`]);

//@<> WL14553-TSFR_8_2 - Kerberos session with no user/password, should fail as there's no cached TGT {kerberos_available}
args = ["--mysql", "--host=localhost",
    `--port=${__mysql_sandbox_port1}`,
    '--schema=test_user_db',
    '--auth-method=authentication_kerberos_client',
    "--credential-store-helper=plaintext"]

// WL14553-TSFR_9_4 - No user/password provided
testutil.callMysqlsh(args.concat(["-i", "--sql", "-e", "SELECT current_user()"]));

// System user is not automatically added to the connection data
EXPECT_OUTPUT_CONTAINS(`Creating a Classic session to 'localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_kerberos_client`)

// The client library will pick the system user anyway
EXPECT_OUTPUT_CONTAINS(`Access denied for user '${__system_user}'@'localhost'`)
WIPE_OUTPUT()

//@<> WL14553-TSFR_9_4 - User given but no password {kerberos_available}
testutil.callMysqlsh(args.concat(["-i", "--sql", "-e", "SELECT current_user()", `--user=${KERBEROS_USER}`]));

// System user is not automatically added to the connection data
EXPECT_OUTPUT_CONTAINS(`Creating a Classic session to '${KERBEROS_USER}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_kerberos_client`)

// The client library will pick the system user anyway
// NOTE: This is the error reported by the server on this scenario rather than a clear message
//       about wrong/missing password for the kerberos account
EXPECT_OUTPUT_CONTAINS(`Unknown MySQL error`)
WIPE_OUTPUT()

args.push("--quiet-start=2")
args.push("--sql")
args.push("-e")

cli_variants = []

// WL14553-TSFR_9_1 - Full credentials will cause the TGT to be created
cli_variants.push([`--user=${KERBEROS_USER}`, `--password=${KERBEROS_PWD}`])

// The rest of the variants fall back to the cached TGT
// WL14553-TSFR_9_2 - No User/Password
cli_variants.push([])

// User but no password
cli_variants.push([`--user=${KERBEROS_USER}`])

// WL14553-TSFR_8_1 - Password but no user
cli_variants.push([`--password=whatever`])

// User and wrong password
cli_variants.push([`--user=${KERBEROS_USER}`, `--password=whatever`])

//@<> Test TGT with independent shell instances {kerberos_available}
for (variant_index in cli_variants) {
    for (query in test_list) {
        testutil.callMysqlsh(args.concat([`${query}`]).concat(cli_variants[variant_index]));
        EXPECT_OUTPUT_CONTAINS(test_list[query] ? test_list[query] : "NULL");
        WIPE_OUTPUT();
    }
}


//@<> Test TGT with interactive shell connections {kerberos_available}
ok_variants = []
// Full credentials will cause the TGT to be created
ok_variants.push(function () {
    shell.connect(`mysql://${KERBEROS_USER}:${KERBEROS_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_kerberos_client`);
});

// Repeats using the Shell API
ok_variants.push(function () {
    session = mysql.getSession(`mysql://${KERBEROS_USER}:${KERBEROS_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_kerberos_client`);
});

// No user/password, falls back to cached TGT
ok_variants.push(function () {
    shell.connect(`mysql://localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_kerberos_client`);
});

// Repeats using the Shell API
ok_variants.push(function () {
    session = mysql.getSession(`mysql://localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_kerberos_client`);
});

// User and wrong password, ignores password and uses cached TGT
ok_variants.push(function () {
    shell.connect(`mysql://${KERBEROS_USER}:fakepwd@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_kerberos_client`);
});

// Repeats using the Shell API
ok_variants.push(function () {
    session = mysql.getSession(`mysql://${KERBEROS_USER}:fakepwd@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_kerberos_client`);
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

//@<> Test loading the plugin {!kerberos_available}
WIPE_SHELL_LOG();
testutil.callMysqlsh([`kerberos_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--log-level=8", "--json=raw", "--auth-method=authentication_kerberos_client", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('{"user()":"kerberos_test@localhost"}');
EXPECT_SHELL_LOG_CONTAINS('authentication_kerberos_client.AUTHENTICATE.AUTH_PLUGIN');

//@<> Cleanup
if (debugAuthPlugins()) {
    println(testutil.catFile(testutil.getSandboxLogPath(__mysql_sandbox_port1)));
}

if (server_conf.authentication_kerberos_service_key_tab) {
    testutil.rmfile(server_conf.authentication_kerberos_service_key_tab);
}

testutil.destroySandbox(__mysql_sandbox_port1);
