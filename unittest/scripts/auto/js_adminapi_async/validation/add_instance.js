//@# INCLUDE async_utils.inc
||

//@# Setup
||

//@# bad parameters (should fail)
|ERROR: Unable to connect to the target instance 'localhost:<<<__mysql_sandbox_port3>>>'. Please verify the connection settings, make sure the instance is available and try again.|
||Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
||Argument #1 is expected to be a string (TypeError)
||Argument #1 is expected to be a string (TypeError)
||Argument #1 is expected to be a string (TypeError)
||Argument #2 is expected to be a map (TypeError)
||Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
||Argument #1 is expected to be a string (TypeError)
||Argument #1 is expected to be a string (TypeError)
||Invalid options: badOption (ArgumentError)
||Argument #1 is expected to be a string (TypeError)
||Could not open connection to 'localhost:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on '<<<libmysql_host_description('localhost', __mysql_sandbox_port3)>>>'
||Invalid value '0' for option 'waitRecovery'. It must be an integer in the range [1, 3].
||Invalid value for option recoveryMethod: bogus (ArgumentError)
||Invalid value '42' for option 'waitRecovery'. It must be an integer in the range [1, 3]. (ArgumentError)
||Invalid value '42' for option 'waitRecovery'. It must be an integer in the range [1, 3]. (ArgumentError)
||Option cloneDonor only allowed if option recoveryMethod is set to 'clone'. (ArgumentError)
||Invalid value for cloneDonor, string value cannot be empty. (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in 'foobar'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in 'root@foobar:3232'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||IPv6 addresses not supported for cloneDonor (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in '::1:3232'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||Invalid value for cloneDonor: Invalid address format in '::1'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)

//@# disconnected rs object (should fail)
||The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object. (RuntimeError)

//@# bad config (should fail) {VER(<8.0.23)}
!This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>!
!+--------------------------+---------------+----------------+--------------------------------------------------+!
!| Variable                 | Current Value | Required Value | Note                                             |!
!+--------------------------+---------------+----------------+--------------------------------------------------+!
!| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |!
!| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |!
!| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |!
!+--------------------------+---------------+----------------+--------------------------------------------------+!
!Some variables need to be changed, but cannot be done dynamically on the server.!
!ERROR: <<<__endpoint_uri3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.!
!!ReplicaSet.addInstance: Instance check failed (MYSQLSH 51150

//@# bad config (should fail) {VER(>=8.0.23) && VER(<8.0.26)}
!This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>!
!+----------------------------------------+---------------+----------------+--------------------------------------------------+!
!| Variable                               | Current Value | Required Value | Note                                             |!
!+----------------------------------------+---------------+----------------+--------------------------------------------------+!
!| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |!
!| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |!
!| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |!
!| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |!
!| slave_parallel_type                    | DATABASE      | LOGICAL_CLOCK  | Update the server variable                       |!
!| slave_preserve_commit_order            | OFF           | ON             | Update the server variable                       |!
!+----------------------------------------+---------------+----------------+--------------------------------------------------+!
!Some variables need to be changed, but cannot be done dynamically on the server.!
!ERROR: <<<__endpoint_uri3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.!
!!ReplicaSet.addInstance: Instance check failed (MYSQLSH 51150

//@# bad config (should fail) {VER(==8.0.26)}
!This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>!
!+----------------------------------------+---------------+----------------+--------------------------------------------------+!
!| Variable                               | Current Value | Required Value | Note                                             |!
!+----------------------------------------+---------------+----------------+--------------------------------------------------+!
!| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |!
!| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |!
!| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |!
!| replica_parallel_type                  | DATABASE      | LOGICAL_CLOCK  | Update the server variable                       |!
!| replica_preserve_commit_order          | OFF           | ON             | Update the server variable                       |!
!| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |!
!+----------------------------------------+---------------+----------------+--------------------------------------------------+!
!Some variables need to be changed, but cannot be done dynamically on the server.!
!ERROR: <<<__endpoint_uri3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.!
!!ReplicaSet.addInstance: Instance check failed (MYSQLSH 51150

//@# bad config (should fail) {VER(>=8.0.27) && VER(<8.3.0)}
!This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>!
!+----------------------------------------+---------------+----------------+--------------------------------------------------+!
!| Variable                               | Current Value | Required Value | Note                                             |!
!+----------------------------------------+---------------+----------------+--------------------------------------------------+!
!| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |!
!| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |!
!| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |!
!| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |!
!+----------------------------------------+---------------+----------------+--------------------------------------------------+!
!Some variables need to be changed, but cannot be done dynamically on the server.!
!ERROR: <<<__endpoint_uri3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.!
!!ReplicaSet.addInstance: Instance check failed (MYSQLSH 51150

//@# bad config (should fail) {VER(>=8.3.0)}
!This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>!
!+--------------------------+---------------+----------------+--------------------------------------------------+!
!| Variable                 | Current Value | Required Value | Note                                             |!
!+--------------------------+---------------+----------------+--------------------------------------------------+!
!| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |!
!| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |!
!| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |!
!+--------------------------+---------------+----------------+--------------------------------------------------+!
!Some variables need to be changed, but cannot be done dynamically on the server.!
!ERROR: <<<__endpoint_uri3>>>: Instance must be configured and validated with dba.configureReplicaSetInstance() before it can be used in a replicaset.!
!!ReplicaSet.addInstance: Instance check failed (MYSQLSH 51150

//@# Invalid loopback ip (should fail)
|ERROR: Cannot use host '127.0.1.1' for instance '127.0.1.1:<<<__mysql_sandbox_port3>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.|
||Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported. (RuntimeError)

//@# duplicated server_id with 1 server (should fail)
||server_id in <<<__endpoint_uri2>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> already uses the same value

//@# duplicated server_uuid with 1 server (should fail)
||server_uuid in <<<__endpoint_uri2>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> already uses the same value

//@ duplicated server_id with 2 servers - same as master (should fail)
||server_id in <<<__endpoint_uri3>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> already uses the same value

//@ duplicated server_id with 2 servers - same as other (should fail)
||server_id in <<<__endpoint_uri3>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> already uses the same value

//@ duplicated server_uuid with 2 servers - same as master (should fail)
||server_uuid in <<<__endpoint_uri3>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> already uses the same value

//@ duplicated server_uuid with 2 servers - same as other (should fail)
||server_uuid in <<<__endpoint_uri3>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> already uses the same value

//@# replication filters (should fail)
||<<<__endpoint_uri3>>>: instance has global replication filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# binlog filters (should fail)
||<<<__endpoint_uri3>>>: instance has binlog filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# invalid instance (should fail)
|ERROR: Unable to connect to the target instance 'localhost:1'. Please verify the connection settings, make sure the instance is available and try again.|
||Could not open connection to 'localhost:1': Can't connect to MySQL server on '<<<libmysql_host_description('localhost', '1')>>>'

//@# admin account has mismatched passwords (should fail)
|ERROR: The administrative account credentials for localhost:<<<__mysql_sandbox_port2>>> do not match the cluster's administrative account. The cluster administrative account user name and password must be the same on all instances that belong to it.|ReplicaSet.addInstance: Could not open connection to 'localhost:<<<__mysql_sandbox_port2>>>': Access denied for user 'foo'@'localhost' (using password: YES) (MySQL Error 1045)

//@# admin account doesn't allow connection from source host (should fail)
|ERROR: The administrative account credentials for <<<hostname>>>:<<<__mysql_sandbox_port2>>> do not match the cluster's administrative account. The cluster administrative account user name and password must be the same on all instances that belong to it.|ReplicaSet.addInstance: Could not open connection to '<<<hostname>>>:<<<__mysql_sandbox_port2>>>': Access denied for user 'foo'@

//@# bad URI with a different user (should fail)
|ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them|
||Invalid target instance specification (ArgumentError)

//@# bad URI with a different password (should fail)
|ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them|
||Invalid target instance specification (ArgumentError)

//@# instance running unmanaged GR (should fail)
||group_replication active (MYSQLSH 51150)

//@# instance belongs to a rs (should fail)
||<<<__endpoint_uri2>>> is already managed by a replicaset.

//@# instance running unmanaged AR (should fail) {VER(<8.0.23)}
|Unmanaged replication channels are not supported in a replicaset. If you'd like|
|to manage an existing MySQL replication topology with the Shell, use the|
|createReplicaSet() operation with the adoptFromAR option. If the addInstance()|
|operation previously failed for the target instance and you are trying to add|
|it again, then after fixing the issue you should reset the current replication|
|settings before retrying to execute the operation. To reset the replication|
|settings on the target instance execute the following statements: 'STOP|
|SLAVE;' and 'RESET SLAVE ALL;.|
||Unexpected replication channel (MYSQLSH 51150)

//@# instance running unmanaged AR (should fail) {VER(>=8.0.22)}
|Unmanaged replication channels are not supported in a replicaset. If you'd like|
|to manage an existing MySQL replication topology with the Shell, use the|
|createReplicaSet() operation with the adoptFromAR option. If the addInstance()|
|operation previously failed for the target instance and you are trying to add|
|it again, then after fixing the issue you should reset the current replication|
|settings before retrying to execute the operation. To reset the replication|
|settings on the target instance execute the following statements: 'STOP|
|REPLICA;' and 'RESET REPLICA ALL;'.|
||Unexpected replication channel (MYSQLSH 51150)

//@# instance already in the same rs (should fail)
||<<<__endpoint_uri2>>> is already a member of this replicaset. (MYSQLSH 51301)

//@# prepare rs with 2 members and a 3rd sandbox
||

//@# add while the instance we got the rs from is down (should fail)
||The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object.

//@# add while PRIMARY down (should fail)
|ReplicaSet change operations will not be possible unless the PRIMARY can be reached.|
||PRIMARY instance is unavailable (MYSQLSH 51118)

//@# add while some secondary down
||

//@# addInstance on a URI
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@# check state of the added instance
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|view_id	cluster_id	instance_id	label	member_id	member_role	master_instance_id	master_member_id|
|3	<<<cluster_id>>>	1	<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>	5ef81566-9395-11e9-87e9-111111111111	PRIMARY	NULL	NULL|
|3	<<<cluster_id>>>	2	<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>	5ef81566-9395-11e9-87e9-222222222222	SECONDARY	1	5ef81566-9395-11e9-87e9-111111111111|
|2|
|*************************** 1. row ***************************|
|             <<<__replica_keyword_capital>>>_IO_State: Waiting for <<<__source_keyword>>> to send event|
|                  <<<__source_keyword_capital>>>_Host: <<<hostname_ip>>>|
|                  <<<__source_keyword_capital>>>_User: mysql_innodb_rs_22|
|                  <<<__source_keyword_capital>>>_Port: <<<__mysql_sandbox_port1>>>|
|                Connect_Retry: 60|
|              <<<__source_keyword_capital>>>_Log_File: |
|          Read_<<<__source_keyword_capital>>>_Log_Pos: |
|               Relay_Log_File: |
|                Relay_Log_Pos: |
|        Relay_<<<__source_keyword_capital>>>_Log_File: |
|           <<<__replica_keyword_capital>>>_IO_Running: Yes|
|          <<<__replica_keyword_capital>>>_SQL_Running: Yes|
|              Replicate_Do_DB: |
|          Replicate_Ignore_DB: |
|           Replicate_Do_Table: |
|       Replicate_Ignore_Table: |
|      Replicate_Wild_Do_Table: |
|  Replicate_Wild_Ignore_Table: |
|                   Last_Errno: 0|
|                   Last_Error: |
|                 Skip_Counter: 0|
|          Exec_<<<__source_keyword_capital>>>_Log_Pos: |
|              Relay_Log_Space: |
|              Until_Condition: None|
|               Until_Log_File: |
|                Until_Log_Pos: 0|
|           <<<__source_keyword_capital>>>_SSL_Allowed: Yes|
|           <<<__source_keyword_capital>>>_SSL_CA_File: |
|           <<<__source_keyword_capital>>>_SSL_CA_Path: |
|              <<<__source_keyword_capital>>>_SSL_Cert: |
|            <<<__source_keyword_capital>>>_SSL_Cipher: |
|               <<<__source_keyword_capital>>>_SSL_Key: |
|        Seconds_Behind_<<<__source_keyword_capital>>>: |
|<<<__source_keyword_capital>>>_SSL_Verify_Server_Cert: No|
|                Last_IO_Errno: 0|
|                Last_IO_Error: |
|               Last_SQL_Errno: 0|
|               Last_SQL_Error: |
|  Replicate_Ignore_Server_Ids: |
|             <<<__source_keyword_capital>>>_Server_Id: 11|
|                  <<<__source_keyword_capital>>>_UUID: 5ef81566-9395-11e9-87e9-111111111111|
|             <<<__source_keyword_capital>>>_Info_File: mysql.slave_master_info|
|                    SQL_Delay: 0|
|          SQL_Remaining_Delay: NULL|
|    <<<__replica_keyword_capital>>>_SQL_Running_State: <<<__replica_keyword_capital>>> has read all relay log; waiting for more updates|
|           <<<__source_keyword_capital>>>_Retry_Count: <<<(__version_num<80100)?86400:10>>>|
|                  <<<__source_keyword_capital>>>_Bind: |
|      Last_IO_Error_Timestamp: |
|     Last_SQL_Error_Timestamp: |
|               <<<__source_keyword_capital>>>_SSL_Crl: |
|           <<<__source_keyword_capital>>>_SSL_Crlpath: |
|           Retrieved_Gtid_Set: 5ef81566-9395-11e9-87e9-111111111111:1-|
|            Executed_Gtid_Set: 5ef81566-9395-11e9-87e9-111111111111:1-|
|                Auto_Position: 1|
|         Replicate_Rewrite_DB: |
|                 Channel_Name: |
|           <<<__source_keyword_capital>>>_TLS_Version: |
|       <<<__source_keyword_capital>>>_public_key_path: |
|        Get_<<<__source_keyword_capital>>>_public_key: 0|
|            Network_Namespace: |

//@# target is super-read-only
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@# target is read-only
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@# dryRun - 1
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|Instance configuration is suitable.|
|* Checking async replication topology...|
|* Checking transaction state of the instance...|
|* Updating topology|
|** Changing replication source of <<<__endpoint_uri2>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|** Waiting for new instance to synchronize with PRIMARY...|
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|dryRun finished.|

//@# dryRun - 2
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|Instance configuration is suitable.|
|* Checking async replication topology...|
|* Checking transaction state of the instance...|
|* Updating topology|
|** Changing replication source of <<<__endpoint_uri2>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|** Waiting for new instance to synchronize with PRIMARY...|
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|dryRun finished.|

//@# label
|instance_name|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|blargh|

//@# timeout -1 {__dbug}
||

//@# timeout 2 and rollback (should fail) {__dbug}
|Reverting topology changes...|
||Timeout reached waiting for transactions from <<<__endpoint_uri1>>> to be applied on instance '<<<__endpoint_uri2>>>' (MYSQLSH 51157)
|{|
|    "replicaSet": {|
|        "name": "myrs", |
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|        "status": "AVAILABLE", |
|        "statusText": "All instances available.", |
|        "topology": {|
|            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|                "instanceRole": "PRIMARY", |
|                "mode": "R/W", |
|                "status": "ONLINE"|
|            }|
|        }, |
|        "type": "ASYNC"|
|    }|
|}|

//@# timeout 10 {__dbug}
||

//@# timeout 0 {__dbug}
||

//@# rebuild test setup
||

//@# Replication conflict error (should fail) {VER(<8.0.23)}
|ERROR: Applier error in replication channel '': Error 'Can't create database 'testdb'; database exists' on query. Default database: 'testdb'. Query: 'CREATE SCHEMA testdb' (1007) at [[*]]|
||<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Error found in replication applier thread (MYSQLSH 51145)

//@# Replication conflict error (should fail) {VER(>=8.0.23)}
|ERROR: Coordinator error in replication channel '': Coordinator stopped because there were error(s) in the worker(s). The most recent failure being: Worker 1 failed executing transaction [[*]]|
||<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Error found in replication coordinator thread (MYSQLSH 51144)

//@# instance has more GTIDs (should fail)
|WARNING: A GTID set check of the MySQL instance at '<<<__endpoint_uri2>>>' determined that|
|it contains transactions that do not originate from the replicaset, which must|
|be discarded before it can join the replicaset.|
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (MYSQLSH 51166)

//@ instance has a subset of the master GTID set
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ instance has more GTIDs (should work with clone) {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
|Monitoring Clone based state recovery of the new member. Press ^C to abort the operation.|
|Clone based state recovery is now in progress.|
|* Waiting for clone to finish...|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is being cloned from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@# master has purged GTIDs (should fail)
|NOTE: A GTID set check of the MySQL instance at '<<<__endpoint_uri2>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
||Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (MYSQLSH 51166)

//@ master has purged GTIDs (should work with clone) {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
|Monitoring Clone based state recovery of the new member. Press ^C to abort the operation.|
|Clone based state recovery is now in progress.|
|* Waiting for clone to finish...|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is being cloned from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ Re-create the replicaset without gtidSetIsComplete
||

//@# instance has empty GTID set + gtidSetIsComplete:0 + not interactive (should fail)
|NOTE: The target instance '<<<__endpoint_uri2>>>' has not been pre-provisioned (GTID set|
|is empty). The Shell is unable to decide whether replication can completely|
|recover its state.|
//@# instance has empty GTID set + gtidSetIsComplete:0 + not interactive (should fail) {VER(>=8.0.17)}
||'recoveryMethod' option must be set to 'clone' or 'incremental' (MYSQLSH 51167)
//@# instance has empty GTID set + gtidSetIsComplete:0 + not interactive (should fail) {VER(<8.0.17)}
||'recoveryMethod' option must be set to 'incremental' (MYSQLSH 51168)

//@# instance has empty GTID set + gtidSetIsComplete:0 prompt-no (should fail)
|NOTE: The target instance '<<<__endpoint_uri2>>>' has not been pre-provisioned (GTID set|
||Cancelled

//@# instance has empty GTID set + gtidSetIsComplete:0 prompt-yes
|NOTE: The target instance '<<<__endpoint_uri2>>>' has not been pre-provisioned (GTID set|
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@# instance has empty GTID set + gtidSetIsComplete:0 + recoveryMethod:INCREMENTAL
|NOTE: The target instance '<<<__endpoint_uri2>>>' has not been pre-provisioned (GTID set|
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@# instance has empty GTID set + gtidSetIsComplete:0 + recoveryMethod:CLONE {VER(>=8.0.17)}
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' has not been pre-provisioned (GTID set is|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@# instance has a subset of the master GTID set + gtidSetIsComplete:0
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ cloneDonor invalid: not a ReplicaSet member {VER(>=8.0.17)}
||Instance <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> does not belong to the replicaset (MYSQLSH 51310)

//@ cloneDonor valid {VER(>=8.0.17)}
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is being cloned from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ cloneDonor valid 2 {VER(>=8.0.17)}
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is being cloned from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ BUG#30628746: wait for timeout {VER(>=8.0.17)}
|* Waiting for the donor to synchronize with PRIMARY...|
|ERROR: The donor instance failed to synchronize its transaction set with the PRIMARY.|
||Timeout reached waiting for transactions from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> to be applied on instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51157)

//@ BUG#30628746: donor primary should not error with timeout {VER(>=8.0.17)}
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ BUG#30281908: add instance using clone and simulating a restart timeout {VER(>= 8.0.17)}
|Reverting topology changes...|
||Timeout waiting for server to restart (MYSQLSH 51156)

//@# Cleanup
||
