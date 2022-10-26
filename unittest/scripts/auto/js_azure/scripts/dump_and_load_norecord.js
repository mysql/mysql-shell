//@{__azure_configured}

//@<> INCLUDE dump_utils.inc

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
shell.connect(__sandbox_uri1);
session.runSql("set global local_infile=1");
session.runSql("create schema world");
testutil.importData(__sandbox_uri1, __data_path+"/sql/fieldtypes_all.sql");


//@<> Testing Dump And Load
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "dump-instance", "instanceDmp", "--azureContainerName", __container_name], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
session.runSql("drop schema xtest");
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "load-dump", "instanceDmp", "--azureContainerName", __container_name], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
res = session.runSql("show schemas like 'xtest'").fetchOne();
EXPECT_EQ('xtest', res[0]);


//@<> Testing Dump Schemas And Load
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "dump-schemas", "xtest", "--outputUrl=schemasDump", "--azureContainerName", __container_name], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
session.runSql("drop schema xtest");
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "load-dump", "schemasDump", "--azureContainerName", __container_name], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
res = session.runSql("show schemas like 'xtest'").fetchOne();
EXPECT_EQ('xtest', res[0]);

//@<> Testing Dump Tables And Load
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "dump-tables", "xtest", "t_tinyint,t_smallint", "--outputUrl=tablesDump", "--azureContainerName", __container_name], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
session.runSql("drop table xtest.t_tinyint");
session.runSql("drop table xtest.t_smallint");
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "load-dump", "tablesDump", "--azureContainerName", __container_name], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
res = session.runSql("use xtest").fetchOne();
res = session.runSql("show tables like 't_tinyint'").fetchOne();
EXPECT_EQ('t_tinyint', res[0]);
res = session.runSql("show tables like 't_smallint'").fetchOne();
EXPECT_EQ('t_smallint', res[0]);


//@<> Testing Export/Import Table
testutil.callMysqlsh([__sandbox_uri1, "--", "util", "export-table", "xtest.t_tinyint", "t_tinyint.data", "--azureContainerName", __container_name], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
session.runSql("drop table xtest.t_tinyint");
session.runSql("use xtest");
session.runSql("CREATE TABLE t_tinyint (c1  TINYINT, c2 TINYINT UNSIGNED)");
testutil.callMysqlsh([__sandbox_uri1 + "/xtest", "--", "util", "import-table", "t_tinyint.data", "--azureContainerName", __container_name], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
res = session.runSql("show tables like 't_tinyint'").fetchOne();
EXPECT_EQ('t_tinyint', res[0]);

//@<> BUG#34657730 - util.exportTable() should output remote options needed to import this dump
// setup
tested_schema = "tested_schema";
tested_table = "tested_table";
dump_dir = "bug_34657730";

session.runSql("DROP SCHEMA IF EXISTS !;", [ tested_schema ]);
session.runSql("CREATE SCHEMA !;", [ tested_schema ]);
session.runSql("CREATE TABLE !.! (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, something BINARY)", [ tested_schema, tested_table ]);
session.runSql("INSERT INTO !.! (id) VALUES (302)", [ tested_schema, tested_table ]);
session.runSql("INSERT INTO !.! (something) VALUES (char(0))", [ tested_schema, tested_table ]);

expected_md5 = md5_table(session, tested_schema, tested_table);

// prepare the dump
WIPE_OUTPUT();
util.exportTable(quote_identifier(tested_schema, tested_table), dump_dir, { "azureContainerName": __container_name });

// capture the import command
full_stdout_output = testutil.fetchCapturedStdout(false);
index_of_util = full_stdout_output.indexOf("util.");
EXPECT_NE(-1, index_of_util);
util_import_table_code = full_stdout_output.substring(index_of_util);

//@<> BUG#34657730 - test
session.runSql("TRUNCATE TABLE !.!", [ tested_schema, tested_table ]);

EXPECT_NO_THROWS(() => eval(util_import_table_code), "importing data");
EXPECT_EQ(expected_md5, md5_table(session, tested_schema, tested_table));

//@<> BUG#34657730 - cleanup
session.runSql("DROP SCHEMA IF EXISTS !;", [ tested_schema ]);

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
