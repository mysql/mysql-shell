//

//@ init
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);

session.runSql("create schema testdb");
session.runSql("create table testdb.table1 (a int primary key, b varchar(30))");
session.runSql("insert into testdb.table1 values (1, 'bla'), (2, 'ble')");
session.runSql("create table testdb.table2 (a int primary key, b varchar(30))");
session.runSql("insert into testdb.table2 values (10, 'foo')");

session.runSql("create schema testdb2");
session.runSql("create table testdb2.table1 (a int primary key, b varchar(30))");
session.runSql("insert into testdb2.table1 values (1, 'bla'), (2, 'ble')");
session.runSql("create table testdb2.table2 (a int primary key, b varchar(30))");
session.runSql("insert into testdb2.table2 values (10, 'foo')");

//@<> dumpData
testutil.dumpData(__sandbox_uri1, "dump.sql", ["testdb", "testdb2"]);

//@<> importData
session.runSql("drop schema testdb");
session.runSql("drop schema testdb2");
testutil.importData(__sandbox_uri1, "dump.sql");

//@<OUT> importData check
session.runSql("select * from testdb.table1").fetchAll();
session.runSql("select * from testdb2.table2").fetchAll();

//@! importData badfile
testutil.importData(__sandbox_uri1, "badfile.sql");

//@! importData badpass
testutil.importData("root:bla@localhost:"+__mysql_sandbox_port1, "dump.sql");

//@ cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
