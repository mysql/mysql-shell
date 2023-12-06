// Assumptions: smart deployment routines available

shell.options.useWizards=false;
var __sandbox_path = testutil.getSandboxPath();

//@<> Deploy sandbox in dir with space
var test_dir = os.path.join(__sandbox_path, "foo \' bar");

testutil.mkdir(test_dir);

EXPECT_NO_THROWS(function() { dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir, password: 'root'}); });

//@<> BUG#29634828 AdminAPI should handle localhost and sandboxes better
// sandboxes have report_host variable set to "127.0.0.1"
shell.connect(__sandbox_uri1);
EXPECT_EQ("127.0.0.1", session.runSql("SELECT @@report_host").fetchOne()[0]);

//@<> Stop sandbox in dir with space
EXPECT_NO_THROWS(function() { dba.stopSandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir, password: 'root'}); });

//@<> BUG@31113914 SHELL TRIES TO RECONNECT WHEN CALLING QUIT COMMAND AFTER CALLING A SHELL EXT OBJ
EXPECT_EQ(false, session.isOpen(), "Session has not been closed");

//@<> Delete sandbox in dir with space
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.rmdir(test_dir, true);

// Regression for BUG25485035:
// Deployment using paths longer than 89 chars should succeed.
//@<> Deploy sandbox in dir with long path
var test_dir_long = os.path.join(__sandbox_path, "012345678911234567892123456789312345678941234567895123456789612345678971234567898123456789");
testutil.mkdir(test_dir_long);

EXPECT_NO_THROWS(function() { dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir_long, password: 'root'}); });

//@<> Stop sandbox in dir with long path
EXPECT_NO_THROWS(function() { dba.stopSandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir_long, password: 'root'}); });

//@<> Delete sandbox in dir with long path
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.rmdir(test_dir_long, true);

// BUG27181177: dba commands fail if user name has non-standard characters
//@<> Deploy sandbox in dir with non-ascii characters.
dba.verbose = 2;
var test_dir_non_ascii = os.path.join(__sandbox_path, "no_café_para_los_niños");
testutil.mkdir(test_dir_non_ascii);

EXPECT_NO_THROWS(function() { dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir_non_ascii, password: 'root'}); });

//@<> Stop sandbox in dir with non-ascii characters.
EXPECT_NO_THROWS(function() { dba.stopSandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir_non_ascii, password: 'root'}); });

//@<> Delete sandbox in dir with non-ascii characters.
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> SETUP BUG@29725222 add restart support to sandboxes {VER(>= 8.0.17)}
EXPECT_NO_THROWS(function() { testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname}); });
session = shell.connect(__sandbox_uri1);

//@<> BUG@29725222 test that restart works {VER(>= 8.0.17)}
session.runSql("RESTART");
testutil.waitSandboxAlive(__mysql_sandbox_port1);
session = shell.connect(__sandbox_uri1);
EXPECT_OUTPUT_CONTAINS(`Query OK, 0 rows affected`);

//@<> TEARDOWN BUG@29725222 add restart support to sandboxes {VER(>= 8.0.17)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Delete a non existing sandbox must throw an error BUG#30863587
var sandbox_path_2 = os.path.join(test_dir_non_ascii, __mysql_sandbox_port2.toString());
EXPECT_THROWS(function() { dba.deleteSandboxInstance(__mysql_sandbox_port2, {sandboxDir: test_dir_non_ascii});}, `Sandbox instance on '${sandbox_path_2}' does not exist.`);

testutil.rmdir(test_dir_non_ascii, true);

//@<> testutil.destroySandbox must not throw any error if deleting a non existing sandbox BUG#30863587
testutil.destroySandbox(__mysql_sandbox_port2);
