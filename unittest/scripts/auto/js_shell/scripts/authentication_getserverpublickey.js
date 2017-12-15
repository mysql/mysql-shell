// Test authentication using mysql_native_password, only MySQL 8.0.4+

// Entry points:
//  mysqlsh executable
//  shell.connect()
//  mysql.getClassicSession() and mysqlx.getSession()

// Variations:
//  classic and x proto
//  @localhost and @%
//  with password and without
//  with cached password and without
//  without ssl and with --get-server-public-key and without

// Test cases:
//  with correct password and without
//  with correct user and without

//@ GlobalSetUp {VER(>=8.0.4)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
var rootsess = mysql.getClassicSession(__sandbox_uri1);

rootsess.runSql("CREATE USER local_blank@localhost IDENTIFIED WITH caching_sha2_password BY ''");
rootsess.runSql("CREATE USER local_pass@localhost IDENTIFIED WITH caching_sha2_password BY 'pass'");
rootsess.runSql("CREATE USER remo_blank@'%' IDENTIFIED WITH caching_sha2_password BY ''");
rootsess.runSql("CREATE USER remo_pass@'%' IDENTIFIED WITH caching_sha2_password BY 'pass'");

// Try all combinations using caching_sha2_password
// ================================================
// This new plugin has a peculiar behaviour:
// 1st authentication must be either without SSL and --get-server-public-key or with --get-server-public-key
// after that, it can be without SSL
// UNLESS password is not set
//
// in X protocol, --get-server-public-key doesn't work yet


// ==== user:local_pass / password:pass (SUCCESS)
//@ session classic -- user:local_pass / password:pass  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl_getpubkey);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell connect classic -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var s = shell.connect(uri_nossl_getpubkey);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect x -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell classic -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ shell x -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '--get-server-public-key', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// ==== user:remo_pass / password:pass (SUCCESS)
//@ session classic -- user:remo_pass / password:pass  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl_getpubkey);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell connect classic -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var s = shell.connect(uri_nossl_getpubkey);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect x -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell classic -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ shell x -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// ==== user:local_blank / password: (SUCCESS)
//@ session classic -- user:local_blank / password:  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should succeed b/c localhost acc with no pass skips this
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl_getpubkey);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell connect classic -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should succeed b/c localhost nopass
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var s = shell.connect(uri_nossl_getpubkey);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect x -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell classic -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should succeed b/c nopass
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ shell x -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// ==== user:remo_blank / password: (SUCCESS)
//@ session classic -- user:remo_blank / password:  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should succeed b/c nopass
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl_getpubkey);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell connect classic -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should succeed b/c blank password
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var s = shell.connect(uri_nossl_getpubkey);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect x -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell classic -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should succeed b/c blank password
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect again without SSL and --get-server-public-key - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ shell x -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// ==== user:local_blank / password:pass (FAIL)
//@ session classic -- user:local_blank / password:pass  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl_getpubkey)}, "Access denied for user 'local_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ session x -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell connect classic -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl_getpubkey)}, "Access denied for user 'local_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ shell connect x -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell classic -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ shell x -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '--get-server-public-key', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// ==== user:remo_blank / password:pass (FAIL)
//@ session classic -- user:remo_blank / password:pass  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl_getpubkey)}, "Access denied for user 'remo_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ session x -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell connect classic -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl_getpubkey)}, "Access denied for user 'remo_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ shell connect x -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell classic -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again without SSL and --get-server-public-key - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ shell x -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// ==== user:local_pass / password: (FAIL)
//@ session classic -- user:local_pass / password:  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Access denied for user 'local_pass'@'localhost'");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl_getpubkey)}, "Access denied for user 'local_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Access denied for user 'local_pass'@'localhost'");

//@ session x -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell connect classic -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_pass'@'localhost'");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl_getpubkey)}, "Access denied for user 'local_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_pass'@'localhost'");

//@ shell connect x -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell classic -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost' ");

// Connect again without SSL and --get-server-public-key - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost' ");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost'");

//@ shell x -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// ==== user:remo_pass / password: (FAIL)
//@ session classic -- user:remo_pass / password:  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' ");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl_getpubkey)}, "Access denied for user 'remo_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' ");

//@ session x -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell connect classic -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost'");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl_getpubkey)}, "Access denied for user 'remo_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost'");

//@ shell connect x -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl_getpubkey)}, "X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Invalid authentication method: PLAIN over unsecure channel");

//@ shell classic -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost'");

// Connect again without SSL and --get-server-public-key - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost'");

//@ shell x -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_nossl_getpubkey = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED&get-server-public-key=true';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

// Connect again without SSL and --get-server-public-key - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl_getpubkey, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("X Protocol: Option get-server-public-key is not supported.");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid authentication method: PLAIN over unsecure channel");

//@ GlobalTearDown {VER(>=8.0.4)}
rootsess.close();
testutil.destroySandbox(__mysql_sandbox_port1);
