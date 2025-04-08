//@ {hasAuthEnvironment('OPENID_CONNECT')}

//@<> Setup
if (debugAuthPlugins()) {
    dba.verbose = 1;
}

var OPENID_CONNECT_USER = "openid-test";
var OPENID_CONNECT_SUBJECT = "openid-test-subject";
var OPENID_CONNECT_PROVIDER = "openid-test-issuer";

var sandbox_uri = __sandbox_uri1;
var sandbox_port = __mysql_sandbox_port1;
var cnfg_path = os.path.join(__data_path, "auth", "openid-test-jwk-cfg");
var token_path = os.path.join(__data_path, "auth", "openid-test-jwt.txt");
var token_path_invalid = os.path.join(__data_path, "auth", "openid-test-invalid-jwt.txt");
var mysqlsh_cnfg_invalid_path = os.path.join(__data_path, "auth", "openid-mysqlsh-invalid.cnf");
var mysqlsh_cnfg_replace_path = os.path.join(__data_path, "auth", "openid-mysqlsh.cnf");
var mysqlsh_cnfg_path = os.path.join(os.getcwd(), "otherconfig.cfg");
var test_uri = "";

var server_conf = getAuthServerConfig('OPENID_CONNECT');
testutil.deployRawSandbox(sandbox_port, 'root', server_conf, { "timeout": 120 });
var openid_available = isAuthMethodSupported('OPENID_CONNECT', sandbox_uri);

try {
    shell.connect(sandbox_uri);

    if (openid_available) {
        // setup
        session.runSql('SET GLOBAL authentication_openid_connect_configuration=?', [`file://${cnfg_path}`]);
        session.runSql(`CREATE USER '${OPENID_CONNECT_USER}' IDENTIFIED WITH authentication_openid_connect BY '{"identity_provider":"${OPENID_CONNECT_PROVIDER}","user":"${OPENID_CONNECT_SUBJECT}"}';`);
        session.runSql("CREATE USER 'other_user' IDENTIFIED BY 'mypwd'");
        session.runSql(`GRANT ALL PRIVILEGES ON *.* TO '${OPENID_CONNECT_USER}'`);
        session.runSql(`GRANT ALL PRIVILEGES ON *.* TO 'other_user'`);
    } else {
        session.runSql(`CREATE USER '${OPENID_CONNECT_USER}' IDENTIFIED BY 'mypwd';`);
    }

    session.close();
} catch (error) {
    println(testutil.catFile(testutil.getSandboxLogPath(sandbox_port)));
    throw error;
}


//@<> no user
testutil.callMysqlsh([
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    '--password=""',
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_CONTAINS(`Access denied for user '${__system_user}'@'localhost'`);

//@<> proper user, invalid auth 
testutil.callMysqlsh([
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    `--user=${OPENID_CONNECT_USER}`,
    '--password=""',
    "--sql", "-e", "SELECT current_user()",
]);
if (__version_num >= 90100 && openid_available) { // support was added in 9.1.0
    EXPECT_OUTPUT_CONTAINS("The path to ID token file is not set.");
} else {
    EXPECT_OUTPUT_CONTAINS("Access denied for user 'openid-test'@'localhost'");
}

//@<> WL16470#FR3_1 - cmdline - proper user, proper auth, no token file
testutil.callMysqlsh([
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    `--user=${OPENID_CONNECT_USER}`,
    '--auth-method=authentication_openid_connect_client',
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_CONTAINS(`The path to ID token file is not set.`);

//@<> WL16470#FR3_1 - uri - proper user, proper auth, no token file
test_uri = shell.unparseUri({
    "host": "localhost",
    "schema": "mysql",
    "port": `${sandbox_port}`,
    "user": `${OPENID_CONNECT_USER}`,
    "auth-method": "authentication_openid_connect_client",
});
testutil.callMysqlsh([
    `${test_uri}`,
    "--mysql",
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_CONTAINS(`The path to ID token file is not set.`);

//@<> WL16470#FR3_1 - dictionary - proper user, proper auth, no token file
EXPECT_THROWS(function () {
    shell.connect({
        'host': 'localhost',
        'schema': 'mysql',
        'port': `${sandbox_port}`,
        'user': `${OPENID_CONNECT_USER}`,
        'password': '',
        'auth-method': 'authentication_openid_connect_client'
    });
    EXPECT_OUTPUT_CONTAINS(`The path to ID token file is not set.`);
}, "Unknown MySQL error");
session.close();

//@<> WL16470#FR3_1 - config - proper user, proper auth, non existant token file provided
testutil.callMysqlsh([
    `--defaults-file=${mysqlsh_cnfg_invalid_path}`,
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    `--user=${OPENID_CONNECT_USER}`,
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_CONTAINS('Token file for openid connect authorization does not exist for authentication_openid_connect_client plugin.');

//@<> WL16470#FR3_1 - cmdline - proper user, proper auth, invalid token file
testutil.callMysqlsh([
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    `--user=${OPENID_CONNECT_USER}`,
    `--authentication-openid-connect-client-id-token-file=${token_path_invalid}`,
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`);

//@<> WL16470#FR3_1 - uri - proper user, proper auth, invalid token file
test_uri = shell.unparseUri({
    "host": "localhost",
    "schema": "mysql",
    "port": `${sandbox_port}`,
    "user": `${OPENID_CONNECT_USER}`,
    "auth-method": "authentication_openid_connect_client",
    "authentication-openid-connect-client-id-token-file": `${token_path_invalid}`,
});
testutil.callMysqlsh([
    `${test_uri}`,
    "--mysql",
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`)

//@<> WL16470#FR3_1 - dictionary - proper user, proper auth, invalid token file
EXPECT_THROWS(function () {
    shell.connect({
        'host': 'localhost',
        'schema': 'mysql',
        'port': `${sandbox_port}`,
        'user': `${OPENID_CONNECT_USER}`,
        'authentication-openid-connect-client-id-token-file': `${token_path_invalid}`
    });
}, "Access denied for user 'openid-test'@'localhost'");
session.close()

//@<> WL16470#FR3_1 - config - proper user, proper auth, invalid token file
{
    var config_file = testutil.catFile(mysqlsh_cnfg_replace_path);
    config_file = config_file.replace("[REPLACE]", token_path_invalid.replace(/\\/g, "/"));
    testutil.createFile(mysqlsh_cnfg_path, config_file);
}
testutil.callMysqlsh([
    `--defaults-file=${mysqlsh_cnfg_path}`,
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    `--user=${OPENID_CONNECT_USER}`,
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`)


//@<> WL16470#FR3_2 - cmdline - proper user, proper auth, valid token file {openid_available}
testutil.callMysqlsh([
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    `--user=${OPENID_CONNECT_USER}`,
    `--authentication-openid-connect-client-id-token-file=${token_path}`,
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_NOT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`);
EXPECT_OUTPUT_CONTAINS(`current_user()`);
EXPECT_OUTPUT_CONTAINS(`${OPENID_CONNECT_USER}`);

//@<> WL16470#FR3_2 - uri - proper user, proper auth, valid token file {openid_available}
test_uri = shell.unparseUri({
    "host": "localhost",
    "schema": 'mysql',
    "port": `${sandbox_port}`,
    "user": `${OPENID_CONNECT_USER}`,
    "auth-method": "authentication_openid_connect_client",
    "authentication-openid-connect-client-id-token-file": `${token_path}`,
});
testutil.callMysqlsh([
    `${test_uri}`,
    "--mysql",
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_NOT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`)
EXPECT_OUTPUT_CONTAINS(`current_user()`);
EXPECT_OUTPUT_CONTAINS(`${OPENID_CONNECT_USER}`);

//@<> WL16470#FR3_2 - dictionary - proper user, proper auth, valid token file {openid_available}
shell.connect({
    'host': 'localhost',
    'schema': 'mysql',
    'port': `${sandbox_port}`,
    'user': `${OPENID_CONNECT_USER}`,
    'authentication-openid-connect-client-id-token-file': `${token_path}`,
});
var result = session.runSql('SELECT current_user()').fetchOne()[0];
EXPECT_OUTPUT_NOT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`);
EXPECT_CONTAINS(`${OPENID_CONNECT_USER}`, result);
session.close();

//@<> WL16470#FR3_2 - config - proper user, proper auth, valid token file {openid_available}
{
    var config_file = testutil.catFile(mysqlsh_cnfg_replace_path);
    config_file = config_file.replace("[REPLACE]", token_path.replace(/\\/g, "/"));
    testutil.createFile(mysqlsh_cnfg_path, config_file);
}
testutil.callMysqlsh([
    `--defaults-file=${mysqlsh_cnfg_path}`,
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    `--user=${OPENID_CONNECT_USER}`,
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_NOT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`);
EXPECT_OUTPUT_CONTAINS(`current_user()`);
EXPECT_OUTPUT_CONTAINS(`${OPENID_CONNECT_USER}`);

//@<> WL16470#FR5_1 - config - another user, other password, openid-auth auth, valid token file {openid_available}
{
    var config_file = testutil.catFile(mysqlsh_cnfg_replace_path);
    config_file = config_file.replace("[REPLACE]", token_path.replace(/\\/g, "/"));
    config_file = config_file + '\npassword=mypwd';
    testutil.createFile(mysqlsh_cnfg_path, config_file);
}
testutil.callMysqlsh([
    `--defaults-file=${mysqlsh_cnfg_path}`,
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    `--user=other_user`,
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_NOT_CONTAINS(`Access denied for user 'other_user'@'localhost'`);
EXPECT_OUTPUT_CONTAINS(`current_user()`);
EXPECT_OUTPUT_CONTAINS(`other_user`);

//@<> Bug#36968554 - Shell asks for password when using OpenID connect {openid_available}
testutil.callMysqlsh([
    "--mysql",
    "--host=localhost",
    "--schema=mysql",
    `--port=${sandbox_port}`,
    `--user=${OPENID_CONNECT_USER}`,
    `--authentication-openid-connect-client-id-token-file=${token_path}`,
    "--sql", "-e", "SELECT current_user()",
]);
EXPECT_OUTPUT_NOT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`);
EXPECT_OUTPUT_NOT_CONTAINS(`Please provide the password for '${OPENID_CONNECT_USER}'`);
EXPECT_OUTPUT_CONTAINS(`current_user()`);
EXPECT_OUTPUT_CONTAINS(`${OPENID_CONNECT_USER}`);

//@<> Bug#36968640 - Shell recalls auth settings with other connect command after initial login {openid_available}
//first valid connection - openid user with token
shell.connect({
    'host': 'localhost',
    'schema': 'mysql',
    'port': `${sandbox_port}`,
    'user': `${OPENID_CONNECT_USER}`,
    'authentication-openid-connect-client-id-token-file': `${token_path}`,
});
var result = session.runSql('SELECT current_user()').fetchOne()[0];
EXPECT_OUTPUT_NOT_CONTAINS(`Access denied for user '${OPENID_CONNECT_USER}'@'localhost'`);
EXPECT_CONTAINS(`${OPENID_CONNECT_USER}`, result);

//second valid connection - another user
shell.connect({
    'host': 'localhost',
    'schema': 'mysql',
    'port': `${sandbox_port}`,
    'user': `other_user`,
    'password': 'mypwd',
});
var result = session.runSql('SELECT current_user()').fetchOne()[0];
EXPECT_OUTPUT_NOT_CONTAINS(`Access denied for user 'other_user'@'localhost'`);
EXPECT_CONTAINS(`other_user`, result);

//third connection - openid user without token - should fail
EXPECT_THROWS(function () {
    shell.connect({
        'host': 'localhost',
        'schema': 'mysql',
        'port': `${sandbox_port}`,
        'user': `${OPENID_CONNECT_USER}`,
        'password':'',
    });
}, "Unknown MySQL error");
session.close()


//@<> CleanUp
if (debugAuthPlugins()) {
    println(testutil.catFile(testutil.getSandboxLogPath(sandbox_port)));
}

testutil.destroySandbox(sandbox_port);
