//@ {hasAuthEnvironment('LDAP_SIMPLE')}
//@<> Setup
if (__os_type == 'windows') {
    plugin = "authentication_ldap_simple.dll"
} else {
    plugin = "authentication_ldap_simple.so"
}

var server_conf = {
    "plugin-load-add": plugin,
    "authentication_ldap_simple_server_host": `${LDAP_SIMPLE_SERVER_HOST}`,
    "authentication_ldap_simple_server_port": parseInt(`${LDAP_SIMPLE_SERVER_PORT}`),
    "authentication_ldap_simple_bind_base_dn": `${LDAP_SIMPLE_BIND_BASE_DN}`
}

if (__os_type == 'windows') {
    server_conf['named_pipe'] = 1
    server_conf['socket'] = "MyNamedPipe"
}

testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', server_conf);

shell.options.mysqlPluginDir = `${MYSQL_PLUGIN_DIR}`;
shell.connect(__sandbox_uri1);
session.runSql("CREATE DATABASE test_user_db");
session.runSql(`CREATE USER '${LDAP_SIMPLE_USER}' IDENTIFIED WITH authentication_ldap_simple BY '${LDAP_SIMPLE_AUTH_STRING}'`);
session.runSql("CREATE USER 'common'");
session.runSql("GRANT ALL PRIVILEGES ON test_user_db.* TO 'common'");
session.runSql(`GRANT PROXY on 'common' TO '${LDAP_SIMPLE_USER}'`);
var socket = "";
var raw_socket = "";
var result = session.runSql("SHOW VARIABLES LIKE 'socket'");
if (result) {
    var row = result.fetchOne();
    if (row) {
        raw_socket = row[1];
        socket = testutil.getSandboxPath(__mysql_sandbox_port1, raw_socket);
    }
}

session.close();

//@<> Attempts without specifying the mysql_clear_password plugin
EXPECT_THROWS(function () {
    shell.connect(`${LDAP_SIMPLE_USER}:${LDAP_SIMPLE_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db`);
}, "Authentication plugin 'mysql_clear_password' cannot be loaded: plugin not enabled");


//@<> WL14553-TSFR_2_2 - Attempts using mysql_clear_password over TCP with ssl-mode=preferred
EXPECT_THROWS(function () {
    shell.connect(`${LDAP_SIMPLE_USER}:${LDAP_SIMPLE_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=mysql_clear_password&ssl-mode=preferred`);
}, "Clear password authentication requires a secure channel, please use ssl-mode=REQUIRED to guarantee a secure channel");

//@<> WL14553-TSFR_2_3 - Attempts using mysql_clear_password over TCP without SSL
EXPECT_THROWS(function () {
    shell.connect(`${LDAP_SIMPLE_USER}:${LDAP_SIMPLE_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=mysql_clear_password`);
}, "Clear password authentication is not supported over insecure channels");


var test_list = {
    "SELECT current_user()": "common@%",
    "SELECT user()": `${LDAP_SIMPLE_USER}@localhost`,
    "SELECT @@local.proxy_user": `'${LDAP_SIMPLE_USER}'@'%'`,
    "SELECT @@local.external_user": "common"
};

//@<> WL14553-TSFR_2_1 - LDAP Simple success connections
// TODO(rennox): Add windows test through pipes
var secure_connections = []
// TCP with ssl-mode=required
secure_connections.push(function () { shell.connect(`${LDAP_SIMPLE_USER}:${LDAP_SIMPLE_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=mysql_clear_password&ssl-mode=required`) });

// Same test but using the Shell API
secure_connections.push(function () { session = mysql.getSession(`${LDAP_SIMPLE_USER}:${LDAP_SIMPLE_PWD}@localhost:${__mysql_sandbox_port1}/test_user_db?auth-method=mysql_clear_password&ssl-mode=required`) });


if (__os_type == 'windows') {
    secure_connections.push(function () { shell.connect(`${LDAP_SIMPLE_USER}:${LDAP_SIMPLE_PWD}@\\\\.\\${raw_socket}/test_user_db?auth-method=mysql_clear_password`) });
} else {
    secure_connections.push(function () { shell.connect(`${LDAP_SIMPLE_USER}:${LDAP_SIMPLE_PWD}@(${socket})/test_user_db?auth-method=mysql_clear_password`) });

    // Same test but using the Shell API
    secure_connections.push(function () { session = mysql.getSession(`${LDAP_SIMPLE_USER}:${LDAP_SIMPLE_PWD}@(${socket})/test_user_db?auth-method=mysql_clear_password`) });
}

for (index in secure_connections) {
    // Connects
    secure_connections[index]();

    print(index);

    // Tests
    for (query in test_list) {
        var result = session.runSql(query);
        var row = result.fetchOne();
        EXPECT_EQ(test_list[query], row[0]);
    }

    // Disconnects
    session.close();
}

//@<> WL14553-TSFR_1_2 - Connections not using mysql_clear_password still connect
shell.connect(__sandbox_uri1)
var result = session.runSql("select current_user()");
var row = result.fetchOne();
EXPECT_EQ("root@localhost", row[0]);
session.close()


//@<> WL14553-TSFR_1_1 - Test CLI LDAP Simple Session - starting with correct auth-method
// Covers: TSFR_1_4
for (query in test_list) {
    testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, `--password=${LDAP_SIMPLE_PWD}`,
        "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
        "--auth-method=mysql_clear_password",
        "--ssl-mode=required",
        "--quiet-start=2", "--sql", "-e", `${query}`], "", ["LIBMYSQL_ENABLE_CLEARTEXT_PLUGIN=0"]);
    EXPECT_OUTPUT_CONTAINS(test_list[query]);
    WIPE_OUTPUT();
}

//@<> WL14553-TSFR_1_5 - Test CLI LDAP Simple Session - starting with ssl-mode=DISABLED
testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, `--password=${LDAP_SIMPLE_PWD}`,
    "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
    "--auth-method=mysql_clear_password",
    "--ssl-mode=disabled",
    "--quiet-start=2", "--sql", "-e", `${query}`]);
EXPECT_OUTPUT_CONTAINS("Clear password authentication is not supported over insecure channels.");
WIPE_OUTPUT();


//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
