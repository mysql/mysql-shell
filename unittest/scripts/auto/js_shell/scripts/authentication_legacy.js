// Test authentication using mysql_native_password

// Entry points:
//  mysqlsh executable
//  shell.connect()
//  mysql.getClassicSession() and mysqlx.getSession()

// Variations:
//  classic and x proto
//  @localhost and @%
//  with password and without
//  with ssl and without

// Test cases:
//  with correct password and without
//  with correct user and without

//@ GlobalSetUp
testutil.deploySandbox(__mysql_sandbox_port1, "root");
var rootsess = mysql.getClassicSession(__sandbox_uri1);

rootsess.runSql("CREATE USER local_blank@localhost IDENTIFIED WITH mysql_native_password BY ''");
rootsess.runSql("CREATE USER local_pass@localhost IDENTIFIED WITH mysql_native_password BY 'pass'");
rootsess.runSql("CREATE USER remo_blank@'%' IDENTIFIED WITH mysql_native_password BY ''");
rootsess.runSql("CREATE USER remo_pass@'%' IDENTIFIED WITH mysql_native_password BY 'pass'");

// error returned for invalid password in 5.7 over xproto is different from 8.0
if (testutil.versionCheck(__version, ">=", "8.0.4"))
  var auth_fail_exc = "Invalid authentication method PLAIN";
else
  var auth_fail_exc = "Invalid user or password";

// Try all combinations using mysql_native_password
// ================================================
// These are for the legacy/old/traditional authentication plugin, which has no special
// considerations regarding ssl

// ==== user:local_blank / password:pass / ssl:DISABLED (FAIL)
rootsess.runSql('flush privileges');
//@ session classic -- user:local_blank / password:pass / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession('local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED')}, "Access denied for user 'local_blank'@'localhost'");

//@ session x -- user:local_blank / password:pass / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession('local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED')}, auth_fail_exc);

//@ shell connect classic -- user:local_blank / password:pass / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED')}, "Access denied for user 'local_blank'@'localhost'");

//@ shell connect x -- user:local_blank / password:pass / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED')}, auth_fail_exc);

//@ shell classic -- user:local_blank / password:pass / ssl:DISABLED (FAIL)
var rc = testutil.callMysqlsh(['mysql://local_blank@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED', '--password=pass', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_blank'@'localhost'");

//@ shell x -- user:local_blank / password:pass / ssl:DISABLED (FAIL)
var rc = testutil.callMysqlsh(['mysqlx://local_blank@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED', '--password=pass', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS(auth_fail_exc);

// ==== user:local_pass / password:pass / ssl:DISABLED (SUCCESS)
rootsess.runSql('flush privileges');
//@ session classic -- user:local_pass / password:pass / ssl:DISABLED (SUCCESS)
var s = mysql.getClassicSession('local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:local_pass / password:pass / ssl:DISABLED (SUCCESS)
var s = mysqlx.getSession('local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:local_pass / password:pass / ssl:DISABLED (SUCCESS)
shell.connect('mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell connect x -- user:local_pass / password:pass / ssl:DISABLED (SUCCESS)
shell.connect('mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell classic -- user:local_pass / password:pass / ssl:DISABLED (SUCCESS)
var rc = testutil.callMysqlsh(['mysql://local_pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED', '--password=pass', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

//@ shell x -- user:local_pass / password:pass / ssl:DISABLED (SUCCESS)
var rc = testutil.callMysqlsh(['mysqlx://local_pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED', '--password=pass', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

// ==== user:remo_blank / password:pass / ssl:DISABLED (FAIL)
rootsess.runSql('flush privileges');
//@ session classic -- user:remo_blank / password:pass / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession('remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED')}, "Access denied for user 'remo_blank'@'localhost'");

//@ session x -- user:remo_blank / password:pass / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession('remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED')}, auth_fail_exc);

//@ shell connect classic -- user:remo_blank / password:pass / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED')}, "Access denied for user 'remo_blank'@'localhost'");

//@ shell connect x -- user:remo_blank / password:pass / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED')}, auth_fail_exc);

//@ shell classic -- user:remo_blank / password:pass / ssl:DISABLED (FAIL)
var rc = testutil.callMysqlsh(['mysql://remo_blank@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED', '--password=pass', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_blank'@'localhost'");

//@ shell x -- user:remo_blank / password:pass / ssl:DISABLED (FAIL)
var rc = testutil.callMysqlsh(['mysqlx://remo_blank@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED', '--password=pass', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS(auth_fail_exc);

// ==== user:remo_pass / password:pass / ssl:DISABLED (SUCCESS)
rootsess.runSql('flush privileges');
//@ session classic -- user:remo_pass / password:pass / ssl:DISABLED (SUCCESS)
var s = mysql.getClassicSession('remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:remo_pass / password:pass / ssl:DISABLED (SUCCESS)
var s = mysqlx.getSession('remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:remo_pass / password:pass / ssl:DISABLED (SUCCESS)
shell.connect('mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell connect x -- user:remo_pass / password:pass / ssl:DISABLED (SUCCESS)
shell.connect('mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell classic -- user:remo_pass / password:pass / ssl:DISABLED (SUCCESS)
var rc = testutil.callMysqlsh(['mysql://remo_pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED', '--password=pass', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

//@ shell x -- user:remo_pass / password:pass / ssl:DISABLED (SUCCESS)
var rc = testutil.callMysqlsh(['mysqlx://remo_pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED', '--password=pass', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

// ==== user:local_blank / password: / ssl:DISABLED (SUCCESS)
rootsess.runSql('flush privileges');
//@ session classic -- user:local_blank / password: / ssl:DISABLED (SUCCESS)
var s = mysql.getClassicSession('local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:local_blank / password: / ssl:DISABLED (SUCCESS)
var s = mysqlx.getSession('local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:local_blank / password: / ssl:DISABLED (SUCCESS)
shell.connect('mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell connect x -- user:local_blank / password: / ssl:DISABLED (SUCCESS)
shell.connect('mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell classic -- user:local_blank / password: / ssl:DISABLED (SUCCESS)
var rc = testutil.callMysqlsh(['mysql://local_blank@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED', '--password=', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

//@ shell x -- user:local_blank / password: / ssl:DISABLED (SUCCESS)
var rc = testutil.callMysqlsh(['mysqlx://local_blank@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED', '--password=', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

// ==== user:local_pass / password: / ssl:DISABLED (FAIL)
rootsess.runSql('flush privileges');
//@ session classic -- user:local_pass / password: / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession('local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED')}, "Access denied for user 'local_pass'@'localhost'");

//@ session x -- user:local_pass / password: / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession('local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED')}, auth_fail_exc);

//@ shell connect classic -- user:local_pass / password: / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED')}, "Access denied for user 'local_pass'@'localhost'");

//@ shell connect x -- user:local_pass / password: / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED')}, auth_fail_exc);

//@ shell classic -- user:local_pass / password: / ssl:DISABLED (FAIL)
var rc = testutil.callMysqlsh(['mysql://local_pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED', '--password=', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost'");

//@ shell x -- user:local_pass / password: / ssl:DISABLED (FAIL)
var rc = testutil.callMysqlsh(['mysqlx://local_pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED', '--password=', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS(auth_fail_exc);

// ==== user:remo_blank / password: / ssl:DISABLED (SUCCESS)
rootsess.runSql('flush privileges');
//@ session classic -- user:remo_blank / password: / ssl:DISABLED (SUCCESS)
var s = mysql.getClassicSession('remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:remo_blank / password: / ssl:DISABLED (SUCCESS)
var s = mysqlx.getSession('remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:remo_blank / password: / ssl:DISABLED (SUCCESS)
shell.connect('mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell connect x -- user:remo_blank / password: / ssl:DISABLED (SUCCESS)
shell.connect('mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell classic -- user:remo_blank / password: / ssl:DISABLED (SUCCESS)
var rc = testutil.callMysqlsh(['mysql://remo_blank@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED', '--password=', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

//@ shell x -- user:remo_blank / password: / ssl:DISABLED (SUCCESS)
var rc = testutil.callMysqlsh(['mysqlx://remo_blank@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED', '--password=', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

// ==== user:remo_pass / password: / ssl:DISABLED (FAIL)
rootsess.runSql('flush privileges');
//@ session classic -- user:remo_pass / password: / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession('remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED')}, "Access denied for user 'remo_pass'@'localhost'");

//@ session x -- user:remo_pass / password: / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession('remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED')}, auth_fail_exc);

//@ shell connect classic -- user:remo_pass / password: / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED')}, "Access denied for user 'remo_pass'@'localhost'");

//@ shell connect x -- user:remo_pass / password: / ssl:DISABLED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED')}, auth_fail_exc);

//@ shell classic -- user:remo_pass / password: / ssl:DISABLED (FAIL)
var rc = testutil.callMysqlsh(['mysql://remo_pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=DISABLED', '--password=', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost'");

//@ shell x -- user:remo_pass / password: / ssl:DISABLED (FAIL)
var rc = testutil.callMysqlsh(['mysqlx://remo_pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=DISABLED', '--password=', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS(auth_fail_exc);

// ==== user:local_blank / password:pass / ssl:REQUIRED (FAIL)
rootsess.runSql('flush privileges');
//@ session classic -- user:local_blank / password:pass / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession('local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED')}, "Access denied for user 'local_blank'@'localhost'");

//@ session x -- user:local_blank / password:pass / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession('local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED')}, "Invalid user or password");

//@ shell connect classic -- user:local_blank / password:pass / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysql://local_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED')}, "Access denied for user 'local_blank'@'localhost'");

//@ shell connect x -- user:local_blank / password:pass / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysqlx://local_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED')}, "Invalid user or password");

//@ shell classic -- user:local_blank / password:pass / ssl:REQUIRED (FAIL)
var rc = testutil.callMysqlsh(['mysql://local_blank@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED', '--password=pass', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_blank'@'localhost'");

//@ shell x -- user:local_blank / password:pass / ssl:REQUIRED (FAIL)
var rc = testutil.callMysqlsh(['mysqlx://local_blank@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED', '--password=pass', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid user or password");

// ==== user:local_pass / password:pass / ssl:REQUIRED (SUCCESS)
rootsess.runSql('flush privileges');
//@ session classic -- user:local_pass / password:pass / ssl:REQUIRED (SUCCESS)
var s = mysql.getClassicSession('local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:local_pass / password:pass / ssl:REQUIRED (SUCCESS)
var s = mysqlx.getSession('local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:local_pass / password:pass / ssl:REQUIRED (SUCCESS)
shell.connect('mysql://local_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell connect x -- user:local_pass / password:pass / ssl:REQUIRED (SUCCESS)
shell.connect('mysqlx://local_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell classic -- user:local_pass / password:pass / ssl:REQUIRED (SUCCESS)
var rc = testutil.callMysqlsh(['mysql://local_pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED', '--password=pass', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

//@ shell x -- user:local_pass / password:pass / ssl:REQUIRED (SUCCESS)
var rc = testutil.callMysqlsh(['mysqlx://local_pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED', '--password=pass', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

// ==== user:remo_blank / password:pass / ssl:REQUIRED (FAIL)
rootsess.runSql('flush privileges');
//@ session classic -- user:remo_blank / password:pass / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession('remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED')}, "Access denied for user 'remo_blank'@'localhost'");

//@ session x -- user:remo_blank / password:pass / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession('remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED')}, "Invalid user or password");

//@ shell connect classic -- user:remo_blank / password:pass / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysql://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED')}, "Access denied for user 'remo_blank'@'localhost'");

//@ shell connect x -- user:remo_blank / password:pass / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysqlx://remo_blank:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED')}, "Invalid user or password");

//@ shell classic -- user:remo_blank / password:pass / ssl:REQUIRED (FAIL)
var rc = testutil.callMysqlsh(['mysql://remo_blank@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED', '--password=pass', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_blank'@'localhost'");

//@ shell x -- user:remo_blank / password:pass / ssl:REQUIRED (FAIL)
var rc = testutil.callMysqlsh(['mysqlx://remo_blank@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED', '--password=pass', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid user or password");

// ==== user:remo_pass / password:pass / ssl:REQUIRED (SUCCESS)
rootsess.runSql('flush privileges');
//@ session classic -- user:remo_pass / password:pass / ssl:REQUIRED (SUCCESS)
var s = mysql.getClassicSession('remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:remo_pass / password:pass / ssl:REQUIRED (SUCCESS)
var s = mysqlx.getSession('remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:remo_pass / password:pass / ssl:REQUIRED (SUCCESS)
shell.connect('mysql://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell connect x -- user:remo_pass / password:pass / ssl:REQUIRED (SUCCESS)
shell.connect('mysqlx://remo_pass:pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell classic -- user:remo_pass / password:pass / ssl:REQUIRED (SUCCESS)
var rc = testutil.callMysqlsh(['mysql://remo_pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED', '--password=pass', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

//@ shell x -- user:remo_pass / password:pass / ssl:REQUIRED (SUCCESS)
var rc = testutil.callMysqlsh(['mysqlx://remo_pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED', '--password=pass', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

// ==== user:local_blank / password: / ssl:REQUIRED (SUCCESS)
rootsess.runSql('flush privileges');
//@ session classic -- user:local_blank / password: / ssl:REQUIRED (SUCCESS)
var s = mysql.getClassicSession('local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:local_blank / password: / ssl:REQUIRED (SUCCESS)
var s = mysqlx.getSession('local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:local_blank / password: / ssl:REQUIRED (SUCCESS)
shell.connect('mysql://local_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell connect x -- user:local_blank / password: / ssl:REQUIRED (SUCCESS)
shell.connect('mysqlx://local_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell classic -- user:local_blank / password: / ssl:REQUIRED (SUCCESS)
var rc = testutil.callMysqlsh(['mysql://local_blank@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED', '--password=', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

//@ shell x -- user:local_blank / password: / ssl:REQUIRED (SUCCESS)
var rc = testutil.callMysqlsh(['mysqlx://local_blank@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED', '--password=', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

// ==== user:local_pass / password: / ssl:REQUIRED (FAIL)
rootsess.runSql('flush privileges');
//@ session classic -- user:local_pass / password: / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession('local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED')}, "Access denied for user 'local_pass'@'localhost'");

//@ session x -- user:local_pass / password: / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession('local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED')}, "Invalid user or password");

//@ shell connect classic -- user:local_pass / password: / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysql://local_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED')}, "Access denied for user 'local_pass'@'localhost'");

//@ shell connect x -- user:local_pass / password: / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysqlx://local_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED')}, "Invalid user or password");

//@ shell classic -- user:local_pass / password: / ssl:REQUIRED (FAIL)
var rc = testutil.callMysqlsh(['mysql://local_pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED', '--password=', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'local_pass'@'localhost'");

//@ shell x -- user:local_pass / password: / ssl:REQUIRED (FAIL)
var rc = testutil.callMysqlsh(['mysqlx://local_pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED', '--password=', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid user or password");

// ==== user:remo_blank / password: / ssl:REQUIRED (SUCCESS)
rootsess.runSql('flush privileges');
//@ session classic -- user:remo_blank / password: / ssl:REQUIRED (SUCCESS)
var s = mysql.getClassicSession('remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ session x -- user:remo_blank / password: / ssl:REQUIRED (SUCCESS)
var s = mysqlx.getSession('remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED');
EXPECT_TRUE(s.isOpen());
s.close();

//@ shell connect classic -- user:remo_blank / password: / ssl:REQUIRED (SUCCESS)
shell.connect('mysql://remo_blank:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell connect x -- user:remo_blank / password: / ssl:REQUIRED (SUCCESS)
shell.connect('mysqlx://remo_blank:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED');
EXPECT_TRUE(session.isOpen());
session.close();

//@ shell classic -- user:remo_blank / password: / ssl:REQUIRED (SUCCESS)
var rc = testutil.callMysqlsh(['mysql://remo_blank@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED', '--password=', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

//@ shell x -- user:remo_blank / password: / ssl:REQUIRED (SUCCESS)
var rc = testutil.callMysqlsh(['mysqlx://remo_blank@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED', '--password=', '-e', 'shell.status()']);
EXPECT_EQ(0, rc);

// ==== user:remo_pass / password: / ssl:REQUIRED (FAIL)
rootsess.runSql('flush privileges');
//@ session classic -- user:remo_pass / password: / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { mysql.getClassicSession('remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED')}, "Access denied for user 'remo_pass'@'localhost'");

//@ session x -- user:remo_pass / password: / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { mysqlx.getSession('remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED')}, "Invalid user or password");

//@ shell connect classic -- user:remo_pass / password: / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysql://remo_pass:@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED')}, "Access denied for user 'remo_pass'@'localhost'");

//@ shell connect x -- user:remo_pass / password: / ssl:REQUIRED (FAIL)
EXPECT_THROWS(function() { shell.connect('mysqlx://remo_pass:@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED')}, "Invalid user or password");

//@ shell classic -- user:remo_pass / password: / ssl:REQUIRED (FAIL)
var rc = testutil.callMysqlsh(['mysql://remo_pass@localhost:'+__mysql_sandbox_port1+'/?ssl-mode=REQUIRED', '--password=', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Access denied for user 'remo_pass'@'localhost'");

//@ shell x -- user:remo_pass / password: / ssl:REQUIRED (FAIL)
var rc = testutil.callMysqlsh(['mysqlx://remo_pass@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode=REQUIRED', '--password=', '-e', 'shell.status()']);
EXPECT_NE(0, rc);
EXPECT_STDOUT_CONTAINS("Invalid user or password");

//@ GlobalTearDown
rootsess.close();
testutil.destroySandbox(__mysql_sandbox_port1);


// Generated with:
// var params = {
//   "user": ["local_blank", "local_pass", "remo_blank", "remo_pass"],
//   "pass": ["pass", ""],
//   "sslmode": ["DISABLED", "REQUIRED"]};
//
// expected_func = function(values) {
//     return (values["user"].indexOf("_pass") > 0 && values["pass"] == "pass") ||
//            (values["user"].indexOf("_blank") > 0 && values["pass"] == "");
// };

// // ==== user:{user} / password:{pass} / ssl:{sslmode} {{EXPECTED}}
// rootsess.runSql('flush privileges');
// //@ session classic -- user:{user} / password:{pass} / ssl:{sslmode} {{EXPECTED}}
// #if {{expected}}
// var s = mysql.getClassicSession('{user}:{pass}@localhost:'+__mysql_sandbox_port1+'/?ssl-mode={sslmode}');
// EXPECT_TRUE(s.isOpen());
// s.close();
// #else
// EXPECT_THROWS(function() { mysql.getClassicSession('{user}:{pass}@localhost:'+__mysql_sandbox_port1+'/?ssl-mode={sslmode}')}, "Access denied for user '{user}'@'localhost'");
// #endif
//
// //@ session x -- user:{user} / password:{pass} / ssl:{sslmode} {{EXPECTED}}
// #if {{expected}}
// var s = mysqlx.getSession('{user}:{pass}@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode={sslmode}');
// EXPECT_TRUE(s.isOpen());
// s.close();
// #else
// EXPECT_THROWS(function() { mysqlx.getSession('{user}:{pass}@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode={sslmode}')}, "Invalid user or password");
// #endif
//
// //@ shell connect classic -- user:{user} / password:{pass} / ssl:{sslmode} {{EXPECTED}}
// #if {{expected}}
// shell.connect('mysql://{user}:{pass}@localhost:'+__mysql_sandbox_port1+'/?ssl-mode={sslmode}');
// EXPECT_TRUE(session.isOpen());
// session.close();
// #else
// EXPECT_THROWS(function() { shell.connect('mysql://{user}:{pass}@localhost:'+__mysql_sandbox_port1+'/?ssl-mode={sslmode}')}, "Access denied for user '{user}'@'localhost'");
// #endif
//
// //@ shell connect x -- user:{user} / password:{pass} / ssl:{sslmode} {{EXPECTED}}
// #if {{expected}}
// shell.connect('mysqlx://{user}:{pass}@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode={sslmode}');
// EXPECT_TRUE(session.isOpen());
// session.close();
// #else
// EXPECT_THROWS(function() { shell.connect('mysqlx://{user}:{pass}@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode={sslmode}')}, "Invalid user or password");
// #endif
//
// //@ shell classic -- user:{user} / password:{pass} / ssl:{sslmode} {{EXPECTED}}
// var rc = testutil.callMysqlsh(['mysql://{user}@localhost:'+__mysql_sandbox_port1+'/?ssl-mode={sslmode}', '--password={pass}', '-e', 'shell.status()']);
// #if {{expected}}
// EXPECT_EQ(0, rc);
// #else
// EXPECT_NE(0, rc);
// EXPECT_STDOUT_CONTAINS("Access denied for user '{user}'@'localhost'");
// #endif
//
// //@ shell x -- user:{user} / password:{pass} / ssl:{sslmode} {{EXPECTED}}
// var rc = testutil.callMysqlsh(['mysqlx://{user}@localhost:'+__mysql_sandbox_port1+'0/?ssl-mode={sslmode}', '--password={pass}', '-e', 'shell.status()']);
// #if {{expected}}
// EXPECT_EQ(0, rc);
// #else
// EXPECT_NE(0, rc);
// EXPECT_STDOUT_CONTAINS("Invalid user or password");
// #endif
