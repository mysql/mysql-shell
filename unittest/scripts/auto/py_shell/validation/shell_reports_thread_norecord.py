#@ create a new session
|<ClassicSession:<<<__mysql_uri>>>>|

#@ create test user
||

#@ create test connection
||

#@ get IDs
||

#@ create database
||

#@ WL11651-TSFR12_1 - Validate that a new report named thread is available in the Shell.
|, thread,|

#@ WL11651-TSFR12_2 - Run the thread report without giving any options, validate that the report print information about the thread used by the current connection.
|Thread ID:                <<<__session_ids['tid']>>>|
|Connection ID:            <<<__session_ids['cid']>>>|

#@ display help for thread report
||

#@ WL11651-TSFR13_2 - --tid
|Thread ID:                <<<__test_ids['tid']>>>|
|Connection ID:            <<<__test_ids['cid']>>>|

#@ WL11651-TSFR13_X - Invalid thread ID
||reports.thread: The specified thread does not exist.

#@ WL11651-TSFR14_3 - tid + cid
||reports.thread: Both 'cid' and 'tid' cannot be used at the same time.

#@<OUT> WL11651-TSFR15_2 - --brief
Thread ID: <<<__test_ids['tid']>>>, Connection ID: <<<__test_ids['cid']>>>, User: thread_test@<<<__host>>>.

#@ WL11651-TSFR16_2 - setup
||

#@<OUT> WL11651-TSFR16_2 - --client
CLIENT
_client_name:             libmysql
_client_version:          [[*]]
_os:                      [[*]]
_pid:                     [[*]]
_platform:                [[*]]
?{__os_type=='windows'}
_thread:                  [[*]]
?{}
program_name:             mysqlsh
Protocol:                 classic
Socket IP:                [[*]]
Socket port:              [[*]]
Socket state:             IDLE
Socket stats:             NULL
Compression:              <<<compression>>>
SSL cipher:               <<<ssl_cipher>>>
SSL version:              <<<ssl_version>>>

#@ WL11651-TSFR16_3 - setup
||

#@<OUT> WL11651-TSFR16_3 - --client
CLIENT
_client_name:             libmysql
_client_version:          [[*]]
_os:                      [[*]]
_pid:                     [[*]]
_platform:                [[*]]
?{__os_type=='windows'}
_thread:                  [[*]]
?{}
program_name:             mysqlsh
Protocol:                 classic
Socket IP:                [[*]]
Socket port:              [[*]]
Socket state:             IDLE
Socket reads:             [[*]]
Read bytes:               [[*]]
Total read wait:          [[*]]
Min. read wait:           [[*]]
Avg. read wait:           [[*]]
Max. read wait:           [[*]]
Socket writes:            [[*]]
Wrote bytes:              [[*]]
Total write wait:         [[*]]
Min. write wait:          [[*]]
Avg. write wait:          [[*]]
Max. write wait:          [[*]]
Compression:              <<<compression>>>
SSL cipher:               <<<ssl_cipher>>>
SSL version:              <<<ssl_version>>>

#@<OUT> WL11651-TSFR16_4 - When using the --client (-C) option and a background thread, validate that no information is provided.
CLIENT
N/A

#@ WL11651-TSFR17_2 - setup
||

#@<OUT> WL11651-TSFR17_2 - --innodb
INNODB STATUS
State:                    RUNNING
ID:                       [[*]]
Elapsed:                  [[*]]
Started:                  [[*]]
Isolation level:          REPEATABLE READ
Access:                   READ WRITE
Locked tables:            1
Locked rows:              2
Modified rows:            1

#@ WL11651-TSFR17_2 - cleanup
||

#@<OUT> WL11651-TSFR17_X - When there's no transaction information is not available.
INNODB STATUS
N/A

#@ WL11651-TSFR18_2 - setup
||

#@ WL11651-TSFR18_2 - --locks - blocking
||

#@ WL11651-TSFR18_4 - When using the --locks (-L) option and thread has lock information, validate that the additional information is displayed in tables. (1)
||

#@ WL11651-TSFR18_2 - --locks - blocked
||

#@ WL11651-TSFR18_4 - When using the --locks (-L) option and thread has lock information, validate that the additional information is displayed in tables. (2)
||

#@ WL11651-TSFR7_5 - Validate that all the columns listed in FR5 can be requested using the --format (-o) option. - nblocked, nblocking - metadata
||

#@ WL11651-TSFR18_2 - cleanup
||

#@ WL11651-TSFR18_2 - setup - InnoDB
||

#@ WL11651-TSFR18_2 - --locks - blocking - InnoDB
||

#@ WL11651-TSFR18_4 - When using the --locks (-L) option and thread has lock information, validate that the additional information is displayed in tables. (3)
||

#@ WL11651-TSFR18_2 - --locks - blocked - InnoDB
||

#@ WL11651-TSFR18_4 - When using the --locks (-L) option and thread has lock information, validate that the additional information is displayed in tables. (4)
||

#@ WL11651-TSFR7_5 - Validate that all the columns listed in FR5 can be requested using the --format (-o) option. - nblocked, nblocking - InnoDB
||

#@ WL11651-TSFR18_2 - cleanup - InnoDB
||

#@<OUT> WL11651-TSFR18_3 - When using the --locks (-L) option and the thread has not any locks, validate that no additional information is displayed.
LOCKS
Waiting for InnoDB locks
N/A

Waiting for metadata locks
N/A

Blocking InnoDB locks
N/A

Blocking metadata locks
N/A

#@ WL11651-TSFR19_2 - setup
||

#@ WL11651-TSFR19_2 - --prep-stmts
||

#@ WL11651-TSFR19_4 - When using the --prep-stmts (-P) option and thread has prepared statements, validate that the additional information is displayed in tables.
||

#@ WL11651-TSFR19_2 - cleanup
||

#@<OUT> WL11651-TSFR19_3 - When using the --prep-stmts (-P) option and the thread has not any prepared statements, validate that no additional information is displayed.
PREPARED STATEMENTS
N/A

#@ WL11651-TSFR20_2 - When using the --status (-S) option, validate that the report display information about the session status variables of the thread.
|Bytes_received:|
|Bytes_sent:|
|Table_open_cache_overflows:|

#@<OUT> WL11651-TSFR20_3 - When using the --status (-S) option and the thread has not any session status variables, validate that no additional information is displayed.
STATUS VARIABLES
N/A

#@<OUT> WL11651-TSFR20_4 - one prefix
STATUS VARIABLES
Bytes_received:           [[*]]
Bytes_sent:               [[*]]

#@<OUT> WL11651-TSFR20_4 - multiple prefixes
STATUS VARIABLES
Select_full_join:         [[*]]
Select_full_range_join:   [[*]]
Select_range:             [[*]]
Select_range_check:       [[*]]
Select_scan:              [[*]]
Sort_merge_passes:        [[*]]
Sort_range:               [[*]]
Sort_rows:                [[*]]
Sort_scan:                [[*]]

#@<OUT> WL11651-TSFR20_4 - invalid prefix
STATUS VARIABLES
N/A

#@<OUT> WL11651-TSFR21_2 - When using the --general (-G) option, validate that the expected information is displayed by the report.
GENERAL
Thread ID:                <<<__test_ids['tid']>>>
Connection ID:            <<<__test_ids['cid']>>>
Thread type:              FOREGROUND
Program name:             mysqlsh
User:                     thread_test
Host:                     localhost
Database:                 NULL
Command:                  Sleep
Time:                     [[*]]
State:                    NULL
Transaction state:        NULL
Prepared statements:      0
Bytes received:           [[*]]
Bytes sent:               [[*]]
Info:                     [[*]]
Previous statement:       DEALLOCATE PREPARE stmt1

#@ WL11651-TSFR22_2 - When using the --vars (-V) option, validate that the report display information about the session system variables of the thread.
|auto_increment_increment:|
|auto_increment_offset:|
|warning_count:|

#@<OUT> WL11651-TSFR22_3 - When using the --vars (-V) option and the thread has not any session system variables, validate that no additional information is displayed.
SYSTEM VARIABLES
N/A

#@<OUT> WL11651-TSFR22_4 - one prefix
SYSTEM VARIABLES
time_zone:                [[*]]
timestamp:                [[*]]

#@<OUT> WL11651-TSFR22_4 - multiple prefixes
SYSTEM VARIABLES
character_set_client:     [[*]]
character_set_connection: [[*]]
character_set_database:   [[*]]
character_set_filesystem: [[*]]
character_set_results:    [[*]]
character_set_server:     [[*]]
collation_connection:     [[*]]
collation_database:       [[*]]
collation_server:         [[*]]

#@<OUT> WL11651-TSFR22_4 - invalid prefix
SYSTEM VARIABLES
N/A

#@ set some user variables
||

#@<OUT> WL11651-TSFR23_2 - When using the --user-vars (-U) option, validate that the report display information about the user-defined variables of the thread.
USER VARIABLES
one_two_var:              12
one_var:                  1
three_var:                3
two_var:                  2

#@<OUT> WL11651-TSFR23_3 - When using the --user-vars (-U) option and the thread has not any user-defined variables, validate that no additional information is displayed.
USER VARIABLES
N/A

#@<OUT> WL11651-TSFR23_4 - one prefix
USER VARIABLES
one_two_var:              12
one_var:                  1

#@<OUT> WL11651-TSFR23_4 - multiple prefixes
USER VARIABLES
three_var:                3
two_var:                  2

#@<OUT> WL11651-TSFR23_4 - invalid prefix
USER VARIABLES
N/A

#@ WL11651-TSFR24_2 - Run the thread report with the --all (-A) option, validate that the info displayed list the information as if all the options were given, skipping the output of blank options.
||

#@ WL11651-TSFR26_1 - If there is any error in the queries executed by the thread report, validate that an exception is thrown.
||reports.thread: SELECT command denied to user 'thread_test'@'<<<__host>>>' for table 'threads'

#@ cleanup - delete the database
||

#@ cleanup - delete the user
||

#@ cleanup - disconnect test session
||

#@ cleanup - disconnect the session
||
