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

var test_list = {
  "SELECT current_user()": "common@%",
  "SELECT user()": `${LDAP_SIMPLE_USER}@localhost`
};

testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', server_conf);

shell.options.mysqlPluginDir = `${MYSQL_PLUGIN_DIR}`;
shell.connect(__sandbox_uri1);
session.runSql("CREATE DATABASE test_user_db");
session.runSql(`CREATE USER '${LDAP_SIMPLE_USER}' IDENTIFIED BY 'abc'`);
session.runSql("CREATE USER 'common'");
session.runSql("GRANT ALL PRIVILEGES ON test_user_db.* TO 'common'");
session.runSql(`GRANT PROXY on 'common' TO '${LDAP_SIMPLE_USER}'`);

//@<> 1 factor authentication
testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, "--password1=abc",
    "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
    "--quiet-start=2", "--sqlc", "-e", "select 'Hello'"]);
EXPECT_OUTPUT_CONTAINS("Hello");
WIPE_OUTPUT();

testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, "--password=abc",
    "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
    "--quiet-start=2", "--sqlc", "-e", "select 'Hello'"]);
EXPECT_OUTPUT_CONTAINS("Hello");
WIPE_OUTPUT();

//@<> 1 factor authentication X protocol
testutil.callMysqlsh(['--mysqlx', `-u${LDAP_SIMPLE_USER}`, "--password1=abc",
    "-hlocalhost", `--port=${__mysql_sandbox_port1}0`,
    "--quiet-start=2", "-e", "select 'Hello'"]);
EXPECT_OUTPUT_CONTAINS("Hello");
WIPE_OUTPUT();

//@<> " Double password givem through alias options, last one overwrites the first one"
testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, "--password=wrong-password", "--password1=abc",
      "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
      "--quiet-start=2", "--sqlc", "-e", "select 'Hello'"]);
EXPECT_OUTPUT_CONTAINS("Hello");
WIPE_OUTPUT();

//@<> 2 factor authentication
session.runSql(`ALTER USER '${LDAP_SIMPLE_USER}' ADD 2 FACTOR IDENTIFIED WITH authentication_ldap_simple BY '${LDAP_SIMPLE_AUTH_STRING}'`)
for (query in test_list) {
  testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, "--password=abc", `--password2=${LDAP_SIMPLE_PWD}`,
      "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
      "--auth-method=mysql_clear_password",
      "--ssl-mode=required",
      "--quiet-start=2", "--sqlc", "-e", `${query}`], "", ["LIBMYSQL_ENABLE_CLEARTEXT_PLUGIN=0"]);
  EXPECT_OUTPUT_CONTAINS(test_list[query]);
  WIPE_OUTPUT();
}

//@<> 2nd factor wrong password
testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, "--password1=abc", "--password2=wrong",
      "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
      "--auth-method=mysql_clear_password",
      "--ssl-mode=required",
      "--quiet-start=2", "--sqlc", "-e", `${query}`], "", ["LIBMYSQL_ENABLE_CLEARTEXT_PLUGIN=0"]);
EXPECT_OUTPUT_CONTAINS("Access denied for user 'admin'@'localhost' (using password: YES)");
WIPE_OUTPUT();


//@<> 3 factor authentication
session.runSql(`ALTER USER '${LDAP_SIMPLE_USER}' ADD 3 FACTOR IDENTIFIED WITH authentication_ldap_simple BY '${LDAP_SIMPLE_AUTH_STRING}'`)
for (query in test_list) {
  testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, 
      "--password1=abc", `--password2=${LDAP_SIMPLE_PWD}`, `--password3=${LDAP_SIMPLE_PWD}`,
      "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
      "--auth-method=mysql_clear_password",
      "--ssl-mode=required",
      "--quiet-start=2", "--sqlc", "-e", `${query}`], "", ["LIBMYSQL_ENABLE_CLEARTEXT_PLUGIN=0"]);
  EXPECT_OUTPUT_CONTAINS(test_list[query]);
  WIPE_OUTPUT();
}

//@<> 3rd factor missing
testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, "--password=abc",
      `--password2=${LDAP_SIMPLE_PWD}`,
      "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
      "--auth-method=mysql_clear_password",
      "--ssl-mode=required",
      "--quiet-start=2", "--sqlc", "-e", `${query}`], "", ["LIBMYSQL_ENABLE_CLEARTEXT_PLUGIN=0"]);
EXPECT_OUTPUT_CONTAINS("Access denied for user 'admin'@'localhost' (using password: YES)");
WIPE_OUTPUT();

//@<> MFA not available for X protocol
testutil.callMysqlsh([`-u${LDAP_SIMPLE_USER}`, "--password=abc",
      `--password2=${LDAP_SIMPLE_PWD}`, `--password3=${LDAP_SIMPLE_PWD}`,
      "-hlocalhost", `--port=${__mysql_sandbox_port1}`,
      "--auth-method=mysql_clear_password",
      "--ssl-mode=required",
      "--quiet-start=2", "--sqlx", "-e", `${query}`], "", ["LIBMYSQL_ENABLE_CLEARTEXT_PLUGIN=0"]);
EXPECT_OUTPUT_CONTAINS("Multi-factor authentication is only compatible with MySQL protocol");
WIPE_OUTPUT();

session.close();

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
