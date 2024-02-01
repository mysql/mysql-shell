//@<> Setup test
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);

// =================================
// Using 'unfiltered' logging option
// =================================

\option logSql = "unfiltered"
\option dba.logSql = 0

//@<> Testing unfiltered sql logging with \use
WIPE_SHELL_LOG();
\use mysql
EXPECT_SHELL_LOG_MATCHES(/Info: js: tid=\d+: SQL: use `mysql`/);

//@<> Testing unfiltered sql logging with \sql
WIPE_SHELL_LOG();
\sql show databases
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: show databases/);

//@<> Testing unfiltered sql logging with \show
WIPE_SHELL_LOG()
\show query SELECT 1
EXPECT_SHELL_LOG_MATCHES(/reports.query: tid=\d+: SQL: SELECT 1/)

//@<> Testing unfiltered sql logging from \status command
WIPE_SHELL_LOG();
\status
EXPECT_SHELL_LOG_MATCHES(/Info: js: tid=\d+: SQL: select DATABASE\(\), USER\(\) limit 1/);
EXPECT_SHELL_LOG_MATCHES(/Info: js: tid=\d+: SQL: show session status like 'ssl_version';/);


testutil.createFile("multi.sql", "create schema sql_logging; select 1; select 2; SELECT 3; select 4; select 5; select 6;\nselect 'a'; select 'b';\nselect 'c'; select 'd'; select 'e'; drop schema sql_logging;\n");

//@<> Testing unfiltered sql logging with \source (\.)
WIPE_SHELL_LOG();
\sql
\. multi.sql
\js

EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: create schema sql_logging/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 1/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 2/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: SELECT 3/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 4/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 5/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 6/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 'a'/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 'b'/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 'c'/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 'd'/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: select 'e'/);
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: drop schema sql_logging/);

//@<> Testing unfiltered sql logging from session object
WIPE_SHELL_LOG();
session.runSql("select 1");
EXPECT_SHELL_LOG_MATCHES(/Info: ClassicSession.runSql: tid=\d+: SQL: select 1/);

//@<> Testing unfiltered sql logging from dba object {VER(>=8.0.0)}
WIPE_SHELL_LOG();
try {
    cluster = dba.getCluster()
} catch (error) {
    print(error)
}
EXPECT_SHELL_LOG_MATCHES(/Info: Dba.getCluster: tid=\d+: CONNECTED: localhost:\d+/);
EXPECT_SHELL_LOG_MATCHES(/Info: Dba.getCluster: tid=\d+: SQL: SET SESSION `autocommit` = 1/);
EXPECT_SHELL_LOG_MATCHES(/Info: Dba.getCluster: tid=\d+: SQL: SHOW DATABASES LIKE 'mysql_innodb_cluster_metadata'/);


//@<> Testing unfiltered sql logging from shell object
WIPE_SHELL_LOG();
shell.status()
EXPECT_SHELL_LOG_MATCHES(/Info: Shell.status: tid=\d+: SQL: select DATABASE\(\), USER\(\) limit 1/);
EXPECT_SHELL_LOG_MATCHES(/Info: Shell.status: tid=\d+: SQL: show session status like 'ssl_version';/);



// =================================
// Using 'on' logging option
// =================================

\option logSql = "on"

//@<> Testing on sql logging with \use
WIPE_SHELL_LOG();
\use mysql
EXPECT_SHELL_LOG_MATCHES(/Info: js: tid=\d+: SQL: use `mysql`/);

//@<> Testing on sql logging with \sql
WIPE_SHELL_LOG();
\sql show databases
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: show databases");

//@<> Testing on sql logging with \show
WIPE_SHELL_LOG()
\show query SELECT 1
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: SELECT 1")

//@<> Testing on sql logging from \status command
WIPE_SHELL_LOG();
\status
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select DATABASE(), USER() limit 1");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: show session status like 'ssl_version';");


testutil.createFile("multi.sql", "create schema sql_logging; select 1; select 2; SELECT 3; select 4; select 5; select 6;\nselect 'a'; select 'b';\nselect 'c'; select 'd'; select 'e'; drop schema sql_logging;\n");

//@<> Testing on sql logging with \source (\.)
WIPE_SHELL_LOG();
\sql
\. multi.sql
\js

EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: create schema sql_logging/);
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 1");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 2");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: SELECT 3");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 4");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 5");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 6");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'a'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'b'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'c'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'd'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'e'");
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: drop schema sql_logging/);


//@<> Testing on sql logging from session object
WIPE_SHELL_LOG();
session.runSql("select 1");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 1");

//@<> Testing on sql logging from dba object {VER(>=8.0.0)}
WIPE_SHELL_LOG();
try {
    cluster = dba.getCluster()
} catch (error) {
    print(error)
}
EXPECT_SHELL_LOG_MATCHES(/Info: Dba.getCluster: tid=\d+: CONNECTED: localhost:\d+/);
EXPECT_SHELL_LOG_MATCHES(/Info: Dba.getCluster: tid=\d+: SQL: SET SESSION `autocommit` = 1/);
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: SHOW DATABASES LIKE 'mysql_innodb_cluster_metadata'");


//@<> Testing on sql logging from shell object
WIPE_SHELL_LOG();
shell.status()
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select DATABASE(), USER() limit 1");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: show session status like 'ssl_version';");

// =================================
// Using 'all' logging option
// =================================

\option logSql = "all"

//@<> Testing all sql logging with \use
WIPE_SHELL_LOG();
\use mysql
EXPECT_SHELL_LOG_MATCHES(/Info: js: tid=\d+: SQL: use `mysql`/);

//@<> Testing all sql logging with \sql
WIPE_SHELL_LOG();
\sql show databases
EXPECT_SHELL_LOG_CONTAINS("SQL: show databases");

WIPE_SHELL_LOG();
\sql CREATE USER 'test'@'localhost' IDENTIFIED BY ''
\sql DROP USER 'test'@'localhost'
EXPECT_SHELL_LOG_NOT_CONTAINS("CREATE USER 'test'@'localhost' IDENTIFIED BY ''");
EXPECT_SHELL_LOG_CONTAINS("DROP USER 'test'@'localhost'");

//@<> Testing all sql logging with \show
WIPE_SHELL_LOG()
\show query SELECT 1
EXPECT_SHELL_LOG_CONTAINS("SQL: SELECT 1");

WIPE_SHELL_LOG();
\show query CREATE USER 'test'@'localhost' IDENTIFIED BY ''
\show query DROP USER 'test'@'localhost'
EXPECT_SHELL_LOG_NOT_CONTAINS("CREATE USER 'test'@'localhost' IDENTIFIED BY ''");
EXPECT_SHELL_LOG_CONTAINS("DROP USER 'test'@'localhost'");

//@<> Testing all sql logging from \status command
WIPE_SHELL_LOG();
\status
EXPECT_SHELL_LOG_CONTAINS("SQL: select DATABASE(), USER() limit 1");
EXPECT_SHELL_LOG_CONTAINS("SQL: show session status like 'ssl_version';");

testutil.createFile("multi.sql", "create schema sql_logging; create user 'test'@'localhost' identified by ''; drop user 'test'@'localhost'; select 1; select 2; SELECT 3; select 4; select 5; select 6;\nselect 'a'; select 'b';\nselect 'c'; select 'd'; select 'e'; drop schema sql_logging;\n");

//@<> Testing all sql logging with \source (\.)
WIPE_SHELL_LOG();
\sql
\. multi.sql
\js

EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: create schema sql_logging/);
EXPECT_SHELL_LOG_NOT_CONTAINS("CREATE USER 'test'@'localhost' IDENTIFIED BY ''");
EXPECT_SHELL_LOG_CONTAINS("DROP USER 'test'@'localhost'");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 1");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 2");
EXPECT_SHELL_LOG_CONTAINS("SQL: SELECT 3");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 4");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 5");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 6");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 'a'");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 'b'");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 'c'");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 'd'");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 'e'");
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: SQL: drop schema sql_logging/);


//@<> Testing all sql logging from session object
WIPE_SHELL_LOG();
session.runSql("select 1");
EXPECT_SHELL_LOG_CONTAINS("SQL: select 1");

//@<> Testing all sql logging from dba object {VER(>=8.0.0)}
WIPE_SHELL_LOG();
try {
    cluster = dba.getCluster()
} catch (error) {
    print(error)
}
EXPECT_SHELL_LOG_MATCHES(/Info: Dba.getCluster: tid=\d+: CONNECTED: localhost:\d+/);
EXPECT_SHELL_LOG_MATCHES(/Info: Dba.getCluster: tid=\d+: SQL: SET SESSION `autocommit` = 1/);
EXPECT_SHELL_LOG_CONTAINS("SQL: SHOW DATABASES LIKE 'mysql_innodb_cluster_metadata'");


//@<> Testing all sql logging from shell object
WIPE_SHELL_LOG();
shell.status()
EXPECT_SHELL_LOG_CONTAINS("SQL: select DATABASE(), USER() limit 1");
EXPECT_SHELL_LOG_CONTAINS("SQL: show session status like 'ssl_version';");


// =================================
// Using 'off' logging option
// =================================

\option logSql = "off"

//@<> Testing off sql logging with \use
WIPE_SHELL_LOG();
\use mysql
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: use `mysql`");

//@<> Testing off sql logging with \sql
WIPE_SHELL_LOG();
\sql show databases
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: show databases");

//@<> Testing off sql logging with \show
WIPE_SHELL_LOG()
\show query SELECT 1
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: SELECT 1")

//@<> Testing off sql logging from \status command
WIPE_SHELL_LOG();
\status
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select DATABASE(), USER() limit 1");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: show session status like 'ssl_version';");


testutil.createFile("multi.sql", "create schema sql_logging; select 1; select 2; SELECT 3; select 4; select 5; select 6;\nselect 'a'; select 'b';\nselect 'c'; select 'd'; select 'e'; drop schema sql_logging;\n");

//@<> Testing off sql logging with \source (\.)
WIPE_SHELL_LOG();
\sql
\. multi.sql
\js

EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: create schema sql_logging");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 1");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 2");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: SELECT 3");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 4");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 5");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 6");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'a'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'b'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'c'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'd'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 'e'");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: drop schema sql_logging");


//@<> Testing off sql logging from session object
WIPE_SHELL_LOG();
session.runSql("select 1");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select 1");

//@<> Testing off sql logging from dba object {VER(>=8.0.0)}
WIPE_SHELL_LOG();
try {
    cluster = dba.getCluster()
} catch (error) {
    print(error)
}
EXPECT_SHELL_LOG_NOT_CONTAINS("CONNECTED: localhost:\d+");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: SET SESSION `autocommit` = 1");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: SHOW DATABASES LIKE 'mysql_innodb_cluster_metadata'");


//@<> Testing off sql logging from shell object
WIPE_SHELL_LOG();
shell.status()
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: select DATABASE(), USER() limit 1");
EXPECT_SHELL_LOG_NOT_CONTAINS("SQL: show session status like 'ssl_version';");


// =================================
// Using 'error' logging option
// =================================

\option logSql = "error"

//@<> Testing error sql logging with \use
WIPE_SHELL_LOG();
\use unexisting
EXPECT_SHELL_LOG_MATCHES(/Info: js: tid=\d+: MySQL Error 1049 \(42000\): Unknown database 'unexisting', SQL: use `unexisting`/);

//@<> Testing error sql logging with \sql
WIPE_SHELL_LOG();
\sql show whatever
EXPECT_SHELL_LOG_MATCHES(/Info: sql: tid=\d+: MySQL Error 1064 \(42000\): You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'whatever' at line 1, SQL: show whatever/);

//@<> Testing error sql logging with \show
WIPE_SHELL_LOG()
\show query whatever
EXPECT_SHELL_LOG_MATCHES(/Info: reports.query: tid=\d+: MySQL Error 1064 \(42000\): You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version for the right syntax to use near 'whatever' at line 1, SQL: whatever ;/);

//@<> Testing error sql logging from session object
WIPE_SHELL_LOG();
session.runSql("select whatever");
EXPECT_SHELL_LOG_MATCHES(/Info: ClassicSession.runSql: tid=\d+: MySQL Error 1054 \(42S22\): Unknown column 'whatever' in 'field list', SQL: select whatever/);

//@<> Clean-up
session.close();
testutil.rmfile("multi.sql");
testutil.destroySandbox(__mysql_sandbox_port1);
