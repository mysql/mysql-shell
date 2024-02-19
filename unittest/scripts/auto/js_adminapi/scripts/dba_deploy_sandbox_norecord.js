// Assumptions: smart deployment routines available
//@<> Initialization
shell.options.useWizards=true;
var __sandbox_path = testutil.getSandboxPath();
var __sandbox_path_1 = os.path.join(__sandbox_path, __mysql_sandbox_port1.toString());

//@<> Deploy sandbox
testutil.expectPassword("*", "root");

EXPECT_NO_THROWS(function() { dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: __sandbox_path, mysqldOptions: ["require_secure_transport=OFF"]});});

EXPECT_SHELL_LOG_CONTAINS("Sandbox instances are only suitable for deploying and running on your local machine for testing purposes and are not accessible from external networks.")

EXPECT_OUTPUT_CONTAINS(`A new MySQL sandbox instance will be created on this host in 
${__sandbox_path_1}`);
EXPECT_OUTPUT_CONTAINS(`
Warning: Sandbox instances are only suitable for deploying and 
running on your local machine for testing purposes and are not 
accessible from external networks.`);
EXPECT_OUTPUT_CONTAINS(`Deploying new MySQL instance...`);
EXPECT_OUTPUT_CONTAINS(`Instance localhost:${__mysql_sandbox_port1} successfully deployed and started.`);
EXPECT_OUTPUT_CONTAINS(`Use shell.connect('root@localhost:${__mysql_sandbox_port1}') to connect to the instance.`);

//@<> BUG#28624006: Deploy sandbox with non-existing dir
var __sandbox_path_invalid = os.path.join(__sandbox_path, "invalid");

EXPECT_THROWS(function() { dba.deploySandboxInstance(__mysql_sandbox_port1,{sandboxDir: __sandbox_path_invalid, mysqldOptions: ["require_secure_transport=OFF"]});}, `The sandbox dir path '${__sandbox_path_invalid}' is not valid: it does not exist.`);

//@<> BUG#28624006: Deploy sandbox with file instead of directory
var __file_path = __sandbox_path + "/file";
testutil.touch(__file_path);

EXPECT_THROWS(function() { dba.deploySandboxInstance(__mysql_sandbox_port1, {sandboxDir: __file_path, mysqldOptions: ["require_secure_transport=OFF"]});}, `The sandbox dir path '${__file_path}' is not valid: it is not a directory.`);

testutil.rmfile(__file_path);

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> BUG#27369121: allowRootFrom null
EXPECT_THROWS_TYPE(function () {
    dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom: null, sandboxDir: __sandbox_path, mysqldOptions: ["require_secure_transport=OFF"]});
}, "Option 'allowRootFrom' is expected to be of type String, but is Null", "TypeError");

//@<> BUG#27369121: allowRootFrom empty string does not create remote root account
const select_root_accounts = "select host, user from mysql.user where user like 'root' order by host;";
dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom: '', password: 'root', sandboxDir: __sandbox_path, mysqldOptions: ["require_secure_transport=OFF"]});
shell.connect(__sandbox_uri1);
var result = session.runSql(select_root_accounts).fetchAll();
EXPECT_EQ(repr(["localhost", "root"]), repr(result[0]));
EXPECT_EQ(1, result.length);
shell.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> BUG#27369121: allowRootFrom=% string create remote root account
dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom: '%', password: 'root', sandboxDir: __sandbox_path, mysqldOptions: ["require_secure_transport=OFF"]});
shell.connect(__sandbox_uri1);
var result = session.runSql(select_root_accounts).fetchAll();
EXPECT_EQ(repr(["%", "root"]), repr(result[0]));
EXPECT_EQ(repr(["localhost", "root"]), repr(result[1]));
EXPECT_EQ(2, result.length);
shell.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> BUG#27369121: allowRootFrom=qqqq string create root@qqqq account
dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom: 'qqqq', password: 'root', sandboxDir: __sandbox_path, mysqldOptions: ["require_secure_transport=OFF"]});
shell.connect(__sandbox_uri1);
var result = session.runSql(select_root_accounts).fetchAll();
EXPECT_EQ(repr(["localhost", "root"]), repr(result[0]));
EXPECT_EQ(repr(["qqqq", "root"]), repr(result[1]));
EXPECT_EQ(2, result.length);
shell.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
