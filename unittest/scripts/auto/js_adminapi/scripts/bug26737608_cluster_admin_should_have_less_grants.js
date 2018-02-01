// deploy sandbox
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.snapshotSandboxConf(__mysql_sandbox_port1);

shell.connect({scheme:'mysql', user:'root', password: 'root', host:'localhost', port:__mysql_sandbox_port1});

//@ create cluster admin
dba.configureLocalInstance("root:root@localhost:" + __mysql_sandbox_port1, {clusterAdmin: "ca", clusterAdminPassword: "ca", mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});

//@<OUT> check global privileges of cluster admin
session.runSql("SELECT PRIVILEGE_TYPE, IS_GRANTABLE FROM INFORMATION_SCHEMA.USER_PRIVILEGES WHERE GRANTEE = \"'ca'@'%'\" ORDER BY PRIVILEGE_TYPE");

//@<OUT> check schema privileges of cluster admin
session.runSql("SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES WHERE GRANTEE = \"'ca'@'%'\" ORDER BY TABLE_SCHEMA, PRIVILEGE_TYPE");

//@<OUT> check table privileges of cluster admin
session.runSql("SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES WHERE GRANTEE = \"'ca'@'%'\" ORDER BY TABLE_SCHEMA, TABLE_NAME, PRIVILEGE_TYPE");

//@ cluster admin should be able to create another cluster admin
dba.configureLocalInstance("ca:ca@localhost:" + __mysql_sandbox_port1, {clusterAdmin: "ca2", clusterAdminPassword: "ca2", mycnfPath: testutil.getSandboxConfPath(__mysql_sandbox_port1)});

// Smart deployment cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
