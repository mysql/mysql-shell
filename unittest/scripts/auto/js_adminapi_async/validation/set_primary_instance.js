//@ INCLUDE async_utils.inc
||

//@# Setup
||

//@<ERR> bad parameters (should fail)
ReplicaSet.setPrimaryInstance: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
ReplicaSet.setPrimaryInstance: Argument #1 is expected to be a string (TypeError)
ReplicaSet.setPrimaryInstance: Argument #1 is expected to be a string (TypeError)
ReplicaSet.setPrimaryInstance: Argument #1 is expected to be a string (TypeError)
ReplicaSet.setPrimaryInstance: Argument #2 is expected to be a map (TypeError)
ReplicaSet.setPrimaryInstance: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
ReplicaSet.setPrimaryInstance: Argument #1 is expected to be a string (TypeError)
ReplicaSet.setPrimaryInstance: Argument #1 is expected to be a string (TypeError)
ReplicaSet.setPrimaryInstance: Argument #2: Invalid options: badOption (ArgumentError)
ReplicaSet.setPrimaryInstance: Argument #1 is expected to be a string (TypeError)

//@<ERR> disconnected rs object (should fail)
ReplicaSet.setPrimaryInstance: The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object. (RuntimeError)

//@<ERR> promoted isn't member (should fail)
ReplicaSet.setPrimaryInstance: Target instance localhost:<<<__mysql_sandbox_port3>>> is not a managed instance. (MYSQLSH 51300)

//@# promoted doesn't exist (should fail)
||Could not open connection to 'localhost:<<<__mysql_sandbox_port3>>>1': Can't connect to MySQL server on '<<<libmysql_host_description('localhost', "" + __mysql_sandbox_port3 + "1")>>>'

//@# bad target with a different user (should fail)
|ERROR: Target instance must be given as host:port. Credentials will be taken from the main session and, if given, must match them|
||Invalid target instance specification (ArgumentError)

//@# bad target with a different password (should fail)
||Invalid target instance specification (ArgumentError)

//@# bad target but allowed for compatibility
|Target instance <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is already the PRIMARY.|

//@# add 3rd instance
||

//@<OUT> check state of instances after switch
<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> will be promoted to PRIMARY of 'myrs'.
The current PRIMARY is <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.

* Connecting to replicaset instances
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>

* Performing validation checks
** Checking async replication topology...
** Checking transaction state of the instance...

* Synchronizing transaction backlog at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>


* Updating metadata

* Acquiring locks in replicaset instances
** Pre-synchronizing SECONDARIES
** Acquiring global lock at PRIMARY
** Acquiring global lock at SECONDARIES

* Updating replication topology
** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>

<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> was promoted to PRIMARY.

view_id	cluster_id	instance_id	label	member_id	member_role	master_instance_id	master_member_id
5	[[*]]	1	<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>	5ef81566-9395-11e9-87e9-111111111111	SECONDARY	2	5ef81566-9395-11e9-87e9-222222222222
5	[[*]]	2	<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>	5ef81566-9395-11e9-87e9-222222222222	PRIMARY	NULL	NULL
5	[[*]]	3	<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>	5ef81566-9395-11e9-87e9-333333333333	SECONDARY	2	5ef81566-9395-11e9-87e9-222222222222
3
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,'>>>
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "replicationLag": [[*]]
                },
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,'>>>
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "replicationLag": [[*]]
                },
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}
0

//@<OUT> check state of instances after switch - view change records
cluster_id	view_id	topology_type	view_change_reason	view_change_time	view_change_info	attributes
<<<cluster_id>>>	1	SINGLE-PRIMARY-TREE	CREATE	[[*]]	{"user": "root@localhost", "source": "5ef81566-9395-11e9-87e9-111111111111"}	{}
<<<cluster_id>>>	2	SINGLE-PRIMARY-TREE	ADD_INSTANCE	[[*]]	{"user": "root@localhost", "source": "5ef81566-9395-11e9-87e9-111111111111"}	{}
<<<cluster_id>>>	3	SINGLE-PRIMARY-TREE	ADD_INSTANCE	[[*]]	{"user": "<<<userhost>>>", "source": "5ef81566-9395-11e9-87e9-111111111111"}	{}
<<<cluster_id>>>	4	SINGLE-PRIMARY-TREE	ADD_INSTANCE	[[*]]	{"user": "<<<userhost>>>", "source": "5ef81566-9395-11e9-87e9-111111111111"}	{}
<<<cluster_id>>>	5	SINGLE-PRIMARY-TREE	SWITCH_ACTIVE	[[*]]	{"user": "<<<userhost>>>", "source": "5ef81566-9395-11e9-87e9-111111111111"}	{}
5

//@# promote back
|The current PRIMARY is <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> was promoted to PRIMARY.|

//@# primary is super-read-only (error ok)
||Replication or configuration errors at <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> (MYSQLSH 51131)

//@<OUT> promoted is already primary
Target instance <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is already the PRIMARY.

//@# promote via URL
|The current PRIMARY is <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> was promoted to PRIMARY.|

//@<OUT> dryRun
<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> will be promoted to PRIMARY of 'myrs'.
The current PRIMARY is <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.

* Connecting to replicaset instances
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>
** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>

* Performing validation checks
** Checking async replication topology...
** Checking transaction state of the instance...

* Synchronizing transaction backlog at <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
* Updating metadata

* Acquiring locks in replicaset instances
** Pre-synchronizing SECONDARIES
** Acquiring global lock at PRIMARY
** Acquiring global lock at SECONDARIES

* Updating replication topology
** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>
** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>

<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> was promoted to PRIMARY.

dryRun finished.

//@# timeout (should fail)
||Timeout reached waiting for transactions from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> to be applied on instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' (MYSQLSH 51157)

//@ try to switch to a different one - (should fail because sb2 wont sync)
|The current PRIMARY is <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: GTID sync failed: MYSQLSH 51157: Timeout waiting for replica to synchronize|
|ERROR: An error occurred while preparing replicaset instances for a PRIMARY switch: 1 SECONDARY instance(s) failed to synchronize|
||1 SECONDARY instance(s) failed to synchronize (MYSQLSH 51160)

//@# Runtime problems
||

//@# primary is down (should fail)
||Failed to execute query on Metadata server <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>: Lost connection to MySQL server during query (MySQL Error 2013)
|ERROR: Unable to connect to the PRIMARY of the replicaset myrs: MYSQLSH 51118: Could not open connection to '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>': Can't connect to MySQL server on '<<<libmysql_host_description(hostname_ip, __mysql_sandbox_port1)>>>'|
|Cluster change operations will not be possible unless the PRIMARY can be reached.|
|If the PRIMARY is unavailable, you must either repair it or perform a forced failover.|
|See \help forcePrimaryInstance for more information.|
||PRIMARY instance is unavailable (MYSQLSH 51118)

//@# promoted has broken/stopped replication (should fail)
||Replication is stopped at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>

//@# a secondary has broken replication (should fail)
||Replication is stopped at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>

//@ primary has unexpected replication channel (should fail)
||Replication or configuration errors at <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> (MYSQLSH 51131)

//@# promoted is down (should fail)
|ERROR: Unable to connect to the target instance 'localhost:<<<__mysql_sandbox_port3>>>'. Please verify the connection settings, make sure the instance is available and try again.|
||Could not open connection to 'localhost:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on '<<<libmysql_host_description('localhost', __mysql_sandbox_port3)>>>' ([[*]]) (MySQL Error 2003)

//@# a secondary is down (should fail)
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> will be promoted to PRIMARY of 'myrs'.|
|The current PRIMARY is <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
||
|* Connecting to replicaset instances|
|** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>|
|WARNING: Could not connect to SECONDARY instance: MySQL Error 2003: Could not open connection to '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>': Can't connect to MySQL server on '<<<libmysql_host_description(hostname_ip, __mysql_sandbox_port3)>>>' ([[*]])|
||One or more instances are unreachable (MYSQLSH 51124)

//@# a secondary has errant GTIDs (should fail)
|The current PRIMARY is <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> has 1 errant transactions that have not originated from the current PRIMARY (<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>)|

||Errant transactions at <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> (MYSQLSH 51152)

//@# Replication conflict error (should fail)
|* Checking transaction state of the instance...|
|ERROR: Replication or configuration errors at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: source="<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>" [[*]]|
||Replication or configuration errors at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>

//@# promoted has errant GTIDs (should fail)
|* Checking transaction state of the instance...|
|ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> has 1 errant transactions that have not originated from the current PRIMARY (<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>)|
||Errant transactions at <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> (MYSQLSH 51152)

//@ BUG#30574971 - Switch primary using rs2.
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> was promoted to PRIMARY.|

//@ BUG#30574971 - Add 3rd instance using the other replicaset object rs1.
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>.|

//@# Cleanup
||
