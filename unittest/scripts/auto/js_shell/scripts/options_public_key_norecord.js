//@ GlobalSetUp {VER(>=8.0.4)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
var rootsess = mysql.getClassicSession(__sandbox_uri1);

rootsess.runSql("CREATE USER local_blank@localhost IDENTIFIED WITH caching_sha2_password BY ''");
rootsess.runSql("CREATE USER local_pass@localhost IDENTIFIED WITH caching_sha2_password BY 'pass'");
rootsess.runSql("CREATE USER remo_blank@'%' IDENTIFIED WITH caching_sha2_password BY ''");
rootsess.runSql("CREATE USER remo_pass@'%' IDENTIFIED WITH caching_sha2_password BY 'pass'");

const uri_nossl = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
const x_uri_nossl = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';

const sandbox_path = testutil.getSandboxPath(__mysql_sandbox_port1);
const public_key = sandbox_path + '/sandboxdata/public_key.pem';
const public_key_non_existing_path = sandbox_path + '/sandboxdata/no-such-file.pem';
const public_key_non_pem = sandbox_path + '/sandboxdata/private_key.pem';

//@ classic, get-server-public-key yes,false,true in URI {VER(>=8.0.4)}
rootsess.runSql('flush privileges');

var rc = testutil.callMysqlsh([uri_nossl + '&get-server-public-key=yes', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid URI: Invalid value 'yes' for 'get-server-public-key'. Allowed values: true, false, 1, 0.");

var rc = testutil.callMysqlsh([uri_nossl + '&get-server-public-key=false', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

var rc = testutil.callMysqlsh([uri_nossl + '&get-server-public-key=true', '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("ClassicSession:local_pass");

//@ classic, get-server-public-key no,0,1 in URI {VER(>=8.0.4)}
rootsess.runSql('flush privileges');

var rc = testutil.callMysqlsh([uri_nossl + '&get-server-public-key=no', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid URI: Invalid value 'no' for 'get-server-public-key'. Allowed values: true, false, 1, 0.");

var rc = testutil.callMysqlsh([uri_nossl + '&get-server-public-key=0', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

var rc = testutil.callMysqlsh([uri_nossl + '&get-server-public-key=1', '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("ClassicSession:local_pass");

//@ X, get-server-public-key yes,false,true in URI {VER(>=8.0.4)}
rootsess.runSql('flush privileges');

var rc = testutil.callMysqlsh([x_uri_nossl + '&get-server-public-key=yes', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid URI: Invalid value 'yes' for 'get-server-public-key'. Allowed values: true, false, 1, 0.");

var rc = testutil.callMysqlsh([x_uri_nossl + '&get-server-public-key=false', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");

var rc = testutil.callMysqlsh([x_uri_nossl + '&get-server-public-key=true', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");

//@ X, get-server-public-key no,0,1 in URI {VER(>=8.0.4)}
rootsess.runSql('flush privileges');

var rc = testutil.callMysqlsh([x_uri_nossl + '&get-server-public-key=no', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid URI: Invalid value 'no' for 'get-server-public-key'. Allowed values: true, false, 1, 0.");

var rc = testutil.callMysqlsh([x_uri_nossl + '&get-server-public-key=0', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");

var rc = testutil.callMysqlsh([x_uri_nossl + '&get-server-public-key=1', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");

//@ classic, --get-server-public-key in command option {VER(>=8.0.4)}
rootsess.runSql('flush privileges');

var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

var rc = testutil.callMysqlsh([uri_nossl, '--get-server-public-key', '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ X, --get-server-public-key in command option {VER(>=8.0.4)}
rootsess.runSql('flush privileges');

var rc = testutil.callMysqlsh([x_uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost' (using password: YES)");

var rc = testutil.callMysqlsh([x_uri_nossl, '--get-server-public-key', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");


//@ classic, server-public-key-path in URI {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

var rc = testutil.callMysqlsh([uri_nossl + '&server-public-key-path=(' + public_key + ')', '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("ClassicSession:local_pass");

var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("ClassicSession:local_pass");


//@ X, server-public-key-path in URI {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var rc = testutil.callMysqlsh([x_uri_nossl + '&server-public-key-path=(' + public_key + ')', '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option server-public-key-path is not supported.");


//@ classic, --server-public-key-path in command option {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

var rc = testutil.callMysqlsh([uri_nossl, '--server-public-key-path=' + public_key, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("ClassicSession:local_pass");

var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);
EXPECT_STDOUT_CONTAINS("ClassicSession:local_pass");


//@ X, --server-public-key-path in command option {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var rc = testutil.callMysqlsh([x_uri_nossl, '--server-public-key-path=' + public_key, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option server-public-key-path is not supported.");

//@ classic, non existing public key path (F10) {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var rc = testutil.callMysqlsh([uri_nossl, '--server-public-key-path=' + public_key_non_existing_path, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
if (testutil.versionCheck(__version, ">=", "8.0.13"))
  EXPECT_STDOUT_CONTAINS("[Warning] Failed to locate server public key '" + public_key_non_existing_path + "'");
else
  EXPECT_STDOUT_CONTAINS("[Warning] Can't locate server public key '" + public_key_non_existing_path + "'");

// fallback to --get-server-public-key
rootsess.runSql('flush privileges');
var rc = testutil.callMysqlsh([uri_nossl, '--server-public-key-path=' + public_key_non_existing_path, '--get-server-public-key', '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);
if (testutil.versionCheck(__version, ">=", "8.0.13"))
  EXPECT_STDOUT_CONTAINS("[Warning] Failed to locate server public key '" + public_key_non_existing_path + "'");
else
  EXPECT_STDOUT_CONTAINS("[Warning] Can't locate server public key '" + public_key_non_existing_path + "'");

EXPECT_STDOUT_CONTAINS("ClassicSession:local_pass");

//@ classic, invalid pem file in public key path (F11) {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var rc = testutil.callMysqlsh([uri_nossl, '--server-public-key-path=' + public_key_non_pem, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
if (testutil.versionCheck(__version, ">=", "8.0.13"))
  EXPECT_STDOUT_CONTAINS("[Warning] Public key is not in Privacy Enhanced Mail format: '" + public_key_non_pem + "'");
else
  EXPECT_STDOUT_CONTAINS("[Warning] Public key is not in PEM format: '" + public_key_non_pem + "'");

// fallback to --get-server-public-key
rootsess.runSql('flush privileges');
var rc = testutil.callMysqlsh([uri_nossl, '--server-public-key-path=' + public_key_non_pem, '--get-server-public-key', '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);
if (testutil.versionCheck(__version, ">=", "8.0.13"))
  EXPECT_STDOUT_CONTAINS("[Warning] Public key is not in Privacy Enhanced Mail format: '" + public_key_non_pem + "'");
else
  EXPECT_STDOUT_CONTAINS("[Warning] Public key is not in PEM format: '" + public_key_non_pem + "'");
EXPECT_STDOUT_CONTAINS("ClassicSession:local_pass");

//@ GlobalTearDown {VER(>=8.0.4)}
rootsess.close();
testutil.destroySandbox(__mysql_sandbox_port1);
