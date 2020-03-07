// Assumptions: smart deployment routines available

//@ Deploy sandbox in dir with space
var test_dir = __sandbox_dir + __path_splitter + "foo \' bar";
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir, password: 'root'});

//@ BUG#29634828 AdminAPI should handle localhost and sandboxes better
// sandboxes have report_host variable set to "127.0.0.1"
shell.connect(__sandbox_uri1);
session.runSql("SELECT @@report_host")
session.close();

//@ Stop sandbox in dir with space
dba.stopSandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir, password: 'root'});

//@ Delete sandbox in dir with space
try_delete_sandbox(__mysql_sandbox_port1, test_dir);

// Regression for BUG25485035:
// Deployment using paths longer than 89 chars should succeed.
//@ Deploy sandbox in dir with long path
var test_dir = __sandbox_dir + __path_splitter + "012345678911234567892123456789312345678941234567895123456789612345678971234567898123456789";
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir, password: 'root'});

//@ Stop sandbox in dir with long path
dba.stopSandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir, password: 'root'});

//@ Delete sandbox in dir with long path
try_delete_sandbox(__mysql_sandbox_port1, test_dir);

// BUG27181177: dba commands fail if user name has non-standard characters
//@ Deploy sandbox in dir with non-ascii characters.
dba.verbose = 2;
var test_dir = __sandbox_dir + "no_café_para_los_niños";
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir, password: 'root'});

//@ Stop sandbox in dir with non-ascii characters.
dba.stopSandboxInstance(__mysql_sandbox_port1, {sandboxDir: test_dir, password: 'root'});

//@ Delete sandbox in dir with non-ascii characters.
try_delete_sandbox(__mysql_sandbox_port1, test_dir);

//@ SETUP BUG@29725222 add restart support to sandboxes {VER(>= 8.0.17)}
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
session = shell.connect(__sandbox_uri1);

//@ BUG@29725222 test that restart works {VER(>= 8.0.17)}
session.runSql("RESTART");
testutil.waitSandboxAlive(__mysql_sandbox_port1);
session = shell.connect(__sandbox_uri1);

//@ TEARDOWN BUG@29725222 add restart support to sandboxes {VER(>= 8.0.17)}
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@ Delete a non existing sandbox must throw an error BUG#30863587
dba.deleteSandboxInstance(__mysql_sandbox_port2, {sandboxDir: test_dir});

//@ testutil.destroySandbox must not throw any error if deleting a non existing sandbox BUG#30863587
testutil.destroySandbox(__mysql_sandbox_port2);
