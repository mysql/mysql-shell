// -----------------------------------------------------------------------------
// test 'threads' report

// -----------------------------------------------------------------------------
// helpers

function get_ids(s) {
  var ids = s.runSql("SELECT THREAD_ID, PROCESSLIST_ID from performance_schema.threads WHERE PROCESSLIST_ID = connection_id()").fetchOne();
  return {'tid': ids[0], 'cid': ids[1]};
}

function count_rows() {
  var line;
  var count = 0;

  while ((line = testutil.fetchCapturedStdout(true)) !== "") {
    if (line[0] === '|') {
      ++count;
    }
  }

  return count - 1;
}

// -----------------------------------------------------------------------------
// setup

//@ create a new session
shell.connect(__mysqluripwd);

//@ create test user
session.runSql("CREATE USER IF NOT EXISTS 'threads_test'@'%' IDENTIFIED BY 'pwd'");
session.runSql("GRANT ALL ON *.* TO 'threads_test'@'%'");

var __test_uri = "threads_test@" + __host + ":" + __mysql_port;
var __test_uripwd = "threads_test:pwd@" + __host + ":" + __mysql_port;

//@ create test connection
var __test_session = mysql.getClassicSession(__test_uripwd);

//@ get IDs
var __test_ids = get_ids(__test_session);
var __session_ids = get_ids(session);

//@ create database
session.runSql("DROP DATABASE IF EXISTS threads_test")
session.runSql("CREATE DATABASE IF NOT EXISTS threads_test")
session.runSql("CREATE TABLE IF NOT EXISTS threads_test.innodb (value int) ENGINE=InnoDB")
session.runSql("INSERT INTO threads_test.innodb VALUES (1)")

// -----------------------------------------------------------------------------
// tests

//@ WL11651-TSFR1_1 - Validate that a new report named threads is available in the Shell.
\show

//@ WL11651-TSFR1_2 - Run the threads report, validate that the info displayed belongs just to the user that executes the report.
var user = session.runSql("SELECT COUNT(*) from performance_schema.threads WHERE PROCESSLIST_USER = '" + __user + "'").fetchOne()[0];

WIPE_STDOUT();

\show threads

EXPECT_STDOUT_CONTAINS(__user);
EXPECT_STDOUT_NOT_CONTAINS("threads_test");
EXPECT_STDERR_EMPTY();

EXPECT_EQ(user, count_rows());

//@ display help for threads report
WIPE_STDOUT();

\show threads --help

// WL11651-TSFR2_1 - Display the help entry for the threads report, validate that the info include an entry for the --foreground (-f) option.
EXPECT_STDOUT_CONTAINS("--foreground, -f");

// WL11651-TSFR3_1 - Display the help entry for the threads report, validate that the info include an entry for the --background (-b) option.
EXPECT_STDOUT_CONTAINS("--background, -b");

// WL11651-TSFR4_1 - Display the help entry for the threads report, validate that the info include an entry for the --all (-A) option.
EXPECT_STDOUT_CONTAINS("--all, -A");

// WL11651-TSFR7_1 - Display the help entry for the threads report, validate that the info include an entry for the --format (-o) option.
EXPECT_STDOUT_CONTAINS("--format=string, -o");

// WL11651-TSFR8_1 - Display the help entry for the threads report, validate that the info include an entry for the --where option.
EXPECT_STDOUT_CONTAINS("--where=string");

// WL11651-TSFR9_1 - Display the help entry for the threads report, validate that the info include an entry for the --order-by option.
EXPECT_STDOUT_CONTAINS("--order-by=string");

// WL11651-TSFR10_1 - Display the help entry for the threads report, validate that the info include an entry for the --desc option.
EXPECT_STDOUT_CONTAINS("--desc");

// WL11651-TSFR11_1 - Display the help entry for the threads report, validate that the info include an entry for the --limit (-l) option.
EXPECT_STDOUT_CONTAINS("--limit=integer, -l");

EXPECT_STDERR_EMPTY();

//@ WL11651-TSFR2_2 - Run the threads report with the --foreground (-f) option, validate that the info displayed list all foreground threads.
var foreground = session.runSql("SELECT COUNT(*) from performance_schema.threads WHERE TYPE = 'FOREGROUND'").fetchOne()[0];

WIPE_STDOUT();

\show threads --foreground --format type

EXPECT_STDOUT_CONTAINS("FOREGROUND");
EXPECT_STDOUT_NOT_CONTAINS("BACKGROUND");
EXPECT_STDERR_EMPTY();

EXPECT_EQ(foreground, count_rows());

WIPE_STDOUT();

\show threads -f --format type

EXPECT_STDOUT_CONTAINS("FOREGROUND");
EXPECT_STDOUT_NOT_CONTAINS("BACKGROUND");
EXPECT_STDERR_EMPTY();

EXPECT_EQ(foreground, count_rows());

//@ WL11651-TSFR3_2 - Run the threads report with the --background (-b) option, validate that the info displayed list all background threads.
var background = session.runSql("SELECT COUNT(*) from performance_schema.threads WHERE TYPE = 'BACKGROUND'").fetchOne()[0];

WIPE_STDOUT();

\show threads --background --format type

EXPECT_STDOUT_CONTAINS("BACKGROUND");
EXPECT_STDOUT_NOT_CONTAINS("FOREGROUND");
EXPECT_STDERR_EMPTY();

EXPECT_EQ(background, count_rows());

WIPE_STDOUT();

\show threads -b --format type

EXPECT_STDOUT_CONTAINS("BACKGROUND");
EXPECT_STDOUT_NOT_CONTAINS("FOREGROUND");
EXPECT_STDERR_EMPTY();

EXPECT_EQ(background, count_rows());


//@ WL11651-TSFR4_2 - Run the threads report with the --all (-A) option, validate that the info displayed list all the threads: foreground and background.
var all = session.runSql("SELECT COUNT(*) from performance_schema.threads").fetchOne()[0];

WIPE_STDOUT();

\show threads --all --format type

EXPECT_STDOUT_CONTAINS("FOREGROUND");
EXPECT_STDOUT_CONTAINS("BACKGROUND");
EXPECT_STDERR_EMPTY();

EXPECT_EQ(all, count_rows());

WIPE_STDOUT();

\show threads -A --format type

EXPECT_STDOUT_CONTAINS("FOREGROUND");
EXPECT_STDOUT_CONTAINS("BACKGROUND");
EXPECT_STDERR_EMPTY();

EXPECT_EQ(all, count_rows());

//@ WL11651-TSFR4_3 - Run the threads report with the --foreground and --background options, validate that the info displayed list all the threads: foreground and background.
WIPE_STDOUT();

\show threads --foreground --background --format type

EXPECT_STDOUT_CONTAINS("FOREGROUND");
EXPECT_STDOUT_CONTAINS("BACKGROUND");
EXPECT_STDERR_EMPTY();

EXPECT_EQ(all, count_rows());

//@ WL11651-TSFR5_1 - Run the threads report, validate that the output of the report is presented as a list-type report.
\show threads -f -o user,command --where "tid = <<<__test_ids.tid>>>"

//@ WL11651-TSFR5_1 - Run the threads report, validate that the output of the report is presented as a list-type report - vertical.
\show threads --vertical -f -o user,command --where "tid = <<<__test_ids.tid>>>"

//@ WL11651-TSFR6_1 - Run the threads report, validate that the default columns in the report are: tid, cid, user, host, db, command, time, state, txstate, info, nblocking.
WIPE_STDOUT();

\show threads

EXPECT_STDOUT_MATCHES(/\| tid +\| cid +\| user +\| host +\| db +\| command +\| time +\| state +\| txstate +\| info +\| nblocking +\|/);
EXPECT_STDERR_EMPTY();

WIPE_STDOUT();

\show threads --foreground

EXPECT_STDOUT_MATCHES(/\| tid +\| cid +\| user +\| host +\| db +\| command +\| time +\| state +\| txstate +\| info +\| nblocking +\|/);
EXPECT_STDERR_EMPTY();

//@ WL11651-TSFR6_2 - Run the threads report with --background option, validate that the default columns in the report are: tid, name, nio, ioltncy, iominltncy, ioavgltncy, iomaxltncy.
WIPE_STDOUT();

\show threads --background

EXPECT_STDOUT_MATCHES(/\| tid +\| name +\| nio +\| ioltncy +\| iominltncy +\| ioavgltncy +\| iomaxltncy +\|/);
EXPECT_STDERR_EMPTY();

// WL11651-TSFR7_2 - Validate that the --format (-o) option only takes a string as its value with the following format: column[=alias][,column[=alias]]*.

//@ WL11651-TSFR7_2 - null
shell.reports.threads(session, [], {'format': null})

//@ WL11651-TSFR7_2 - undefined
shell.reports.threads(session, [], {'format': undefined})

//@ WL11651-TSFR7_2 - bool
shell.reports.threads(session, [], {'format': false})

//@ WL11651-TSFR7_2 - int [USE: WL11651-TSFR7_2 - bool]
shell.reports.threads(session, [], {'format': 1})

//@ WL11651-TSFR7_2 - string
shell.reports.threads(session, [], {'format': 'tid=TID,cid=CID', 'where': 'tid = ' + __session_ids.tid})

//@ WL11651-TSFR7_3 - Validate that the --format (-o) option can be used to specify the order of the columns in the report.
WIPE_OUTPUT();

\show threads --format cid,tid

EXPECT_STDOUT_MATCHES(/\| cid +\| tid +\|/);
EXPECT_STDOUT_MATCHES(/\| <<<__session_ids.cid>>> +\| <<<__session_ids.tid>>> +\|/);
EXPECT_STDERR_EMPTY();

WIPE_OUTPUT();

\show threads -o host,user

EXPECT_STDOUT_MATCHES(/\| host +\| user +\|/);
EXPECT_STDOUT_MATCHES(/\| <<<__host>>> +\| <<<__user>>> +\|/);
EXPECT_STDERR_EMPTY();

//@ WL11651-TSFR7_4 - Validate that the --format (-o) option can be used to specify alias for the columns in the report.
WIPE_OUTPUT();

\show threads --format cid=cciidd,tid=ttiidd

EXPECT_STDOUT_MATCHES(/\| cciidd +\| ttiidd +\|/);
EXPECT_STDOUT_MATCHES(/\| <<<__session_ids.cid>>> +\| <<<__session_ids.tid>>> +\|/);
EXPECT_STDERR_EMPTY();

WIPE_OUTPUT();

\show threads -o host=user,user=host

EXPECT_STDOUT_MATCHES(/\| user +\| host +\|/);
EXPECT_STDOUT_MATCHES(/\| <<<__host>>> +\| <<<__user>>> +\|/);
EXPECT_STDERR_EMPTY();

//@ WL11651-TSFR7_5 - Validate that all the columns listed in FR5 can be requested using the --format (-o) option.
function test_column(column, expected) {
  var result = shell.reports.threads(session, [], {'foreground': true, 'format': column, 'where': 'tid = ' + __test_ids.tid}).report;
  EXPECT_EQ(2, result.length, 'number of rows');
  EXPECT_EQ(1, result[0].length, 'number of columns');
  EXPECT_EQ(column, result[0][0], 'column: ' + column);
  if (expected !== 'ignore') {
    EXPECT_EQ(expected, result[1][0], 'value of: ' + column);
  }
}

var is_8_0 = testutil.versionCheck(__version, '>=', '8.0.0');

// tid - thread ID
test_column("tid", __test_ids.tid);

// cid - connection ID in case of threads associated with a user connection, or NULL for a background thread
test_column("cid", __test_ids.cid);

// command - the type of command the thread is executing
test_column("command", "Sleep");

// db - the default database, if one is selected; otherwise NULL
__test_session.runSql('USE mysql');
test_column("db", "mysql");

// group - the resource group label; this value is NULL if resource groups are not supported on the current platform or server configuration
test_column("group", is_8_0 ? session.runSql("SELECT t.RESOURCE_GROUP FROM performance_schema.threads AS t WHERE t.THREAD_ID = " + __test_ids.tid).fetchOne()[0] : null);

// history - whether event history logging is enabled for the thread
test_column("history", "YES");

// host - the host name of the client who issued the statement, or NULL for a background thread
test_column("host", __host);

// instr - whether events executed by the thread are instrumented
test_column("instr", "YES");

// lastwait - the name of the most recent wait event for the thread
test_column("lastwait", null);

// lastwaitl - the wait time of the most recent wait event for the thread
test_column("lastwaitl", null);

// memory - the number of bytes allocated by the thread
test_column("memory", "ignore");

// name - the name associated with the current thread instrumentation in the server
test_column("name", "sql/one_connection");

// osid - the thread or task identifier as defined by the underlying operating system, if there is one
test_column("osid", session.runSql("SELECT t.THREAD_OS_ID FROM performance_schema.threads AS t WHERE t.THREAD_ID = " + __test_ids.tid).fetchOne()[0]);

// protocol - the protocol used to establish the connection, or NULL for background threads
test_column("protocol", "SSL/TLS");

// ptid - If this thread is a subthread, this is the thread ID value of the spawning thread.
test_column("ptid", "ignore");

// started - time when thread started to be in its current state
test_column("started", "ignore");

// state - an action, event, or state that indicates what the thread is doing
test_column("state", null);

// tidle - the time the thread has been idle
test_column("tidle", "ignore");

// time - the time that the thread has been in its current state,
test_column("time", "ignore");

// type - the thread type, either FOREGROUND or BACKGROUND
test_column("type", "FOREGROUND");

// user - the user who issued the statement, or NULL for a background thread
test_column("user", "threads_test");

// digest - the hash value of the statement digest the thread is executing
test_column("digest", null);

// digesttxt - the normalized digest of the statement the thread is executing
test_column("digesttxt", null);

// fullscan - whether a full table scan was performed by the current statement
test_column("fullscan", "NO");

// info - the statement the thread is executing, truncated to be shorter
test_column("info", "ignore");

// infofull - the statement the thread is executing, or NULL if it is not executing any statement
test_column("infofull", "ignore");

// latency - how long the statement has been executing
test_column("latency", "ignore");

// llatency - the time spent waiting for locks by the current statement
test_column("llatency", "ignore");

// nraffected - the number of rows affected by the current statement
test_column("nraffected", 0);

// nrexamined - the number of rows read from storage engines by the current statement
test_column("nrexamined", 0);

// nrsent - the number of rows returned by the current statement
test_column("nrsent", 0);

// ntmpdtbls - the number of internal on-disk temporary tables created by the current statement
test_column("ntmpdtbls", 0);

// ntmptbls - the number of internal in-memory temporary tables created by the current statement
test_column("ntmptbls", 0);

// pdigest - the hash value of the last statement digest executed by the thread
__test_session.runSql('SELECT 1');
test_column("pdigest", is_8_0 ? "d1b44b0c19af710b5a679907e284acd2ddc285201794bc69a2389d77baedddae" : "3d4fc22e33e10d7235eced3c75a84c2c");

// pdigesttxt - the normalized digest of the last statement executed by the thread
test_column("pdigesttxt", is_8_0 ? "SELECT ?" : "SELECT ? ");

// pinfo - the last statement executed by the thread, if there is no currently executing statement or wait
test_column("pinfo", "SELECT 1");

// platency - how long the last statement was executed
test_column("platency", "ignore");

// progress - the percentage of work completed for stages that support progress reporting
test_column("progress", null);

// stmtid - ID of the statement that is currently being executed by the thread
test_column("stmtid", "ignore");

// nblocked - the number of other threads blocked by the thread
// tested in shell_reports_thread_norecord.py, when locks are set up

// nblocking - the number of other threads blocking the thread
// tested in shell_reports_thread_norecord.py, when locks are set up

// npstmts - the number of prepared statements allocated by the thread
test_column("npstmts", 0);

// nvars - the number of user variables defined for the thread
test_column("nvars", 0);

// begin InnoDB transaction in order to test related columns
__test_session.runSql("BEGIN");
__test_session.runSql("UPDATE threads_test.innodb SET value = 5");
os.sleep(1);

// ntxrlckd - the approximate number or rows locked by the current InnoDB transaction
test_column("ntxrlckd", 2);

// ntxrmod - the number of modified and inserted rows in the current InnoDB transaction
test_column("ntxrmod", 1);

// ntxtlckd - the number of InnoDB tables that the current transaction has row locks on
test_column("ntxtlckd", 1);

// txaccess - the access mode of the current InnoDB transaction
test_column("txaccess", "READ WRITE");

// txid - ID of the InnoDB transaction that is currently being executed by the thread
test_column("txid", "ignore");

// txisolvl - the isolation level of the current InnoDB transaction
test_column("txisolvl", "REPEATABLE READ");

// txstart - time when thread started executing the current InnoDB transaction
test_column("txstart", "ignore");

// txstate - the current InnoDB transaction state
test_column("txstate", "RUNNING");

// txtime - the time that the thread is executing the current InnoDB transaction
test_column("txtime", "ignore");

// finish the transaction
__test_session.runSql("ROLLBACK");
os.sleep(1);

// ioavgltncy - the average wait time per timed I/O event for the thread
test_column("ioavgltncy", "ignore");

// ioltncy - the total wait time of timed I/O events for the thread
test_column("ioltncy", "ignore");

// iomaxltncy - the maximum single wait time of timed I/O events for the thread
test_column("iomaxltncy", "ignore");

// iominltncy - the minimum single wait time of timed I/O events for the thread
test_column("iominltncy", "ignore");

// nio - the total number of I/O events for the thread
test_column("nio", "ignore");

// pid - the client process ID
test_column("pid", session.runSql("SELECT p.pid FROM sys.processlist AS p WHERE p.thd_id = " + __test_ids.tid).fetchOne()[0]);

// progname - the client program name
test_column("progname", "mysqlsh");

// ssl - SSL cipher in use by the client
var ssl_cipher = session.runSql("SELECT sbt.VARIABLE_VALUE FROM performance_schema.status_by_thread AS sbt WHERE sbt.THREAD_ID = " + __test_ids.tid + " AND sbt.VARIABLE_NAME = 'Ssl_cipher'").fetchOne()[0];
test_column("ssl", ssl_cipher);

// run a query which will result in error
try {
  __test_session.runSql("SELECT invalid_db.invalid_table");
} catch (e) {}

// diagerrno - the statement error number
test_column("diagerrno", 1109);

// diagerror - whether an error occurred for the statement
test_column("diagerror", "YES");

// diagstate - the statement SQLSTATE value
test_column("diagstate", "42S02");

// diagtext - the statement error message
test_column("diagtext", "42S02");

// diagwarn - the statement number of warnings
test_column("diagwarn", 0);

// nseljoin - the number of joins that perform table scans because they do not use indexes
test_column("nseljoin", 0);

// nselrjoin - the number of joins that used a range search on a reference table
test_column("nselrjoin", 0);

// nselrange - the number of joins that used ranges on the first table
test_column("nselrange", 0);

// nselrangec - the number of joins without keys that check for key usage after each row
test_column("nselrangec", 0);

// nselscan - the number of joins that did a full scan of the first table
test_column("nselscan", 0);

// nsortmp - the number of merge passes that the sort algorithm has had to do
test_column("nsortmp", 0);

// nsortrange - the number of sorts that were done using ranges
test_column("nsortrange", 0);

// nsortrows - the number of sorted rows
test_column("nsortrows", 0);

// nsortscan - the number of sorts that were done by scanning the table
test_column("nsortscan", 0);

// status - used as status.NAME, provides value of session status variable 'NAME'
test_column("status.ssl_cipher", ssl_cipher);
test_column("status.unknown_variable", null);

// system - used as system.NAME, provides value of session system variable 'NAME'
test_column("system.autocommit", "ON");
test_column("system.unknown_variable", null);

//@ status - run without variable name
\show threads -o status

//@ system - run without variable name
\show threads -o system

// WL11651-TSFR7_6 - Set the --format (-o) option to values different than the string with the specified format, validate that an exception is thrown: include empty value and string with invalid format.

//@ WL11651-TSFR7_6 - empty value
\show threads --format ""

//@ WL11651-TSFR7_6 - empty value - function call
shell.reports.threads(session, [], {'format': ''})

//@ WL11651-TSFR7_6 - invalid format
\show threads --format "tid cid"

//@ WL11651-TSFR7_7 - Use the --format (-o) option to request columns different than the ones listed in FR5, validate that an exception is thrown.
\show threads --format invalidcolumn

//@ WL11651-TSFR7_8 - Use the --format (-o) option to request columns with certain prefix, validate that all columns matching this prefix are displayed and the alias is ignored.
WIPE_STDOUT();

\show threads --format io=alias

EXPECT_STDOUT_MATCHES(/\| ioavgltncy +\| ioltncy +\| iomaxltncy +\| iominltncy +\|/);
EXPECT_STDERR_EMPTY();

//@ WL11651-TSFR7_8 - prefix does not match any column
\show threads --format k

//@ WL11651-TSFR7_9 - Use the --format (-o) option to request a column listed in FR5 but using an upper-case name.
WIPE_STDOUT();

\show threads -f --format CID

EXPECT_STDOUT_MATCHES(/\| CID +\|/);
EXPECT_STDOUT_MATCHES(/\| <<<__test_ids.cid>>> +\|/);
EXPECT_STDOUT_MATCHES(/\| <<<__session_ids.cid>>> +\|/);
EXPECT_STDERR_EMPTY();

//@ WL11651-TSFR8_2 - Validate that all the columns listed in FR5 can be used as a key to filter the results of the report.
var all_columns = [
  // tid - thread ID
  "tid",
  // cid - connection ID in case of threads associated with a user connection, or NULL for a background thread
  "cid",
  // command - the type of command the thread is executing
  "command",
  // db - the default database, if one is selected; otherwise NULL
  "db",
  // group - the resource group label; this value is NULL if resource groups are not supported on the current platform or server configuration
  "group",
  // history - whether event history logging is enabled for the thread
  "history",
  // host - the host name of the client who issued the statement, or NULL for a background thread
  "host",
  // instr - whether events executed by the thread are instrumented
  "instr",
  // lastwait - the name of the most recent wait event for the thread
  "lastwait",
  // lastwaitl - the wait time of the most recent wait event for the thread
  "lastwaitl",
  // memory - the number of bytes allocated by the thread
  "memory",
  // name - the name associated with the current thread instrumentation in the server
  "name",
  // osid - the thread or task identifier as defined by the underlying operating system, if there is one
  "osid",
  // protocol - the protocol used to establish the connection, or NULL for background threads
  "protocol",
  // ptid - If this thread is a subthread, this is the thread ID value of the spawning thread.
  "ptid",
  // started - time when thread started to be in its current state
  "started",
  // state - an action, event, or state that indicates what the thread is doing
  "state",
  // tidle - the time the thread has been idle
  "tidle",
  // time - the time that the thread has been in its current state,
  "time",
  // type - the thread type, either FOREGROUND or BACKGROUND
  "type",
  // user - the user who issued the statement, or NULL for a background thread
  "user",
  // digest - the hash value of the statement digest the thread is executing
  "digest",
  // digesttxt - the normalized digest of the statement the thread is executing
  "digesttxt",
  // fullscan - whether a full table scan was performed by the current statement
  "fullscan",
  // info - the statement the thread is executing, truncated to be shorter
  "info",
  // infofull - the statement the thread is executing, or NULL if it is not executing any statement
  "infofull",
  // latency - how long the statement has been executing
  "latency",
  // llatency - the time spent waiting for locks by the current statement
  "llatency",
  // nraffected - the number of rows affected by the current statement
  "nraffected",
  // nrexamined - the number of rows read from storage engines by the current statement
  "nrexamined",
  // nrsent - the number of rows returned by the current statement
  "nrsent",
  // ntmpdtbls - the number of internal on-disk temporary tables created by the current statement
  "ntmpdtbls",
  // ntmptbls - the number of internal in-memory temporary tables created by the current statement
  "ntmptbls",
  // pdigest - the hash value of the last statement digest executed by the thread
  "pdigest",
  // pdigesttxt - the normalized digest of the last statement executed by the thread
  "pdigesttxt",
  // pinfo - the last statement executed by the thread, if there is no currently executing statement or wait
  "pinfo",
  // platency - how long the last statement was executed
  "platency",
  // progress - the percentage of work completed for stages that support progress reporting
  "progress",
  // stmtid - ID of the statement that is currently being executed by the thread
  "stmtid",
  // nblocked - the number of other threads blocked by the thread
  "nblocked",
  // nblocking - the number of other threads blocking the thread
  "nblocking",
  // npstmts - the number of prepared statements allocated by the thread
  "npstmts",
  // nvars - the number of user variables defined for the thread
  "nvars",
  // ntxrlckd - the approximate number or rows locked by the current InnoDB transaction
  "ntxrlckd",
  // ntxrmod - the number of modified and inserted rows in the current InnoDB transaction
  "ntxrmod",
  // ntxtlckd - the number of InnoDB tables that the current transaction has row locks on
  "ntxtlckd",
  // txaccess - the access mode of the current InnoDB transaction
  "txaccess",
  // txid - ID of the InnoDB transaction that is currently being executed by the thread
  "txid",
  // txisolvl - the isolation level of the current InnoDB transaction
  "txisolvl",
  // txstart - time when thread started executing the current InnoDB transaction
  "txstart",
  // txstate - the current InnoDB transaction state
  "txstate",
  // txtime - the time that the thread is executing the current InnoDB transaction
  "txtime",
  // ioavgltncy - the average wait time per timed I/O event for the thread
  "ioavgltncy",
  // ioltncy - the total wait time of timed I/O events for the thread
  "ioltncy",
  // iomaxltncy - the maximum single wait time of timed I/O events for the thread
  "iomaxltncy",
  // iominltncy - the minimum single wait time of timed I/O events for the thread
  "iominltncy",
  // nio - the total number of I/O events for the thread
  "nio",
  // pid - the client process ID
  "pid",
  // progname - the client program name
  "progname",
  // ssl - SSL cipher in use by the client
  "ssl",
  // diagerrno - the statement error number
  "diagerrno",
  // diagerror - whether an error occurred for the statement
  "diagerror",
  // diagstate - the statement SQLSTATE value
  "diagstate",
  // diagtext - the statement error message
  "diagtext",
  // diagwarn - the statement number of warnings
  "diagwarn",
  // nseljoin - the number of joins that perform table scans because they do not use indexes
  "nseljoin",
  // nselrjoin - the number of joins that used a range search on a reference table
  "nselrjoin",
  // nselrange - the number of joins that used ranges on the first table
  "nselrange",
  // nselrangec - the number of joins without keys that check for key usage after each row
  "nselrangec",
  // nselscan - the number of joins that did a full scan of the first table
  "nselscan",
  // nsortmp - the number of merge passes that the sort algorithm has had to do
  "nsortmp",
  // nsortrange - the number of sorts that were done using ranges
  "nsortrange",
  // nsortrows - the number of sorted rows
  "nsortrows",
  // nsortscan - the number of sorts that were done by scanning the table
  "nsortscan",
  // status - used as status.NAME, provides value of session status variable 'NAME'
  "status.bytes_sent",
  // system - used as system.NAME, provides value of session system variable 'NAME'
  "system.autocommit"
];

function test_column_where(column) {
  EXPECT_NO_THROWS(function(){ shell.reports.threads(session, [], {'format': column, 'where': column + ' = 0'}); }, column);
}

for (c of all_columns) {
  test_column_where(c);
}

//@ WL11651-TSFR8_3 - Validate that the use of key names different than the columns listed in FR5 causes an exception.
\show threads -A --where "invalidcolumn = 1"

//@ WL11651-TSFR8_4 - Try to use non SQL values as the value for a key using the --where option, an exception should be thrown.
\show threads -A --where "name = one"

//@ WL11651-TSFR8_5 - Validate that the following relational operator can be used with the --where option: =, !=, <>, >, >=, <, <=, LIKE.
function test_operator(op) {
  EXPECT_NO_THROWS(function(){ shell.reports.threads(session, [], {'all': true, 'where': "tid " + op + " " + __session_ids.tid}); }, op);
}

test_operator("=");
test_operator("!=");
test_operator("<>");
test_operator(">");
test_operator(">=");
test_operator("<");
test_operator("<=");
test_operator("LIKE");

// disallowed operator
EXPECT_THROWS(function(){ shell.reports.threads(session, [], {'all': true, 'where': "tid | " + __session_ids.tid}); }, "reports.threads: Failed to parse 'where' parameter: Disallowed operator: |");

//@ WL11651-TSFR8_6 - Validate that the --where option support the following logical operators: AND, OR, NOT. (case insensitive)
function test_logic_operator(op) {
  EXPECT_NO_THROWS(function(){ shell.reports.threads(session, [], {'all': true, 'where': "tid = " + __session_ids.tid + " " + op + " cid = " + __session_ids.cid}); }, op);
}

test_logic_operator("AND");
test_logic_operator("and");
test_logic_operator("And");
test_logic_operator("OR");
test_logic_operator("or");
test_logic_operator("Or");

EXPECT_NO_THROWS(function(){ shell.reports.threads(session, [], {'all': true, 'where': "NOT tid = " + __session_ids.tid}); }, "NOT");
EXPECT_NO_THROWS(function(){ shell.reports.threads(session, [], {'all': true, 'where': "not tid = " + __session_ids.tid}); }, "not");
EXPECT_NO_THROWS(function(){ shell.reports.threads(session, [], {'all': true, 'where': "Not tid = " + __session_ids.tid}); }, "Not");

// disallowed logic operator
EXPECT_THROWS(function(){ shell.reports.threads(session, [], {'all': true, 'where': "tid = " + __session_ids.tid + " IS cid = " + __session_ids.cid}); }, "reports.threads: Failed to parse 'where' parameter: Disallowed operator: is");

//@ WL11651-TSFR8_7 - Validate that the --where option support multiple logical operations.
//  WL11651-TSFR8_8 - Validate that the --where option support grouping logical operations using parenthesis.
WIPE_OUTPUT();

\show threads -A -o tid,cid --where "(tid = <<<__test_ids.tid>>> OR cid = <<<__test_ids.cid>>>) AND NOT (tid = <<<__session_ids.tid>>> OR cid = <<<__session_ids.cid>>>)"

EXPECT_STDOUT_CONTAINS(__test_ids.tid);
EXPECT_STDOUT_CONTAINS(__test_ids.cid);
EXPECT_STDOUT_NOT_CONTAINS(__session_ids.tid);
EXPECT_STDOUT_NOT_CONTAINS(__session_ids.cid);
EXPECT_STDERR_EMPTY();

EXPECT_EQ(1, count_rows());

// WL11651-TSFR8_9 - Validate that an exception is thrown if the --where option is used without arguments.

//@ WL11651-TSFR8_9 - empty value
\show threads --where ""

//@ WL11651-TSFR8_9 - empty value - function call
shell.reports.threads(session, [], {'where': ''})

//@ WL11651-TSFR8_10 - Validate that an exception is thrown if the --where option receive an argument that doesn't comply with the form specified.
\show threads --where "one two"

//@ WL11651-TSFR9_2 - When the --order-by option is not used, validate that the report is ordered by the tid column.
WIPE_STDOUT();

\show threads -f

EXPECT_STDOUT_CONTAINS(__test_ids.tid);
EXPECT_STDOUT_CONTAINS(__session_ids.tid);
EXPECT_STDERR_EMPTY();

var stdout = testutil.fetchCapturedStdout(false);

// if one ID is smaller than the other one, it should appear in the output before the other one
var test_tid_offset = stdout.indexOf("| " + __test_ids.tid + " ");
var session_tid_offset = stdout.indexOf("| " + __session_ids.tid + " ");
EXPECT_TRUE(test_tid_offset !== -1);
EXPECT_TRUE(session_tid_offset !== -1);
EXPECT_TRUE((__session_ids.tid < __test_ids.tid) === (session_tid_offset < test_tid_offset));

//@ WL11651-TSFR9_3 - When the --order-by option is used, validate that the report is ordered by the column specified.

WIPE_STDOUT();

\show threads -A -o tid,type --order-by type

EXPECT_STDERR_EMPTY();

var stdout = testutil.fetchCapturedStdout(false);
var foreground = stdout.indexOf('FOREGROUND');

EXPECT_NE(-1, foreground, 'There should be at least one foreground thread');

var background = -1;

while ((background = stdout.indexOf('BACKGROUND', background + 1)) !== -1) {
  EXPECT_TRUE(background < foreground);
}

//@ WL11651-TSFR9_4 - Use multiple columns with the --order-by option, validate that the report is ordered by the columns specified.
var result = shell.reports.threads(session, [], {'all': true, 'format': 'tid,type', 'order_by': 'type,tid'}).report;
var type = 'BACKGROUND';

for (var i = 1; i < result.length - 1; ++i) {
  // if type is the same, tid should grow
  if (result[i][1] === result[i + 1][1]) {
    EXPECT_EQ(type, result[i][1]);
    EXPECT_TRUE(result[i][0] < result[i + 1][0]);
  } else {
    // type has changed from background to foreground, order of tid does not have to be maintained here
    EXPECT_EQ('FOREGROUND', result[i + 1][1]);
    type = 'FOREGROUND';
  }
}

//@ WL11651-TSFR9_5 - Validate that the --order-by only accepts the columns listed in FR5 as valid columns to order the report.
function test_column_order_by(column) {
  EXPECT_NO_THROWS(function(){ shell.reports.threads(session, [], {'format': column, 'order_by': column}); }, column);
}

for (c of all_columns) {
  test_column_order_by(c);
}

//@ WL11651-TSFR9_X - Validate that the --order-by throws an exception if column is unknown.
\show threads --order-by invalidcolumn

//@ WL11651-TSFR10_2 - When using the --desc option, validate that the output is sorted in descending order.
var result = shell.reports.threads(session, [], {'all': true, 'format': 'tid', 'desc': true}).report;

for (var i = 1; i < result.length - 1; ++i) {
  EXPECT_TRUE(result[i][0] > result[i + 1][0]);
}

//@ WL11651-TSFR10_3 - When not using the --desc option, validate that the output is sorted in ascending order.
var result = shell.reports.threads(session, [], {'all': true, 'format': 'tid'}).report;

for (var i = 1; i < result.length - 1; ++i) {
  EXPECT_TRUE(result[i][0] < result[i + 1][0]);
}

//@ WL11651-TSFR11_2 - When using the --limit option, validate that the output is limited to the number of threads specified.
var limit = 5;
var result = shell.reports.threads(session, [], {'all': true, 'limit': limit}).report;
EXPECT_EQ(limit, result.length - 1);

// -----------------------------------------------------------------------------
// cleanup

//@ cleanup - delete the database
session.runSql("DROP DATABASE threads_test")

//@ cleanup - delete the user
session.runSql("DROP USER 'threads_test'@'%'");

//@ cleanup - disconnect test session
__test_session.close();

//@ cleanup - disconnect the session
session.close();
