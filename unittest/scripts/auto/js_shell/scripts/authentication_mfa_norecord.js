//@ {__dbug}

const dbug_log = os.path.join(__tmp_dir, "authentication_mfa_dbug.log");
const base_connection_data = shell.parseUri(__mysqluripwd);
const password1 = base_connection_data.password;
delete base_connection_data.password;
const credential_url = shell.unparseUri(base_connection_data);
const password2 = "cache-password2";
const password3 = "cache-password3";

function expect_mfa_passwords(trace) {
  EXPECT_TRUE(trace.includes(`MYSQL_OPT_USER_PASSWORD: 1: ${password1}`));
  EXPECT_TRUE(trace.includes(`MYSQL_OPT_USER_PASSWORD: 2: ${password2}`));
  EXPECT_TRUE(trace.includes(`MYSQL_OPT_USER_PASSWORD: 3: ${password3}`));
}

function capture_mfa_passwords(callback) {
  testutil.rmfile(dbug_log);
  testutil.dbugSet(`+d,sqlconnectpwd:o,${dbug_log}`);
  try {
    callback();
  } finally {
    testutil.dbugSet("");
  }

  return testutil.catFile(dbug_log);
}

const connection_data = Object.assign({}, base_connection_data, {
  password: password1,
  password2: password2,
  password3: password3
});

//@<> Command line password1..3 options are forwarded to libmysql
testutil.rmfile(dbug_log);
EXPECT_EQ(0, testutil.callMysqlsh([
  `--debug=+d,sqlconnectpwd:o,${dbug_log}`,
  credential_url,
  `--password1=${password1}`,
  `--password2=${password2}`,
  `--password3=${password3}`,
  "--quiet-start=2",
  "--sqlc",
  "-e",
  "SELECT 1"
]));
expect_mfa_passwords(testutil.catFile(dbug_log));

//@<> password is an alias for password1
testutil.rmfile(dbug_log);
EXPECT_EQ(0, testutil.callMysqlsh([
  `--debug=+d,sqlconnectpwd:o,${dbug_log}`,
  credential_url,
  `--password=${password1}`,
  `--password2=${password2}`,
  `--password3=${password3}`,
  "--quiet-start=2",
  "--sqlc",
  "-e",
  "SELECT 1"
]));
expect_mfa_passwords(testutil.catFile(dbug_log));

//@<> password1 takes precedence over password
testutil.rmfile(dbug_log);
EXPECT_EQ(0, testutil.callMysqlsh([
  `--debug=+d,sqlconnectpwd:o,${dbug_log}`,
  credential_url,
  "--password=wrong-password",
  `--password1=${password1}`,
  `--password2=${password2}`,
  `--password3=${password3}`,
  "--quiet-start=2",
  "--sqlc",
  "-e",
  "SELECT 1"
]));
expect_mfa_passwords(testutil.catFile(dbug_log));

//@<> MFA through Shell APIs
expect_mfa_passwords(capture_mfa_passwords(() => {
  shell.connect(connection_data);
}));

expect_mfa_passwords(capture_mfa_passwords(() => {
  const mfa_session = mysql.getSession(connection_data);
  mfa_session.close();
}));

//@<> MFA credential cache through Shell APIs
const credential_helper = shell.options["credentialStore.helper"];
shell.options["credentialStore.helper"] = "plaintext";
shell.deleteAllCredentials();
shell.storeCredential(credential_url, {
  password: password1,
  password2: password2,
  password3: password3
});

expect_mfa_passwords(capture_mfa_passwords(() => {
  shell.connect(Object.assign({}, base_connection_data, {
    password: null,
    password2: null,
    password3: null
  }));
}));
testutil.assertNoPrompts();

//@<> Missing cached passwords are prompted and forwarded to libmysql
const save_passwords = shell.options["credentialStore.savePasswords"];
const use_wizards = shell.options["useWizards"];
shell.options["credentialStore.savePasswords"] = "prompt";
shell.options["useWizards"] = true;
shell.deleteAllCredentials();
shell.storeCredential(credential_url, {password: password1});

testutil.expectPassword(
    `Please provide the password2 for '${credential_url}': `,
    password2);
testutil.expectPassword(
    `Please provide the password3 for '${credential_url}': `,
    password3);
testutil.expectPrompt(
    `Save all prompted passwords for '${credential_url}'?`,
    "N");
expect_mfa_passwords(capture_mfa_passwords(() => {
  shell.connect(Object.assign({}, base_connection_data, {
    password: null,
    password2: null,
    password3: null
  }));
}));
testutil.assertNoPrompts();

//@<> MFA is not available for X protocol with connection data
EXPECT_THROWS(function() {
  mysqlx.getSession(connection_data);
}, "Multi-factor authentication is only compatible with MySQL protocol");

//@<> MFA is not available for X protocol
testutil.callMysqlsh([
  credential_url,
  `--password1=${password1}`,
  `--password2=${password2}`,
  `--password3=${password3}`,
  "--quiet-start=2",
  "--sqlx",
  "-e",
  "SELECT 1"
]);
EXPECT_OUTPUT_CONTAINS(
    "Multi-factor authentication is only compatible with MySQL protocol");
WIPE_OUTPUT();

//@<> Cleanup
shell.deleteAllCredentials();
shell.options["credentialStore.savePasswords"] = save_passwords;
shell.options["useWizards"] = use_wizards;
shell.options["credentialStore.helper"] = credential_helper;
testutil.rmfile(dbug_log);
session.close();
