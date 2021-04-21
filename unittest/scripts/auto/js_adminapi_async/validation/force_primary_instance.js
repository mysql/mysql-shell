//@ INCLUDE async_utils.inc
||

//@# bad parameters (should fail)
||Argument #1 is expected to be a string (TypeError)
||Argument #2 is expected to be a map (TypeError)
||Invalid number of arguments, expected 0 to 2 but got 3 (ArgumentError)
||Invalid options: badOption (ArgumentError)
||Argument #1 is expected to be a string (TypeError)
||Argument #2 is expected to be a map (TypeError)
||Argument #1 is expected to be a string (TypeError)

//@# disconnected rs object (should fail)
||The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object. (RuntimeError)

//@# auto-promote while there's only a primary (should fail)
||Could not find a suitable candidate to failover to (MYSQLSH 51129)

//@# primary still available (should fail)
||PRIMARY still available (MYSQLSH 51116)

//@# primary promoted (should fail)
||<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is already the PRIMARY (MYSQLSH 51128)

//@# stop primary and failover with bad handle (should fail)
||Failed to execute query on Metadata server <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>: Lost connection to MySQL server during query (MySQL Error 2013)

//@# promoted isn't member (should fail)
||Target instance localhost:<<<__mysql_sandbox_port3>>> is not a managed instance. (MYSQLSH 51300)

//@# promoted doesn't exist (should fail)
|ERROR: Unable to connect to the target instance 'localhost:<<<__mysql_sandbox_port3>>>1'. Please verify the connection settings, make sure the instance is available and try again.|
||Could not open connection to 'localhost:<<<__mysql_sandbox_port3>>>1': Can't connect to MySQL server on '<<<libmysql_host_description('localhost', "" + __mysql_sandbox_port3 + "1")>>>'

//@# check state of instances after switch
|Failover finished successfully.|
|view_id	cluster_id	instance_id	label	member_id	member_role	master_instance_id	master_member_id|
|65536	<<<cluster_id>>>	2	<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>	5ef81566-9395-11e9-87e9-222222222222	PRIMARY	NULL	NULL|
|65536	<<<cluster_id>>>	3	<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>	5ef81566-9395-11e9-87e9-333333333333	SECONDARY	2	5ef81566-9395-11e9-87e9-222222222222|
|{|
|    "replicaSet": {|
|        "name": "myrs", |
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|        "status": "AVAILABLE_PARTIAL", |
|        "statusText": "The PRIMARY instance is available, but one or more SECONDARY instances are not.", |
|        "topology": {|
|            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|                "connectError": "Could not open connection to '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>': Can't connect to MySQL server on '<<<libmysql_host_description(hostname_ip, __mysql_sandbox_port1)>>>'|
|                "fenced": null,|
|                "instanceRole": null, |
|                "mode": null, |
|                "status": "INVALIDATED"|
|            }, |
|            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|                "instanceRole": "PRIMARY", |
|                "mode": "R/W", |
|                "status": "ONLINE"|
|            }, |
|            "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>": {|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>", |
|                "instanceRole": "SECONDARY", |
|                "mode": "R/O", |
|                "replication": {|
|                    "applierStatus": "APPLIED_ALL",|
|                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",'>>>|
|                    <<<(__version_num<80023)?'"applierWorkerThreads": 4':''>>>|
|                    "receiverStatus": "ON", |
|                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event"|
|                }, |
|                "status": "ONLINE"|
|            }|
|        }, |
|        "type": "ASYNC"|
|    }|
|}|
|0|

//@<OUT> check state of instances after switch - view change records
cluster_id	view_id	topology_type	view_change_reason	view_change_time	view_change_info	attributes
<<<cluster_id>>>	1	SINGLE-PRIMARY-TREE	CREATE	[[*]]	{"user": "root@localhost", "source": "5ef81566-9395-11e9-87e9-111111111111"}	{}
<<<cluster_id>>>	2	SINGLE-PRIMARY-TREE	ADD_INSTANCE	[[*]]	{"user": "root@localhost", "source": "5ef81566-9395-11e9-87e9-111111111111"}	{}
<<<cluster_id>>>	3	SINGLE-PRIMARY-TREE	ADD_INSTANCE	[[*]]	{"user": "root@[[*]]", "source": "5ef81566-9395-11e9-87e9-111111111111"}	{}
<<<cluster_id>>>	4	SINGLE-PRIMARY-TREE	ADD_INSTANCE	[[*]]	{"user": "root@[[*]]", "source": "5ef81566-9395-11e9-87e9-111111111111"}	{}
<<<cluster_id>>>	65536	SINGLE-PRIMARY-TREE	FORCE_ACTIVE	[[*]]	{"user": "root@[[*]]", "source": "5ef81566-9395-11e9-87e9-222222222222"}	{}

//@# promoted is not super-read-only
|Failover finished successfully.|

//@# promote another one right after (should fail)
||PRIMARY still available (MYSQLSH 51116)

//@# normal promotion
|<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> was promoted to PRIMARY.|

//@# automatically pick the promoted primary
|"primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|Failover finished successfully.|
|"primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|

//@# automatically pick the promoted primary, while one of them is behind
|"primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|NOTE: Former PRIMARY <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is now invalidated and must be removed from the replicaset.|
|Failover finished successfully.|
|"primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|

//@# dryRun prepare
||

//@# dryRun
|* Connecting to replicaset instances|
|** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>|
|* Checking status of last known PRIMARY|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is UNREACHABLE|
|* Checking status of promoted instance|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> has status ERROR|
|* Checking transaction set status|
|* Promoting <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to a PRIMARY...|
|* Updating metadata...|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> was force-promoted to PRIMARY.|
|NOTE: Former PRIMARY <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is now invalidated and must be removed from the replicaset.|
|* Updating source of remaining SECONDARY instances|
|** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|Failover finished successfully.|
|dryRun finished.|
|* Connecting to replicaset instances|
|** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|** Connecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>|
|* Checking status of last known PRIMARY|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is UNREACHABLE|
|* Checking status of promoted instance|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> has status ERROR|
|* Checking transaction set status|
|* Promoting <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> to a PRIMARY...|
|* Updating metadata...|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> was force-promoted to PRIMARY.|
|NOTE: Former PRIMARY <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is now invalidated and must be removed from the replicaset.|
|* Updating source of remaining SECONDARY instances|
|** Changing replication source of <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|Failover finished successfully.|
|dryRun finished.|

//@# timeout (should fail)
||Error during failover: MySQL Error 2013: Failed to execute query on Metadata server <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>: Lost connection to MySQL server during query (MYSQLSH 51162)

//@# ensure switch over rolls back after the timeout
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"fenced": null,|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|"fenced": true,|

//@# try to switch to a different one (should work this time)
|Failover finished successfully.|
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|                "status": "INVALIDATED"|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|
|                "applierStatus": "APPLYING",|
|                "status": "ONLINE"|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|                "status": "ONLINE"|

//@ the frozen instance should finish applying its relay log
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|                "status": "INVALIDATED"|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|
|                "applierStatus": "APPLIED_ALL",|
|                "status": "ONLINE"|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|                "status": "ONLINE"|

//@# invalidateErrorInstances - preparation
||

//@# a different secondary is down (should fail)
||One or more instances are unreachable

//@# a different secondary is down with invalidateErrorInstances
|<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> was force-promoted to PRIMARY.|
|NOTE: Former PRIMARY <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is now invalidated and must be removed from the replicaset.|
|Failover finished successfully.|

