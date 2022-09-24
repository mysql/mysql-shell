//@{__azure_configured && !__azure_emulator} // The Azure Emulator (Azurite) does not support SAS Tokens
//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
shell.connect(__sandbox_uri1);
session.runSql("set global local_infile=1");
session.runSql("create schema world");
testutil.importData(__sandbox_uri1, __data_path+"/sql/fieldtypes_all.sql");
env_vars = ["MYSQLSH_TERM_COLOR_MODE=nocolor", "AZURE_STORAGE_CONNECTION_STRING="];


//@<> Testing Dump And Load
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "dump-instance", "instanceDmp", "--azureContainerName", __container_name, "--azureStorageAccount", __azure_account, "--azureStorageSasToken", `${__azure_account_rwl_sas_token}`], "", env_vars);
session.runSql("drop schema xtest");
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "load-dump", "instanceDmp", "--azureContainerName", __container_name, "--azureStorageAccount", __azure_account, "--azureStorageSasToken", `${__azure_account_rl_sas_token}`, "--progressFile=load_dump_progress.txt"], "", env_vars);
res = session.runSql("show schemas like 'xtest'").fetchOne();
EXPECT_EQ('xtest', res[0]);


//@<> Testing Dump Schemas And Load
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "dump-schemas", "xtest", "--outputUrl=schemasDump", "--azureContainerName", __container_name, "--azureStorageAccount", __azure_account, "--azureStorageSasToken", `${__azure_account_rwl_sas_token}`], "", env_vars);
session.runSql("drop schema xtest");
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "load-dump", "schemasDump", "--azureContainerName", __container_name, "--azureStorageAccount", __azure_account, "--azureStorageSasToken", `${__azure_account_rl_sas_token}`, "--progressFile=load_schemas_progress.txt"], "", env_vars);
res = session.runSql("show schemas like 'xtest'").fetchOne();
EXPECT_EQ('xtest', res[0]);

//@<> Testing Dump Tables And Load
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "dump-tables", "xtest", "t_tinyint,t_smallint", "--outputUrl=tablesDump", "--azureContainerName", __container_name, "--azureStorageAccount", __azure_account, "--azureStorageSasToken", `${__azure_account_rwl_sas_token}`], "", env_vars);
session.runSql("drop table xtest.t_tinyint");
session.runSql("drop table xtest.t_smallint");
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "load-dump", "tablesDump", "--azureContainerName", __container_name, "--azureStorageAccount", __azure_account, "--azureStorageSasToken", `${__azure_account_rl_sas_token}`, "--progressFile=load_tables_progress.txt"], "", env_vars);
res = session.runSql("use xtest").fetchOne();
res = session.runSql("show tables like 't_tinyint'").fetchOne();
EXPECT_EQ('t_tinyint', res[0]);
res = session.runSql("show tables like 't_smallint'").fetchOne();
EXPECT_EQ('t_smallint', res[0]);


//@<> Testing Export/Import Table
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "export-table", "xtest.t_tinyint", "t_tinyint.data", "--azureContainerName", __container_name], "", env_vars);
session.runSql("drop table xtest.t_tinyint");
session.runSql("use xtest");
session.runSql("CREATE TABLE t_tinyint (c1  TINYINT, c2 TINYINT UNSIGNED)");
testutil.callMysqlsh([__sandbox_uri1 + "/xtest", "--", "util", "import-table", "t_tinyint.data", "--azureContainerName", __container_name, "--azureStorageSasToken", `${__azure_account_rl_sas_token}`], "", env_vars);
res = session.runSql("show tables like 't_tinyint'").fetchOne();
EXPECT_EQ('t_tinyint', res[0]);

//@<> Cleanup
testutil.rmfile("load_dump_progress.txt");
testutil.rmfile("load_schemas_progress.txt");
testutil.rmfile("load_tables_progress.txt");

testutil.destroySandbox(__mysql_sandbox_port1);

