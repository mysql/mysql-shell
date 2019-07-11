# -----------------------------------------------------------------------------
# test 'thread' report
import re
import subprocess
import time

# -----------------------------------------------------------------------------
# helpers

def get_ids(s):
  ids = s.run_sql("SELECT THREAD_ID, PROCESSLIST_ID from performance_schema.threads WHERE PROCESSLIST_ID = connection_id()").fetch_one()
  return {'tid': ids[0], 'cid': ids[1]}

def wait_for_blocked_thread():
    retry_count = 0
    blocked_cid = None
    while retry_count < 5:
        # need to wait a bit for process to start
        time.sleep(1)
        blocked_cid = session.run_sql("SELECT t.PROCESSLIST_ID FROM performance_schema.threads AS t WHERE t.PROCESSLIST_USER = 'thread_test' AND t.THREAD_ID <> {0}".format(__test_ids['tid'])).fetch_one()
        if blocked_cid is None:
            exit_code = sh.poll()
            if exit_code is not None:
                context = "<b>Context:</b> " + __test_context + "\n<red>Shell terminated with exit code:</red> " + str(exit_code)
                testutil.fail(context)
                break
        else:
            blocked_cid = blocked_cid[0]
            break
        retry_count += 1
    return blocked_cid

# -----------------------------------------------------------------------------
# setup

#@ create a new session
shell.connect(__mysqluripwd)

#@ create test user
session.run_sql("CREATE USER IF NOT EXISTS 'thread_test'@'%' IDENTIFIED BY 'pwd'")
session.run_sql("GRANT ALL ON *.* TO 'thread_test'@'%'")

__test_uri = "thread_test@{0}:{1}".format(__host, __mysql_port)
__test_uripwd = "thread_test:pwd@{0}:{1}".format(__host, __mysql_port)

#@ create test connection
__test_session = mysql.get_classic_session(__test_uripwd)

#@ get IDs
__test_ids = get_ids(__test_session)
__session_ids = get_ids(session)

#@ create database
session.run_sql("CREATE DATABASE IF NOT EXISTS thread_test")
session.run_sql("CREATE TABLE IF NOT EXISTS thread_test.innodb (value int) ENGINE=InnoDB")
session.run_sql("INSERT INTO thread_test.innodb VALUES (1)")

# -----------------------------------------------------------------------------
# tests

#@ WL11651-TSFR12_1 - Validate that a new report named thread is available in the Shell.
\show

#@ WL11651-TSFR12_2 - Run the thread report without giving any options, validate that the report print information about the thread used by the current connection.
\show thread

#@ display help for thread report
WIPE_STDOUT()

\show thread --help

# WL11651-TSFR13_1 - Display the help entry for the thread report, validate that the info include an entry for the --tid (-t) option.
EXPECT_STDOUT_CONTAINS("--tid=integer, -t")

# WL11651-TSFR14_1 - Display the help entry for the thread report, validate that the info include an entry for the --cid (-c) option.
EXPECT_STDOUT_CONTAINS("--cid=integer, -c")

# WL11651-TSFR15_1 - Display the help entry for the thread report, validate that the info include an entry for the --brief (-B) option.
EXPECT_STDOUT_CONTAINS("--brief, -B")

# WL11651-TSFR16_1 - Display the help entry for the thread report, validate that the info include an entry for the --client (-C) option.
EXPECT_STDOUT_CONTAINS("--client, -C")

# WL11651-TSFR17_1 - Display the help entry for the thread report, validate that the info include an entry for the --innodb (-I) option.
EXPECT_STDOUT_CONTAINS("--innodb, -I")

# WL11651-TSFR18_1 - Display the help entry for the thread report, validate that the info include an entry for the --locks (-L) option.
EXPECT_STDOUT_CONTAINS("--locks, -L")

# WL11651-TSFR19_1 - Display the help entry for the thread report, validate that the info include an entry for the --prep-stmts (-P) option.
EXPECT_STDOUT_CONTAINS("--prep-stmts, -P")

# WL11651-TSFR20_1 - Display the help entry for the thread report, validate that the info include an entry for the --status (-S) option.
EXPECT_STDOUT_CONTAINS("--status[=string], -S")

# WL11651-TSFR21_1 - Display the help entry for the thread report, validate that the info include an entry for the --general (-G) option.
EXPECT_STDOUT_CONTAINS("--general, -G")

# WL11651-TSFR22_1 - Display the help entry for the thread report, validate that the info include an entry for the --vars (-V) option.
EXPECT_STDOUT_CONTAINS("--vars[=string], -V")

# WL11651-TSFR23_1 - Display the help entry for the thread report, validate that the info include an entry for the --user-vars (-U) option.
EXPECT_STDOUT_CONTAINS("--user-vars[=string], -U")

# WL11651-TSFR24_1 - Display the help entry for the thread report, validate that the info include an entry for the --all (-A) option.
EXPECT_STDOUT_CONTAINS("--all, -A")

# WL11651-TSFR13_2 - Validate that when the --tid (-t) option is used, the argument given is treated as a thread ID. The report must display information about the thread ID.

#@ WL11651-TSFR13_2 - --tid
\show thread --tid <<<__test_ids['tid']>>>

#@ WL11651-TSFR13_2 - -t [USE: WL11651-TSFR13_2 - --tid]
\show thread -t <<<__test_ids['tid']>>>

#@ WL11651-TSFR13_X - Invalid thread ID
invalid_tid = session.run_sql("SELECT MAX(THREAD_ID) FROM performance_schema.threads").fetch_one()[0] + 1
\show thread --tid <<<invalid_tid>>>

# WL11651-TSFR14_2 - Validate that when the --cid (-c) option is used, the argument given is treated as a connection ID. The report must display information about the connection ID.

#@ WL11651-TSFR14_2 - --cid [USE: WL11651-TSFR13_2 - --tid]
\show thread --cid <<<__test_ids['cid']>>>

#@ WL11651-TSFR14_2 - -c [USE: WL11651-TSFR13_2 - --tid]
\show thread -c <<<__test_ids['cid']>>>

# WL11651-TSFR14_3 - Validate that when both --tid (-t) and --cid (-c) options are used at the same time, an exception is thrown.

#@ WL11651-TSFR14_3 - tid + cid
\show thread --tid <<<__test_ids['tid']>>> --cid <<<__test_ids['cid']>>>

#@ WL11651-TSFR14_3 - tid + c [USE: WL11651-TSFR14_3 - tid + cid]
\show thread --tid <<<__test_ids['tid']>>> -c <<<__test_ids['cid']>>>

#@ WL11651-TSFR14_3 - t + cid [USE: WL11651-TSFR14_3 - tid + cid]
\show thread -t <<<__test_ids['tid']>>> --cid <<<__test_ids['cid']>>>

#@ WL11651-TSFR14_3 - t + c [USE: WL11651-TSFR14_3 - tid + cid]
\show thread -t <<<__test_ids['tid']>>> -c <<<__test_ids['cid']>>>

#@ WL11651-TSFR14_X - Invalid connection ID [USE: WL11651-TSFR13_X - Invalid thread ID]
invalid_cid = session.run_sql("SELECT MAX(PROCESSLIST_ID) FROM performance_schema.threads").fetch_one()[0] + 1
\show thread --cid <<<invalid_cid>>>

# WL11651-TSFR15_2 - When using the --brief (-B) option, validate that the report is presented in one line containing the fields: thread ID, connection ID (if available), user@host (if available).

#@ WL11651-TSFR15_2 - --brief
\show thread --tid <<<__test_ids['tid']>>> --brief

#@ WL11651-TSFR15_2 - -B [USE: WL11651-TSFR15_2 - --brief]
\show thread --tid <<<__test_ids['tid']>>> -B

# WL11651-TSFR16_2 - When using the --client (-C) option, validate that the following extra information is displayed by the report: attributes set by the client (from performance_schema.session_connect_attrs), protocol being used, socket IP, socket port, socket state, compression used, SSL cipher, SSL version.

#@ WL11651-TSFR16_2 - setup
session.run_sql("UPDATE performance_schema.setup_instruments AS setup SET ENABLED = 'NO', TIMED = 'NO' WHERE setup.NAME = (SELECT si.EVENT_NAME FROM performance_schema.socket_instances AS si WHERE si.THREAD_ID = {0})".format(__test_ids['tid']))
compression = session.run_sql("SELECT sbt.VARIABLE_VALUE FROM performance_schema.status_by_thread AS sbt WHERE sbt.THREAD_ID = {0} AND sbt.VARIABLE_NAME = 'Compression'".format(__test_ids['tid'])).fetch_one()[0]
ssl_cipher = session.run_sql("SELECT sbt.VARIABLE_VALUE FROM performance_schema.status_by_thread AS sbt WHERE sbt.THREAD_ID = {0} AND sbt.VARIABLE_NAME = 'Ssl_cipher'".format(__test_ids['tid'])).fetch_one()[0]
ssl_version = session.run_sql("SELECT sbt.VARIABLE_VALUE FROM performance_schema.status_by_thread AS sbt WHERE sbt.THREAD_ID = {0} AND sbt.VARIABLE_NAME = 'Ssl_version'".format(__test_ids['tid'])).fetch_one()[0]

#@ WL11651-TSFR16_2 - --client
\show thread --tid <<<__test_ids['tid']>>> --client

#@ WL11651-TSFR16_2 - -C [USE: WL11651-TSFR16_2 - --client]
\show thread --tid <<<__test_ids['tid']>>> -C

# WL11651-TSFR16_3 - When using the --client (-C) option and instrumentation is enabled for the socket used by the thread, validate that the report display statistic about the read and write operations: number of operations, total number of bytes, total wait time, minimum wait time, average wait time, maximum wait time.

#@ WL11651-TSFR16_3 - setup
session.run_sql("UPDATE performance_schema.setup_instruments AS setup SET ENABLED = 'YES', TIMED = 'YES' WHERE setup.NAME = (SELECT si.EVENT_NAME FROM performance_schema.socket_instances AS si WHERE si.THREAD_ID = {0})".format(__test_ids['tid']))

#@ WL11651-TSFR16_3 - --client
\show thread --tid <<<__test_ids['tid']>>> --client

#@ WL11651-TSFR16_3 - -C [USE: WL11651-TSFR16_3 - --client]
\show thread --tid <<<__test_ids['tid']>>> -C

#@ WL11651-TSFR16_4 - When using the --client (-C) option and a background thread, validate that no information is provided.
background_tid = session.run_sql("SELECT t.THREAD_ID FROM performance_schema.threads AS t WHERE t.TYPE = 'BACKGROUND'").fetch_one()[0]
\show thread --tid <<<background_tid>>> --client

# WL11651-TSFR17_2 - When using the --innodb (-I) option, validate that the report display information about the current InnoDB transaction.

#@ WL11651-TSFR17_2 - setup
__test_session.run_sql("BEGIN")
__test_session.run_sql("UPDATE thread_test.innodb SET value = 5")

# need to wait a bit for transaction to be registered
time.sleep(1)

#@ WL11651-TSFR17_2 - --innodb
\show thread --tid <<<__test_ids['tid']>>> --innodb

#@ WL11651-TSFR17_2 - -I [USE: WL11651-TSFR17_2 - --innodb]
\show thread --tid <<<__test_ids['tid']>>> -I

#@ WL11651-TSFR17_2 - cleanup
__test_session.run_sql("ROLLBACK")

# need to wait a bit for rollback to happen
time.sleep(1)

#@ WL11651-TSFR17_X - When there's no transaction information is not available.
\show thread --tid <<<__test_ids['tid']>>> --innodb

# WL11651-TSFR18_2 - When using the --locks (-L) option, validate that all required data is displayed.

#@ WL11651-TSFR18_2 - setup
session.run_sql("UPDATE performance_schema.setup_instruments SET ENABLED = 'YES', TIMED = 'YES' WHERE NAME = 'wait/lock/metadata/sql/mdl'")
__test_session.run_sql("LOCK TABLES thread_test.innodb WRITE")

sh = subprocess.Popen([__mysqlsh, __test_uripwd, '--sql', '-e', 'SELECT * FROM thread_test.innodb'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

blocked_cid = wait_for_blocked_thread()

#@ WL11651-TSFR18_2 - --locks - blocking
WIPE_STDOUT()

\show thread --tid <<<__test_ids['tid']>>> --locks

EXPECT_STDOUT_MATCHES(re.compile(r'| Wait started +| Elapsed +| Locked table +| Type +| CID +| Query +|'))
EXPECT_STDOUT_CONTAINS(str(blocked_cid))
EXPECT_STDOUT_CONTAINS('`thread_test`.`innodb`')
EXPECT_STDOUT_CONTAINS('SHARED_READ (TRANSACTION)')
EXPECT_STDOUT_CONTAINS('SELECT * FROM thread_test.innodb')

#@ WL11651-TSFR18_4 - When using the --locks (-L) option and thread has lock information, validate that the additional information is displayed in tables. (1)
EXPECT_STDOUT_MATCHES(re.compile(r'^|-+|-+|-+|-+|-+|-+|$'))

#@ WL11651-TSFR18_2 - --locks - blocked
WIPE_STDOUT()

\show thread --cid <<<blocked_cid>>> --locks

EXPECT_STDOUT_MATCHES(re.compile(r'| Wait started +| Elapsed +| Locked table +| Type +| CID +| Query +| Account +| Transaction started +| Elapsed +|'))
EXPECT_STDOUT_CONTAINS(str(__test_ids['cid']))
EXPECT_STDOUT_CONTAINS('`thread_test`.`innodb`')
EXPECT_STDOUT_CONTAINS('SHARED_READ (TRANSACTION)')
EXPECT_STDOUT_CONTAINS('thread_test@{0}'.format(__host))

#@ WL11651-TSFR18_4 - When using the --locks (-L) option and thread has lock information, validate that the additional information is displayed in tables. (2)
EXPECT_STDOUT_MATCHES(re.compile(r'^|-+|-+|-+|-+|-+|-+|-+|-+|-+|$'))

#@ WL11651-TSFR7_5 - Validate that all the columns listed in FR5 can be requested using the --format (-o) option. - nblocked, nblocking - metadata
report = shell.reports.threads(session, [], {'foreground': True, 'format': 'nblocked,nblocking', 'where': 'tid = {0}'.format(__test_ids['tid'])})['report']
EXPECT_EQ(1, report[1][0])
EXPECT_EQ(0, report[1][1])

report = shell.reports.threads(session, [], {'foreground': True, 'format': 'nblocked,nblocking', 'where': 'cid = {0}'.format(blocked_cid)})['report']
EXPECT_EQ(0, report[1][0])
EXPECT_EQ(1, report[1][1])

#@ WL11651-TSFR18_2 - cleanup
__test_session.run_sql("UNLOCK TABLES")

sh.wait()

#@ WL11651-TSFR18_2 - setup - InnoDB
__test_session.run_sql("BEGIN")
__test_session.run_sql("UPDATE thread_test.innodb SET value = 5")

sh = subprocess.Popen([__mysqlsh, __test_uripwd, '--sql', '-e', 'UPDATE thread_test.innodb SET value = 7'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

blocked_cid = wait_for_blocked_thread()

#@ WL11651-TSFR18_2 - --locks - blocking - InnoDB
WIPE_STDOUT()

\show thread --tid <<<__test_ids['tid']>>> --locks

EXPECT_STDOUT_MATCHES(re.compile(r'| Wait started +| Elapsed +| Locked table +| Type +| CID +| Query +|'))
EXPECT_STDOUT_CONTAINS(str(blocked_cid))
EXPECT_STDOUT_CONTAINS('`thread_test`.`innodb`')
EXPECT_STDOUT_CONTAINS('RECORD')
EXPECT_STDOUT_CONTAINS('UPDATE thread_test.innodb SET value = 7')

#@ WL11651-TSFR18_4 - When using the --locks (-L) option and thread has lock information, validate that the additional information is displayed in tables. (3)
EXPECT_STDOUT_MATCHES(re.compile(r'^|-+|-+|-+|-+|-+|-+|$'))

#@ WL11651-TSFR18_2 - --locks - blocked - InnoDB
WIPE_STDOUT()

\show thread --cid <<<blocked_cid>>> --locks

EXPECT_STDOUT_MATCHES(re.compile(r'| Wait started +| Elapsed +| Locked table +| Type +| CID +| Query +| Account +| Transaction started +| Elapsed +|'))
EXPECT_STDOUT_CONTAINS(str(__test_ids['cid']))
EXPECT_STDOUT_CONTAINS('`thread_test`.`innodb`')
EXPECT_STDOUT_CONTAINS('RECORD')
EXPECT_STDOUT_CONTAINS('thread_test@{0}'.format(__host))

#@ WL11651-TSFR18_4 - When using the --locks (-L) option and thread has lock information, validate that the additional information is displayed in tables. (4)
EXPECT_STDOUT_MATCHES(re.compile(r'^|-+|-+|-+|-+|-+|-+|-+|-+|-+|$'))

#@ WL11651-TSFR7_5 - Validate that all the columns listed in FR5 can be requested using the --format (-o) option. - nblocked, nblocking - InnoDB
report = shell.reports.threads(session, [], {'foreground': True, 'format': 'nblocked,nblocking', 'where': 'tid = {0}'.format(__test_ids['tid'])})['report']
EXPECT_EQ(1, report[1][0])
EXPECT_EQ(0, report[1][1])

report = shell.reports.threads(session, [], {'foreground': True, 'format': 'nblocked,nblocking', 'where': 'cid = {0}'.format(blocked_cid)})['report']
EXPECT_EQ(0, report[1][0])
EXPECT_EQ(1, report[1][1])


#@ WL11651-TSFR18_2 - cleanup - InnoDB
__test_session.run_sql("ROLLBACK")

sh.wait()

# need to wait a bit for rollback to happen
time.sleep(1)

#@ WL11651-TSFR18_3 - When using the --locks (-L) option and the thread has not any locks, validate that no additional information is displayed.
\show thread --tid <<<__test_ids['tid']>>> --locks

# WL11651-TSFR19_2 - When using the --prep-stmts (-P) option, validate that the report display the following information about prepared statements: ID of the prepared statement, ID of the event which created the statement, name of the statement, or NULL if not available, name of the stored program which created the statement, or NULL if not available, time it took to prepare the statement, number of times the prepared statement was executed, execution time (total, minimum, average, maximum).

#@ WL11651-TSFR19_2 - setup
__test_session.run_sql("PREPARE stmt1 FROM 'SELECT SQRT(POW(10,2) + POW(20,2))'")
__test_session.run_sql("EXECUTE stmt1")
__test_session.run_sql("EXECUTE stmt1")
__test_session.run_sql("EXECUTE stmt1")

#@ WL11651-TSFR19_2 - --prep-stmts
WIPE_STDOUT()

\show thread --tid <<<__test_ids['tid']>>> --prep-stmts

EXPECT_STDOUT_MATCHES(re.compile(r'| ID +| Event ID +| Name +| Owner +| Prepared in +| Executed +| Total time +| Min. time +| Avg. time +| Max. time +|'))
EXPECT_STDOUT_MATCHES(re.compile(r'| 1 +| \d+:\d+ +| stmt1 +| +|[^|]+| 3 +|[^|]+|[^|]+|[^|]+|[^|]+|'))

#@ WL11651-TSFR19_4 - When using the --prep-stmts (-P) option and thread has prepared statements, validate that the additional information is displayed in tables.
EXPECT_STDOUT_MATCHES(re.compile(r'^|-+|-+|-+|-+|-+|-+|-+|-+|-+|-+|$'))

#@ WL11651-TSFR19_2 - cleanup
__test_session.run_sql("DEALLOCATE PREPARE stmt1")

#@ WL11651-TSFR19_3 - When using the --prep-stmts (-P) option and the thread has not any prepared statements, validate that no additional information is displayed.
\show thread --tid <<<__test_ids['tid']>>> --prep-stmts

#@ WL11651-TSFR20_2 - When using the --status (-S) option, validate that the report display information about the session status variables of the thread.
\show thread --tid <<<__test_ids['tid']>>> --status

#@ WL11651-TSFR20_3 - When using the --status (-S) option and the thread has not any session status variables, validate that no additional information is displayed.
\show thread --tid <<<background_tid>>> --status

# WL11651-TSFR20_4 - When using the --status (-S) option with an argument, validate that the report displays information about the session status variables that match the specified prefix.

#@ WL11651-TSFR20_4 - one prefix
\show thread --tid <<<__test_ids['tid']>>> --status Bytes

#@ WL11651-TSFR20_4 - multiple prefixes
\show thread --tid <<<__test_ids['tid']>>> --status Select,Sort

#@ WL11651-TSFR20_4 - invalid prefix
\show thread --tid <<<__test_ids['tid']>>> --status invalidprefix

#@ WL11651-TSFR21_2 - When using the --general (-G) option, validate that the expected information is displayed by the report.
\show thread --tid <<<__test_ids['tid']>>> --general

#@ WL11651-TSFR21_3 - If the thread report is called without specifying any option, validate that the default information displayed is the information displayed when using the --general (-G) option. [USE: WL11651-TSFR21_2 - When using the --general (-G) option, validate that the expected information is displayed by the report.]
\show thread --tid <<<__test_ids['tid']>>>

#@ WL11651-TSFR22_2 - When using the --vars (-V) option, validate that the report display information about the session system variables of the thread.
\show thread --tid <<<__test_ids['tid']>>> --vars

#@ WL11651-TSFR22_3 - When using the --vars (-V) option and the thread has not any session system variables, validate that no additional information is displayed.
\show thread --tid <<<background_tid>>> --vars

# WL11651-TSFR22_4 - When using the --vars (-V) option with an argument, validate that the report displays information about the session system variables that match the specified prefix.

#@ WL11651-TSFR22_4 - one prefix
\show thread --tid <<<__test_ids['tid']>>> --vars time

#@ WL11651-TSFR22_4 - multiple prefixes
\show thread --tid <<<__test_ids['tid']>>> --vars character_set,collation

#@ WL11651-TSFR22_4 - invalid prefix
\show thread --tid <<<__test_ids['tid']>>> --vars invalidprefix

#@ set some user variables
__test_session.run_sql("SET @one_var = 1")
__test_session.run_sql("SET @one_two_var = 12")
__test_session.run_sql("SET @two_var = 2")
__test_session.run_sql("SET @three_var = 3")

#@ WL11651-TSFR23_2 - When using the --user-vars (-U) option, validate that the report display information about the user-defined variables of the thread.
\show thread --tid <<<__test_ids['tid']>>> --user-vars

#@ WL11651-TSFR23_3 - When using the --user-vars (-U) option and the thread has not any user-defined variables, validate that no additional information is displayed.
\show thread --tid <<<background_tid>>> --user-vars

# WL11651-TSFR23_4 - When using the --user-vars (-U) option with an argument, validate that the report displays information about the user-defined variables that match the specified prefix.

#@ WL11651-TSFR23_4 - one prefix
\show thread --tid <<<__test_ids['tid']>>> --user-vars one

#@ WL11651-TSFR23_4 - multiple prefixes
\show thread --tid <<<__test_ids['tid']>>> --user-vars two,three

#@ WL11651-TSFR23_4 - invalid prefix
\show thread --tid <<<__test_ids['tid']>>> --user-vars invalidprefix

#@ WL11651-TSFR24_2 - Run the thread report with the --all (-A) option, validate that the info displayed list the information as if all the options were given, skipping the output of blank options.
WIPE_STDOUT()

\show thread --tid <<<__test_ids['tid']>>> --all

EXPECT_STDOUT_CONTAINS('GENERAL')
EXPECT_STDOUT_CONTAINS('CLIENT')
EXPECT_STDOUT_NOT_CONTAINS('N/A')
EXPECT_STDOUT_NOT_CONTAINS('INNODB STATUS')
EXPECT_STDOUT_NOT_CONTAINS('LOCKS')
EXPECT_STDOUT_NOT_CONTAINS('PREPARED STATEMENTS')
EXPECT_STDOUT_CONTAINS('STATUS VARIABLES')
EXPECT_STDOUT_CONTAINS('SYSTEM VARIABLES')
EXPECT_STDOUT_CONTAINS('USER VARIABLES')

# add a prepared statement
__test_session.run_sql("PREPARE stmt2 FROM 'SELECT SQRT(POW(30,2) + POW(40,2))'")

# section on prepared statements should appear
WIPE_STDOUT()

\show thread --tid <<<__test_ids['tid']>>> --all

EXPECT_STDOUT_CONTAINS('GENERAL')
EXPECT_STDOUT_CONTAINS('CLIENT')
EXPECT_STDOUT_NOT_CONTAINS('N/A')
EXPECT_STDOUT_NOT_CONTAINS('INNODB STATUS')
EXPECT_STDOUT_NOT_CONTAINS('LOCKS')
EXPECT_STDOUT_CONTAINS('PREPARED STATEMENTS')
EXPECT_STDOUT_CONTAINS('STATUS VARIABLES')
EXPECT_STDOUT_CONTAINS('SYSTEM VARIABLES')
EXPECT_STDOUT_CONTAINS('USER VARIABLES')

__test_session.run_sql("DEALLOCATE PREPARE stmt2")

# WL11651-TSFR25_1 - Validate that the information printed by the thread report has a section per option.
# This is verified in multiple other tests.

# WL11651-TSFR25_2 - Validate that the information printed for each section is displayed in two columns, except Locks and Prepared Statements that will display the info in tables.
# This is verified in multiple other tests.

#@ WL11651-TSFR26_1 - If there is any error in the queries executed by the thread report, validate that an exception is thrown.
# revoke all privileges from the test user, it's not going to be possible to run a report
session.run_sql("REVOKE ALL PRIVILEGES, GRANT OPTION FROM 'thread_test'@'%'")
# reconnect
__test_session = mysql.get_classic_session(__test_uripwd)
# try to run a report
shell.reports.thread(__test_session, [], {'all': True})

# WL11651-TSFR27_1 - Validate that the thread report works with Server versions 5.7 or 8.0.
# Tests are run with 5.7 and 8.0 servers.

# WL11651-TSFR25_2 - Validate that any information not present in any version (5.7 or 8.0) is omitted from the report.
# Currently there is no difference in server versions, tests are run with 5.7 and 8.0 servers.

# -----------------------------------------------------------------------------
# cleanup

#@ cleanup - delete the database
session.run_sql("DROP DATABASE thread_test")

#@ cleanup - delete the user
session.run_sql("DROP USER 'thread_test'@'%'")

#@ cleanup - disconnect test session
__test_session.close()

#@ cleanup - disconnect the session
session.close()
