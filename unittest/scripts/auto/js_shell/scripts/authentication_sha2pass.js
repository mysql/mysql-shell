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
//  with ssl and without

// Test cases:
//  with correct password and without
//  with correct user and without

// TODO: handling for --get-server-public-key

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
// 1st authentication must be either with SSL or with --get-server-public-key
// after that, it can be without SSL
// UNLESS password is not set
//
// in X protocol, --get-server-public-key doesn't work yet


// ==== user:local_pass / password:pass (SUCCESS)
//@ session classic -- user:local_pass / password:pass  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'local_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (SUCCESS)
var s = mysqlx.getSession(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysqlx.getSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (SUCCESS)
var s = shell.connect(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect x -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (SUCCESS)
var s = shell.connect(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell classic -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_ssl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ shell x -- user:local_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_ssl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// ==== user:remo_pass / password:pass (SUCCESS)
//@ session classic -- user:remo_pass / password:pass  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (SUCCESS)
var s = mysqlx.getSession(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysqlx.getSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (SUCCESS)
var s = shell.connect(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect x -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (SUCCESS)
var s = shell.connect(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell classic -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_ssl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ shell x -- user:remo_pass / password:pass (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_ssl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// ==== user:local_blank / password: (SUCCESS)
//@ session classic -- user:local_blank / password:  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should succeed b/c localhost acc with no pass skips this
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect again with SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'local_blank'@'localhost' (using password: YES)");

// TODO() - ther's a bug in xplugin that won't let a user with no password login
// // Connect again with SSL - should (SUCCESS)
// var s = mysqlx.getSession(uri_ssl);
// EXPECT_TRUE(s.isOpen());
// s.close();
//
// // Connect once more without SSL - should (SUCCESS)
// var s = mysqlx.getSession(uri_nossl);
// EXPECT_TRUE(s.isOpen());
// s.close();

//@ shell connect classic -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should succeed b/c localhost nopass
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect again with SSL - should (SUCCESS)
var s = shell.connect(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect x -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_blank'@'localhost' (using password: YES)");

// TODO() - disabled because of xplugin bug
// // Connect again with SSL - should (SUCCESS)
// var s = shell.connect(uri_ssl);
// EXPECT_TRUE(s.isOpen());
// s.close();
//
// // Connect once more without SSL - should (SUCCESS)
// var s = shell.connect(uri_nossl);
// EXPECT_TRUE(s.isOpen());
// s.close();

//@ shell classic -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should succeed b/c nopass
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect again with SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_ssl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ shell x -- user:local_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_blank'@'localhost' (using password: YES)");

// TODO() - disabled because of xplugin bug
// // Connect again with SSL - should (SUCCESS)
// var rc = testutil.callMysqlsh([uri_ssl, '--password=', '-e', 'println(session)']);
// EXPECT_EQ(0, rc);
//
// // Connect once more without SSL - should (SUCCESS)
// var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
// EXPECT_EQ(0, rc);

// ==== user:remo_blank / password: (SUCCESS)
//@ session classic -- user:remo_blank / password:  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should succeed b/c nopass
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect again with SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = mysql.getClassicSession(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// TODO() - xplugin bug
// Connect again with SSL - should (SUCCESS)
// var s = mysqlx.getSession(uri_ssl);
// EXPECT_TRUE(s.isOpen());
// s.close();
//
// // Connect once more without SSL - should (SUCCESS)
// var s = mysqlx.getSession(uri_nossl);
// EXPECT_TRUE(s.isOpen());
// s.close();

//@ shell connect classic -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should succeed b/c blank password
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect again with SSL - should (SUCCESS)
var s = shell.connect(uri_ssl);
EXPECT_TRUE(s.isOpen());
s.close();

// Connect once more without SSL - should (SUCCESS)
var s = shell.connect(uri_nossl);
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect x -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// TODO() xplugin bug
// Connect again with SSL - should (SUCCESS)
// var s = shell.connect(uri_ssl);
// EXPECT_TRUE(s.isOpen());
// s.close();
//
// // Connect once more without SSL - should (SUCCESS)
// var s = shell.connect(uri_nossl);
// EXPECT_TRUE(s.isOpen());
// s.close();

//@ shell classic -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should succeed b/c blank password
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect again with SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_ssl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

// Connect once more without SSL - should (SUCCESS)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ shell x -- user:remo_blank / password: (SUCCESS)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// TODO xplugin bug
// // Connect again with SSL - should (SUCCESS)
// var rc = testutil.callMysqlsh([uri_ssl, '--password=', '-e', 'println(session)']);
// EXPECT_EQ(0, rc);
//
// // Connect once more without SSL - should (SUCCESS)
// var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
// EXPECT_EQ(0, rc);



// ==== user:local_blank / password:pass (FAIL)
//@ session classic -- user:local_blank / password:pass  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_ssl)}, "Access denied for user 'local_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ session x -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'local_blank'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_ssl)}, "Access denied for user 'local_blank'@'localhost' (using password: YES)");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'local_blank'@'localhost' (using password: YES)");

//@ shell connect classic -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_ssl)}, "Access denied for user 'local_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ shell connect x -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_blank'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_ssl)}, "Access denied for user 'local_blank'@'localhost' (using password: YES)");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_blank'@'localhost' (using password: YES)");

//@ shell classic -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_ssl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ shell x -- user:local_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_blank'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_ssl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_blank'@'localhost' (using password: YES)");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_blank'@'localhost' (using password: YES)");

// ==== user:remo_blank / password:pass (FAIL)
//@ session classic -- user:remo_blank / password:pass  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_ssl)}, "Access denied for user 'remo_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ session x -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_ssl)}, "Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'remo_blank'@'localhost' (using password: YES)");

//@ shell connect classic -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_ssl)}, "Access denied for user 'remo_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ shell connect x -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_ssl)}, "Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_blank'@'localhost' (using password: YES)");

//@ shell classic -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

// Connect again with SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_ssl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_blank'@'localhost'");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Authentication plugin 'caching_sha2_password' reported error: Authentication requires secure connection.");

//@ shell x -- user:remo_blank / password:pass (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_ssl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=pass', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_blank'@'localhost' (using password: YES)");

// ==== user:local_pass / password: (FAIL)
//@ session classic -- user:local_pass / password:  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Access denied for user 'local_pass'@'localhost'");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_ssl)}, "Access denied for user 'local_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Access denied for user 'local_pass'@'localhost'");

//@ session x -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'local_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_ssl)}, "Access denied for user 'local_pass'@'localhost' (using password: NO)");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'local_pass'@'localhost' (using password: YES)");

//@ shell connect classic -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_pass'@'localhost'");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_ssl)}, "Access denied for user 'local_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_pass'@'localhost'");

//@ shell connect x -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_ssl)}, "Access denied for user 'local_pass'@'localhost' (using password: NO)");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'local_pass'@'localhost' (using password: YES)");

//@ shell classic -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost' ");

// Connect again with SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_ssl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost' ");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost'");

//@ shell x -- user:local_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_ssl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost' (using password: YES)");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost' (using password: YES)");

// ==== user:remo_pass / password: (FAIL)
//@ session classic -- user:remo_pass / password:  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' ");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_ssl)}, "Access denied for user 'remo_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' ");

//@ session x -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_ssl)}, "Access denied for user 'remo_pass'@'localhost' (using password: NO)");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' (using password: YES)");

//@ shell connect classic -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost'");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_ssl)}, "Access denied for user 'remo_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost'");

//@ shell connect x -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_ssl)}, "Access denied for user 'remo_pass'@'localhost' (using password: NO)");

// Connect once more without SSL - should (FAIL)
EXPECT_THROWS(function() { shell.connect(uri_nossl)}, "Access denied for user 'remo_pass'@'localhost' (using password: YES)");

//@ shell classic -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED';
var uri_ssl = 'mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost'");

// Connect again with SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_ssl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost'");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost'");

//@ shell x -- user:remo_pass / password: (FAIL)  {VER(>=8.0.4)}
rootsess.runSql('flush privileges');
var uri_nossl = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED';
var uri_ssl = 'mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED';

// First connect without SSL - should fail
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost' (using password: YES)");

// Connect again with SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_ssl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost' (using password: YES)");

// Connect once more without SSL - should (FAIL)
var rc = testutil.callMysqlsh([uri_nossl, '--password=', '-e', 'println(session)']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost' (using password: YES)");

//@ GlobalTearDown {VER(>=8.0.4)}
rootsess.close();
testutil.destroySandbox(__mysql_sandbox_port1);
