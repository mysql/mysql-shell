//@ {hasAuthEnvironment('LDAP_KERBEROS') && __version_num >= 80020}
//@<> Setup
if (__os_type == 'windows') {
    plugin = "authentication_ldap_sasl.dll"
} else {
    plugin = "authentication_ldap_sasl.so"
}
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root',
    {
        "plugin-load-add": plugin,
        "authentication_ldap_sasl_server_host": `${LDAP_KERBEROS_SERVER_HOST}`,
        "authentication_ldap_sasl_server_port": parseInt(`${LDAP_KERBEROS_SERVER_PORT}`),
        "authentication_ldap_sasl_bind_base_dn": `${LDAP_KERBEROS_BIND_BASE_DN}`,
        "authentication_ldap_sasl_user_search_attr": `${LDAP_KERBEROS_USER_SEARCH_ATTR}`,
        "authentication_ldap_sasl_bind_root_dn": `${LDAP_KERBEROS_BIND_ROOT_DN}`,
        "authentication_ldap_sasl_bind_root_pwd": `${LDAP_KERBEROS_BIND_ROOT_PWD}`,
        "authentication_ldap_sasl_group_search_filter": `${LDAP_KERBEROS_GROUP_SEARCH_FILTER}`,
        "authentication_ldap_sasl_auth_method_name": 'GSSAPI',
        "authentication_ldap_sasl_log_status": 5,
        "log_error_verbosity": 3,
        "net_read_timeout": 360,
        "connect_timeout": 360
    });

shell.options.mysqlPluginDir = MYSQL_PLUGIN_DIR;
var sasl_available = true;
shell.connect(__sandbox_uri1);

try {
    session.runSql("CREATE DATABASE test_user_db");
    session.runSql(`CREATE USER '${LDAP_KERBEROS_USER}@MYSQL.LOCAL' IDENTIFIED WITH authentication_ldap_sasl BY "${LDAP_KERBEROS_AUTH_STRING}"`);
    session.runSql("CREATE USER 'invalid_user' IDENTIFIED WITH authentication_ldap_sasl");
    session.runSql("CREATE USER 'mysql_engineering'");
    session.runSql("GRANT ALL PRIVILEGES ON test_user_db.* TO 'mysql_engineering'");
    session.runSql("GRANT ALL PRIVILEGES ON test_user_db.* TO 'invalid_user'");
    session.runSql(`GRANT PROXY on 'mysql_engineering' TO '${LDAP_KERBEROS_USER}@MYSQL.LOCAL'`);
} catch (error) {
    // The SASL authentication may not be available on these platforms, if that's the case we skip the tests
    if (['windows', "macos", "unknown"].includes(__os_type) && error.message.indexOf("Plugin 'authentication_ldap_sasl' is not loaded") != -1) {
        sasl_available = false;
    } else {
        throw error;
    }
}

session.close();

var test_list = {
    "SELECT current_user()": "mysql_engineering@%",
    "SELECT user()": `${LDAP_KERBEROS_USER}@MYSQL.LOCAL@localhost`,
    "SELECT @@local.proxy_user": `'${LDAP_KERBEROS_USER}@MYSQL.LOCAL'@'%'`,
    "SELECT @@local.external_user": "mysql_engineering"
};

// Cleans the Kerberos cache
testutil.callMysqlsh(["--py", "-i", "-e", "import os;os.system('kdestroy')"])

//@<> WL14553-TSFR_8_2 - Kerberos session with no user/password, should fail as there's no cached TGT {sasl_available}
args = ["--mysql", "--host=localhost",
    `--port=${__mysql_sandbox_port1}`,
    '--schema=test_user_db',
    '--auth-method=authentication_ldap_sasl_client',
    "--credential-store-helper=plaintext",
    `--mysql-plugin-dir=${MYSQL_PLUGIN_DIR}`]

// WL14553-TSFR_9_4 - No user/password provided
testutil.callMysqlsh(args.concat(["-i"]));

// System user is not automatically added to the connection data
EXPECT_OUTPUT_CONTAINS(`Creating a Classic session to 'localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=authentication_ldap_sasl_client`)

// The client library will pick the system user anyway
EXPECT_OUTPUT_CONTAINS(`SSO user not found, Please perform SSO authentication using kerberos.`)
WIPE_OUTPUT()

//@<> WL14553-TSFR_9_4 - User given but no password {sasl_available}
testutil.callMysqlsh(args.concat(["-i", `--user=${LDAP_KERBEROS_USER}@MYSQL.LOCAL`]));

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

//@<> Test TGT with independent shell instances {sasl_available}
for (variant_index in cli_variants) {
    for (query in test_list) {
        testutil.callMysqlsh(args.concat([`${query}`]).concat(cli_variants[variant_index]));
        EXPECT_OUTPUT_CONTAINS(test_list[query] ? test_list[query] : "NULL");
        WIPE_OUTPUT();
    }
}

//@<> Test TGT with interactive shell connections {sasl_available}
shell.options.mysqlPluginDir = MYSQL_PLUGIN_DIR;
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

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
