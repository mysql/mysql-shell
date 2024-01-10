//@ {hasAuthEnvironment('LDAP_SASL')}

//@<> Setup
if (debugAuthPlugins()) {
  testutil.setenv("AUTHENTICATION_LDAP_CLIENT_LOG", "5");
}

testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', getAuthServerConfig('LDAP_SASL'));
var ldap_sasl_available = isAuthMethodSupported('LDAP_SASL', __sandbox_uri1);

try {
    shell.connect(__sandbox_uri1);

    if (ldap_sasl_available) {
        session.runSql("CREATE DATABASE test_user_db");
        session.runSql(`CREATE USER '${LDAP_SASL_USER}' IDENTIFIED WITH authentication_ldap_sasl`);
        session.runSql("CREATE USER 'common'");
        session.runSql("GRANT ALL PRIVILEGES ON test_user_db.* TO 'common'");
        session.runSql(`GRANT PROXY on 'common' TO '${LDAP_SASL_USER}'`);
        // User without group
        session.runSql("CREATE USER 'test1' IDENTIFIED WITH authentication_ldap_sasl");
    } else {
        session.runSql("create user ldap_sasl_test identified by 'mypwd'");
    }

    session.close();
} catch (error) {
    println(testutil.catFile(testutil.getSandboxLogPath(__mysql_sandbox_port1)));
    throw error;
}

var test_list = {
    "SELECT current_user()": "common@%",
    "SELECT user()": `${LDAP_SASL_USER}@localhost`,
    "SELECT @@local.proxy_user": `'${LDAP_SASL_USER}'@'%'`,
    "SELECT @@local.external_user": "common"
};

function escape_string(s) {
  // path on Windows needs escaping
  return s.replaceAll("\\", "\\\\");
}

//@<> WL14553-TSFR_4_3 - Interactive LDAP Simple TCP session with PLUGIN_DIR with no plugins {ldap_sasl_available}
testutil.callMysqlsh(["--", "shell", "options", "set-persist", "mysqlPluginDir", __test_data_path]);
var args = [`-u${LDAP_SASL_USER}`, `--password=${LDAP_SASL_PWD}`,
            "-hlocalhost", "--port=" + __mysql_sandbox_port1,
            "--auth-method=authentication_ldap_sasl_client",
            "--quiet-start=2", "--sql", "-e", "show databases"]
testutil.callMysqlsh(args, "");
EXPECT_OUTPUT_CONTAINS("Authentication plugin 'authentication_ldap_sasl_client' cannot be loaded");
WIPE_OUTPUT();
testutil.callMysqlsh(["--", "shell", "options", "unset-persist", "mysqlPluginDir"]);

//@<> Interactive LDAP Simple TCP session {ldap_sasl_available}

var interactive_variants = [];
interactive_variants.push(function () {
    shell.connect(`${LDAP_SASL_USER}:${LDAP_SASL_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`);
})

// Using the Shell API
interactive_variants.push(function () {
    session = mysql.getSession(`${LDAP_SASL_USER}:${LDAP_SASL_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`);
})

for (variant_index in interactive_variants) {
    // Connects
    interactive_variants[variant_index]();
    for (query in test_list) {
        var result = session.runSql(query);
        var row = result.fetchOne();
        EXPECT_EQ(test_list[query], row[0]);
    }

    // Closes the session
    session.close();
}



//@<> Test CLI LDAP Simple Session - Invalid plugin path scenarios {ldap_sasl_available}
// WL14553-TSFR_4_3
// WL14553-TSFR_4_4
// WL14553-TSFR_4_7
// Tests failure combinations for not having the correct plugins path
// the LIBMYSQL_PLUGIN_DIR env var or the --mysql-plugin-dir  cmdline arg
var fail_variants = []
// WL14553-TSFR_4_11 - NO env var, and shell option is WRONG
fail_variants.push([[], `--mysql-plugin-dir=${__test_data_path}`])
// OK in env var, but shell is wrong
fail_variants.push([[`LIBMYSQL_PLUGIN_DIR=${shell.options.mysqlPluginDir}`], `--mysql-plugin-dir=${__test_data_path}`])

for (variant_index in fail_variants) {
    for (query in test_list) {
        var args = [`-u${LDAP_SASL_USER}`, `--password=${LDAP_SASL_PWD}`,
            "-hlocalhost", "--port=" + __mysql_sandbox_port1,
            "--auth-method=authentication_ldap_sasl_client",
            "--quiet-start=2", "--sql", "-e", `${query}`]
        if (fail_variants[variant_index][1].length > 0) {
            args.push(fail_variants[variant_index][1]);
        }

        testutil.callMysqlsh(args, "", fail_variants[variant_index][0]);
        EXPECT_OUTPUT_CONTAINS("Authentication plugin 'authentication_ldap_sasl_client' cannot be loaded");
        WIPE_OUTPUT();
    }
}


//@<> Test CLI LDAP Simple Session - Valid plugin path scenarios {ldap_sasl_available}
// WL14553-TSFR_4_8
// Tests success combinations defining the correct plugins path in either
// Array Items Are:
// 0: Value to be used in the LIBMYSQL_PLUGIN_DIR env variable
// 1: Value to be used in the --mysql-plugin-dir cmdline arg
// 2: Value to be set in the mysqlPluginDir shell option

var ok_variants = []
// WL14553-TSFR_4_12 - No plugin path defined at all
ok_variants.push([[], "", ""])
// WRONG in env var, no cmdline arg, default shell option
fail_variants.push([[`LIBMYSQL_PLUGIN_DIR=${__test_data_path}`], "", ""])
// OK in env var, no cmdline arg, default shell option
ok_variants.push([[`LIBMYSQL_PLUGIN_DIR=${shell.options.mysqlPluginDir}`], "", ""])
// NO env var, OK in shell cmdline arg, default shell option
ok_variants.push([[], `--mysql-plugin-dir=${shell.options.mysqlPluginDir}`, ""])
// WRONG in env var, OK in shell option, default shell option
ok_variants.push([[`LIBMYSQL_PLUGIN_DIR=${__test_data_path}`], `--mysql-plugin-dir=${shell.options.mysqlPluginDir}`, ""])
// NO env var, NO cmdline arg but OK in shell option
ok_variants.push([[], "", shell.options.mysqlPluginDir])
// WL14553-TSFR_5_2 - NO env var, OK in cmdline arg, WRONG shell option
ok_variants.push([[], `--mysql-plugin-dir=${shell.options.mysqlPluginDir}`, __test_data_path])

for (variant_index in ok_variants) {
    // WL14553-TSFR_4_9
    if (ok_variants[variant_index][2].length != 0) {
        testutil.callMysqlsh(['--', "shell", "options", "set-persist", "mysqlPluginDir", ok_variants[variant_index][2]]);
        testutil.callMysqlsh(['-i', '-e', "shell.options"]);
        EXPECT_OUTPUT_CONTAINS(`"mysqlPluginDir": "${escape_string(ok_variants[variant_index][2])}"`);
        WIPE_OUTPUT();
    }
    for (query in test_list) {
        var args = [`-u${LDAP_SASL_USER}`, `--password=${LDAP_SASL_PWD}`,
            "-hlocalhost", "--port=" + __mysql_sandbox_port1,
            "--auth-method=authentication_ldap_sasl_client",
            "--quiet-start=2", "--sql", "-e", `${query}`]
        if (ok_variants[variant_index][1].length > 0) {
            args.push(ok_variants[variant_index][1]);
        }

        testutil.callMysqlsh(args, "", ok_variants[variant_index][0]);
        EXPECT_OUTPUT_CONTAINS(test_list[query]);
        WIPE_OUTPUT();
    }
    // WL14553-TSFR_5_2 - Ensures despite the value used for plugin dirs, the persisted value is still the same
    if (ok_variants[variant_index][2].length != 0) {
        testutil.callMysqlsh(['-i', '-e', "shell.options"]);
        EXPECT_OUTPUT_CONTAINS(`"mysqlPluginDir": "${escape_string(ok_variants[variant_index][2])}"`);
        WIPE_OUTPUT();
    }
}

//@<> WL14553-TSFR_4_9 - Testing the Unset Persist {ldap_sasl_available}
testutil.callMysqlsh(['--', "shell", "options", "unset-persist", "mysqlPluginDir"]);
testutil.callMysqlsh(['-i', '-e', "shell.options"]);
EXPECT_OUTPUT_CONTAINS(`"mysqlPluginDir": "${escape_string(shell.options.mysqlPluginDir)}"`);

//@<> Test loading the plugin {!ldap_sasl_available}
WIPE_SHELL_LOG();
testutil.callMysqlsh([`ldap_sasl_test:mypwd@localhost:${__mysql_sandbox_port1}`, "--log-level=8", "--json=raw", "--auth-method=authentication_ldap_sasl_client", "--sql", "-e", 'select user()']);
EXPECT_OUTPUT_CONTAINS('{"user()":"ldap_sasl_test@localhost"}');
EXPECT_SHELL_LOG_CONTAINS('authentication_ldap_sasl_client.AUTHENTICATE.AUTH_PLUGIN');

//@<> Cleanup
if (debugAuthPlugins()) {
    println(testutil.catFile(testutil.getSandboxLogPath(__mysql_sandbox_port1)));
}

testutil.destroySandbox(__mysql_sandbox_port1);
