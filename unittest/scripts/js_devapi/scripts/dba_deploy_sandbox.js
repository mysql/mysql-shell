// Assumptions: smart deployment routines available
//@ Initialization
var connected = connect_to_sandbox([__mysql_sandbox_port1]);
if (connected)
    cleanup_sandbox(__mysql_sandbox_port1);

//@<OUT> Deploy sandbox
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir:__sandbox_dir});

//@ BUG#28624006: Deploy sandbox with non-existing dir
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: __sandbox_dir + "invalid"});

//@ BUG#28624006: Deploy sandbox with file instead of directory
var file = __sandbox_dir + "/file";
testutil.touch(file);
dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: file});
testutil.rmfile(file);

//@ Finalization
if (!connected)
    // if sandbox was not available when test start, clean it
    cleanup_sandbox(__mysql_sandbox_port1);

//@<> BUG#27369121: allowRootFrom null
EXPECT_THROWS_TYPE(function () {
    dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom: null, sandboxDir: __sandbox_dir});
}, "Option 'allowRootFrom' is expected to be of type String, but is Null", "TypeError");

//@<> BUG#27369121: allowRootFrom empty string does not create remote root account
const select_root_accounts = "select host, user from mysql.user where user like 'root' order by host;";
dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom: '', password: 'root', sandboxDir: __sandbox_dir});
shell.connect(__sandbox_uri1);
var result = session.runSql(select_root_accounts).fetchAll();
EXPECT_EQ(repr(["localhost", "root"]), repr(result[0]));
EXPECT_EQ(1, result.length);
shell.disconnect();
cleanup_sandbox(__mysql_sandbox_port1);

//@<> BUG#27369121: allowRootFrom=% string create remote root account
dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom: '%', password: 'root', sandboxDir: __sandbox_dir});
shell.connect(__sandbox_uri1);
var result = session.runSql(select_root_accounts).fetchAll();
EXPECT_EQ(repr(["%", "root"]), repr(result[0]));
EXPECT_EQ(repr(["localhost", "root"]), repr(result[1]));
EXPECT_EQ(2, result.length);
shell.disconnect();
cleanup_sandbox(__mysql_sandbox_port1);

//@<> BUG#27369121: allowRootFrom=qqqq string create root@qqqq account
dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom: 'qqqq', password: 'root', sandboxDir: __sandbox_dir});
shell.connect(__sandbox_uri1);
var result = session.runSql(select_root_accounts).fetchAll();
EXPECT_EQ(repr(["localhost", "root"]), repr(result[0]));
EXPECT_EQ(repr(["qqqq", "root"]), repr(result[1]));
EXPECT_EQ(2, result.length);
shell.disconnect();
cleanup_sandbox(__mysql_sandbox_port1);
