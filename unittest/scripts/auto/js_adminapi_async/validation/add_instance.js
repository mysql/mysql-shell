//@# INCLUDE async_utils.inc
||

//@# Setup
||

//@# bad parameters (should fail)
|ERROR: Unable to connect to the target instance 'localhost:<<<__mysql_sandbox_port3>>>'. Please verify the connection settings, make sure the instance is available and try again.|
||ReplicaSet.addInstance: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
||ReplicaSet.addInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.addInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.addInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.addInstance: Argument #2 is expected to be a map (ArgumentError)
||ReplicaSet.addInstance: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
||ReplicaSet.addInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.addInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.addInstance: Invalid options: badOption (ArgumentError)
||ReplicaSet.addInstance: Argument #1 is expected to be a string (ArgumentError)
||ReplicaSet.addInstance: Could not open connection to 'localhost:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on 'localhost'
||ReplicaSet.addInstance: Invalid value '0' for option 'waitRecovery'. It must be an integer in the range [1, 3].
||ReplicaSet.addInstance: Invalid value for option recoveryMethod: bogus (ArgumentError)
||ReplicaSet.addInstance: Invalid value '42' for option 'waitRecovery'. It must be an integer in the range [1, 3]. (ArgumentError)
||ReplicaSet.addInstance: Invalid value '42' for option 'waitRecovery'. It must be an integer in the range [1, 3]. (ArgumentError)
||ReplicaSet.addInstance: Option cloneDonor only allowed if option recoveryMethod is set to 'clone'. (ArgumentError)
||ReplicaSet.addInstance: Invalid value for cloneDonor, string value cannot be empty. (ArgumentError)
||ReplicaSet.addInstance: Invalid value for cloneDonor: Invalid address format in 'foobar'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||ReplicaSet.addInstance: Invalid value for cloneDonor: Invalid address format in 'root@foobar:3232'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||ReplicaSet.addInstance: IPv6 addresses not supported for cloneDonor (ArgumentError)
||ReplicaSet.addInstance: Invalid value for cloneDonor: Invalid address format in '::1:3232'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)
||ReplicaSet.addInstance: Invalid value for cloneDonor: Invalid address format in '::1'. Must be <host>:<port> or [<ip>]:<port> for IPv6 addresses (ArgumentError)

//@# disconnected rs object (should fail)
||ReplicaSet.addInstance: The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object. (RuntimeError)

//@# bad config (should fail)
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
||ReplicaSet.addInstance: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported. (RuntimeError)

//@# duplicated server_id with 1 server (should fail)
||ReplicaSet.addInstance: server_id in <<<__endpoint_uri2>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> already uses the same value

//@# duplicated server_uuid with 1 server (should fail)
||ReplicaSet.addInstance: server_uuid in <<<__endpoint_uri2>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> already uses the same value

//@ duplicated server_id with 2 servers - same as master (should fail)
||ReplicaSet.addInstance: server_id in <<<__endpoint_uri3>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> already uses the same value

//@ duplicated server_id with 2 servers - same as other (should fail)
||ReplicaSet.addInstance: server_id in <<<__endpoint_uri3>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> already uses the same value

//@ duplicated server_uuid with 2 servers - same as master (should fail)
||ReplicaSet.addInstance: server_uuid in <<<__endpoint_uri3>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> already uses the same value

//@ duplicated server_uuid with 2 servers - same as other (should fail)
||ReplicaSet.addInstance: server_uuid in <<<__endpoint_uri3>>> is expected to be unique, but <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> already uses the same value

//@# replication filters (should fail)
||ReplicaSet.addInstance: <<<__endpoint_uri3>>>: instance has global replication filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# binlog filters (should fail)
||ReplicaSet.addInstance: <<<__endpoint_uri3>>>: instance has binlog filters configured, but they are not supported in InnoDB ReplicaSets. (MYSQLSH 51150)

//@# invalid instance (should fail)
|ERROR: Unable to connect to the target instance 'localhost:1'. Please verify the connection settings, make sure the instance is available and try again.|
||ReplicaSet.addInstance: Could not open connection to 'localhost:1': Can't connect to MySQL server on 'localhost'

//@# admin account has mismatched passwords (should fail)
|ERROR: Unable to connect to the target instance 'localhost:<<<__mysql_sandbox_port2>>>'. Please verify the connection settings, make sure the instance is available and try again.|ReplicaSet.addInstance: Could not open connection to 'localhost:<<<__mysql_sandbox_port2>>>': Access denied for user 'foo'@'localhost' (using password: YES) (MySQL Error 1045)

//@# admin account doesn't allow connection from source host (should fail)
|ERROR: Unable to connect to the target instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'. Please verify the connection settings, make sure the instance is available and try again.|ReplicaSet.addInstance: Could not open connection to '<<<hostname>>>:<<<__mysql_sandbox_port2>>>': Access denied for user 'foo'@

//@# bad URI with a different user (should fail)
|ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them.|
||ReplicaSet.addInstance: Invalid target instance specification (ArgumentError)

//@# bad URI with a different password (should fail)
|ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them.|
||ReplicaSet.addInstance: Invalid target instance specification (ArgumentError)

//@# instance running unmanaged GR (should fail)
||ReplicaSet.addInstance: group_replication active (MYSQLSH 51150)

//@# instance belongs to a rs (should fail)
||ReplicaSet.addInstance: <<<__endpoint_uri2>>> is already managed by a replicaset.

//@# instance running unmanaged AR (should fail)
//@# instance running unmanaged AR (should fail)
|Unmanaged replication channels are not supported in a replicaset. If you'd like|
|to manage an existing MySQL replication topology with the Shell, use the|
|createReplicaSet() operation with the adoptFromAR option. If the addInstance()|
|operation previously failed for the target instance and you are trying to add|
|it again, then after fixing the issue you should reset the current replication|
|settings before retrying to execute the operation. To reset the replication|
|settings on the target instance execute the following statements: 'STOP SLAVE'|
|and 'RESET SLAVE ALL'.|
||ReplicaSet.addInstance: Unexpected replication channel (MYSQLSH 51150)

//@# instance already in the same rs (should fail)
||ReplicaSet.addInstance: <<<__endpoint_uri2>>> is already a member of this replicaset. (MYSQLSH 51301)

//@# prepare rs with 2 members and a 3rd sandbox
||

//@# add while the instance we got the rs from is down (should fail)
||ReplicaSet.addInstance: Failed to execute query on Metadata server <<<__endpoint_uri1>>>: Lost connection to MySQL server during query (MySQL Error 2013)

//@# add while PRIMARY down (should fail)
|Cluster change operations will not be possible unless the PRIMARY can be reached.|
||ReplicaSet.addInstance: PRIMARY instance is unavailable (MYSQLSH 51118)

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
|               Slave_IO_State: Waiting for master to send event|
|                  Master_Host: <<<hostname_ip>>>|
|                  Master_User: mysql_innodb_rs_22|
|                  Master_Port: <<<__mysql_sandbox_port1>>>|
|                Connect_Retry: 60|
|              Master_Log_File: |
|          Read_Master_Log_Pos: |
|               Relay_Log_File: |
|                Relay_Log_Pos: |
|        Relay_Master_Log_File: |
|             Slave_IO_Running: Yes|
|            Slave_SQL_Running: Yes|
|              Replicate_Do_DB: |
|          Replicate_Ignore_DB: |
|           Replicate_Do_Table: |
|       Replicate_Ignore_Table: |
|      Replicate_Wild_Do_Table: |
|  Replicate_Wild_Ignore_Table: |
|                   Last_Errno: 0|
|                   Last_Error: |
|                 Skip_Counter: 0|
|          Exec_Master_Log_Pos: |
|              Relay_Log_Space: |
|              Until_Condition: None|
|               Until_Log_File: |
|                Until_Log_Pos: 0|
|           Master_SSL_Allowed: No|
|           Master_SSL_CA_File: |
|           Master_SSL_CA_Path: |
|              Master_SSL_Cert: |
|            Master_SSL_Cipher: |
|               Master_SSL_Key: |
|        Seconds_Behind_Master: 0|
|Master_SSL_Verify_Server_Cert: No|
|                Last_IO_Errno: 0|
|                Last_IO_Error: |
|               Last_SQL_Errno: 0|
|               Last_SQL_Error: |
|  Replicate_Ignore_Server_Ids: |
|             Master_Server_Id: 11|
|                  Master_UUID: 5ef81566-9395-11e9-87e9-111111111111|
|             Master_Info_File: mysql.slave_master_info|
|                    SQL_Delay: 0|
|          SQL_Remaining_Delay: NULL|
|      Slave_SQL_Running_State: Slave has read all relay log; waiting for more updates|
|           Master_Retry_Count: 86400|
|                  Master_Bind: |
|      Last_IO_Error_Timestamp: |
|     Last_SQL_Error_Timestamp: |
|               Master_SSL_Crl: |
|           Master_SSL_Crlpath: |
|           Retrieved_Gtid_Set: 5ef81566-9395-11e9-87e9-111111111111:1-|
|            Executed_Gtid_Set: 5ef81566-9395-11e9-87e9-111111111111:1-|
|                Auto_Position: 1|
|         Replicate_Rewrite_DB: |
|                 Channel_Name: |
|           Master_TLS_Version: |
|       Master_public_key_path: |
|        Get_master_public_key: 1|
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
|** Configuring <<<__endpoint_uri2>>> to replicate from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|** Waiting for new instance to synchronize with PRIMARY...|
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|dryRun finished.|

//@# dryRun - 2
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|Instance configuration is suitable.|
|* Checking async replication topology...|
|* Checking transaction state of the instance...|
|* Updating topology|
|** Configuring <<<__endpoint_uri2>>> to replicate from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
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
||ReplicaSet.addInstance: Timeout reached waiting for transactions from <<<__endpoint_uri1>>> to be applied on instance '<<<__endpoint_uri2>>>' (MYSQLSH 51157)
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

//@# Replication conflict error (should fail)
|ERROR: Applier error in replication channel '': Error 'Can't create database 'testdb'; database exists' on query. Default database: 'testdb'. Query: 'CREATE SCHEMA testdb' (1007) at [[*]]|
||ReplicaSet.addInstance: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Error found in replication applier thread (MYSQLSH 51145)

//@# instance has more GTIDs (should fail)
|WARNING: A GTID set check of the MySQL instance at '<<<__endpoint_uri2>>>' determined that|
|it contains transactions that do not originate from the replicaset, which must|
|be discarded before it can join the replicaset.|
||ReplicaSet.addInstance: Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)

//@ instance has a subset of the master GTID set
|The instance '<<<__endpoint_uri2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ instance has more GTIDs (should work with clone) {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
|Waiting for clone process of the new member to complete. Press ^C to abort the operation.|
|* Waiting for clone to finish...|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is being cloned from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@# master has purged GTIDs (should fail)
|NOTE: A GTID set check of the MySQL instance at '<<<__endpoint_uri2>>>' determined that it|
|is missing transactions that were purged from all replicaset members.|
||ReplicaSet.addInstance: Cannot use recoveryMethod=incremental option because the GTID state is not compatible or cannot be recovered. (RuntimeError)

//@ master has purged GTIDs (should work with clone) {VER(>=8.0.17)}
|Clone based recovery selected through the recoveryMethod option|
|Waiting for clone process of the new member to complete. Press ^C to abort the operation.|
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
||ReplicaSet.addInstance: 'recoveryMethod' option must be set to 'clone' or 'incremental' (RuntimeError)
//@# instance has empty GTID set + gtidSetIsComplete:0 + not interactive (should fail) {VER(<8.0.17)}
||ReplicaSet.addInstance: 'recoveryMethod' option must be set to 'incremental'

//@# instance has empty GTID set + gtidSetIsComplete:0 prompt-no (should fail)
|NOTE: The target instance '<<<__endpoint_uri2>>>' has not been pre-provisioned (GTID set|
||ReplicaSet.addInstance: Cancelled

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
||ReplicaSet.addInstance: Instance <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> does not belong to the replicaset (MYSQLSH 51310)

//@ cloneDonor valid {VER(>=8.0.17)}
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is being cloned from <<<__sandbox1>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ cloneDonor valid 2 {VER(>=8.0.17)}
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is being cloned from <<<__sandbox2>>>|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ BUG#30628746: wait for timeout {VER(>=8.0.17)}
|* Waiting for the donor to synchronize with PRIMARY...|
|ERROR: The donor instance failed to synchronize its transaction set with the PRIMARY.|
||ReplicaSet.addInstance: Timeout reached waiting for transactions from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> to be applied on instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51157)

//@ BUG#30628746: donor primary should not error with timeout {VER(>=8.0.17)}
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|

//@ BUG#30281908: add instance using clone and simulating a restart timeout {VER(>= 8.0.17)}
|Reverting topology changes...|
||ReplicaSet.addInstance: Timeout waiting for server to restart (MYSQLSH 51156)

//@# Cleanup
||
