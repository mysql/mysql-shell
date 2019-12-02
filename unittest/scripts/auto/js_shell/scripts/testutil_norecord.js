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

//@<> setTrap {!__dbug_off && !__recording && !__replaying}

EXPECT_NO_THROWS(function(){session.runSql("select 1").fetchOne()});

// throw error when a given query pattern is found
testutil.setTrap("mysql", ["sql regex select [0-9]"], {code: 3232, msg: "uh oh"});

EXPECT_THROWS(function(){session.runSql("select 2").fetchOne()}, "uh oh");
EXPECT_THROWS(function(){session.runSql("select 9").fetchOne()}, "uh oh");

testutil.clearTraps("mysql");

EXPECT_NO_THROWS(function(){session.runSql("select 2").fetchOne()});

// same but only once
testutil.setTrap("mysql", ["sql regex select [0-9]"], {code: 3232, msg: "bla", onetime:1});

EXPECT_THROWS(function(){session.runSql("select 2").fetchOne()}, "bla");
EXPECT_NO_THROWS(function(){session.runSql("select 2").fetchOne()});

// throw error only on connections to the sandbox
session2 = mysql.getSession(__mysqluripwd);

testutil.setTrap("mysql", ["uri regex .*:"+__mysql_sandbox_port1], {code:3232, msg: "ble", onetime:0});

EXPECT_THROWS(function(){session.runSql("show schemas").fetchOne()}, "ble");

EXPECT_NO_THROWS(function(){session2.runSql("show schemas").fetchOne()});

EXPECT_THROWS(function(){session.runSql("show schemas").fetchOne()}, "ble");

// throw error after 2 stmts (after resetting counter states)
testutil.clearTraps();
testutil.setTrap("mysql", ["++match_counter > 2"], {code:3333, msg:"too many selects"});

session.runSql("select 1");
session.runSql("select 2");
EXPECT_THROWS(function(){session.runSql("select * from mysql.host")}, "too many selects");

// throw error after 3 stmts (in mysqlshrec)
testutil.callMysqlsh(["mysql://root:root@localhost:"+__mysql_sandbox_port1, "--debug=[{'on':'mysql','if':['++match_counter == 3'],'code':1234,'msg':'injected error'}]", "--sql", "-e", "select 5; select 7; select 9;"], "", [], "mysqlshrec");
EXPECT_STDOUT_CONTAINS("ERROR: 1234 at line 1: injected error");

// crash after 3 stmts (in mysqlshrec)
r = testutil.callMysqlsh(["mysql://root:root@localhost:"+__mysql_sandbox_port1, "--debug={'on':'mysql','if':['++match_counter == 3'],'abort':1}", "--sql", "-e", "select 5; select 7; select 9;"], "", [], "mysqlshrec");
EXPECT_NE(0, r);

// throw error when a specific stmt is executed twice
testutil.clearTraps();
testutil.setTrap("mysql", ["sql == select 42", "++match_counter > 1"], {code:3333, msg:"trap hit"});
session.runSql("select 42");
session.runSql("select 4");
session.runSql("select 412");
EXPECT_THROWS(function(){session.runSql("select 42")}, "trap hit");

// clear all traps before leaving
testutil.clearTraps();

//@ cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
