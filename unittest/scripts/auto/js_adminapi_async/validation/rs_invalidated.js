//@# INCLUDE async_utils.inc
||

//@# Setup
||

//@# Check status from a member
|<ReplicaSet:myrs>|
|"status": "UNAVAILABLE",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"status": "UNREACHABLE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|
|"status": "ERROR",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|"status": "ERROR",|

//@# Try addInstance (should fail)
||PRIMARY instance is unavailable (MYSQLSH 51118)

//@# Try removeInstance (should fail)
||PRIMARY instance is unavailable (MYSQLSH 51118)

//@# Try setPrimary (should fail)
||PRIMARY instance is unavailable (MYSQLSH 51118)

//@# force failover
|Failover finished successfully.|
|"status": "AVAILABLE_PARTIAL",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"status": "INVALIDATED"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|"status": "ONLINE"|

//@# Restart invalidated member and check status from the current primary
|"status": "AVAILABLE_PARTIAL",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"status": "INVALIDATED"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|"status": "ONLINE"|

//@# same from the invalidated primary
|WARNING: MYSQLSH 51121: Target <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> was invalidated in a failover: reconnecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>|
|"status": "AVAILABLE_PARTIAL",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"status": "INVALIDATED"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|"status": "ONLINE"|

//@# do stuff in the old primary to simulate split-brain
|{|
|    "replicaSet": {|
|        "name": "myrs", |
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|        "status": "AVAILABLE_PARTIAL", |
|        "statusText": "The PRIMARY instance is available, but one or more SECONDARY instances are not.", |
|        "topology": {|
|            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|                "instanceErrors": [|
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
|                    "applierStatus": "APPLIED_ALL", |
|                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",'>>>|
|                    <<<(__version_num<80023)?'"applierWorkerThreads": 4':''>>>|
//@# do stuff in the old primary to simulate split-brain
|                    "receiverStatus": "ON", |
|                    "receiverThreadState": "Waiting for master to send event", |
|                    "replicationLag": null|
|                }, |
|                "status": "ONLINE"|
|            }|
|        }, |
|        "type": "ASYNC"|
|    }|
|}|

//@# remove the invalidated member while it's up (should fail)
|WARNING: <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> has 1 errant transactions that have not originated from the current PRIMARY (<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>)|
|WARNING: Replication is not active in instance <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|Use the 'force' option to remove this instance anyway. The instance may be left in an inconsistent state after removed.|
||Replication is not active in target instance (MYSQLSH 51132)
|{|
|    "replicaSet": {|
|        "name": "myrs", |
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>", |
|        "status": "AVAILABLE_PARTIAL", |
|        "statusText": "The PRIMARY instance is available, but one or more SECONDARY instances are not.", |
|        "topology": {|
|            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|                "instanceErrors": [|
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
|                    "applierStatus": "APPLIED_ALL", |
|                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",'>>>|
|                    <<<(__version_num<80023)?'"applierWorkerThreads": 4':''>>>|
|                    "receiverStatus": "ON", |
|                    "receiverThreadState": "Waiting for master to send event", |
|                    "replicationLag": null|
|                }, |
|                "status": "ONLINE"|
|            }|
|        }, |
|        "type": "ASYNC"|
|    }|
|}|

//@# remove the invalidated member while it's up with force
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' was removed from the replicaset.|

//@ add back the split-brained instance (should fail)
|Adding instance to the replicaset...|
|* Performing validation checks|
|This instance reports its own address as <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|<<<__endpoint_uri1>>>: Instance configuration is suitable.|
|* Checking async replication topology...|
|* Checking transaction state of the instance...|
|WARNING: A GTID set check of the MySQL instance at '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' determined that|
|contains transactions that do not originate from the replicaset, which must|
|discarded before it can join the replicaset.|
//@ add back the split-brained instance (should fail)
||Instance provisioning required (MYSQLSH 51153)

//@ add back the split-brained instance after re-building it
|NOTE: The target instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' has not been pre-provisioned (GTID|

//@# status should show sb1 as PRIMARY
|"primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|

//@# connect to the invalidated PRIMARY and check status from there
|WARNING: MYSQLSH 51121: Target <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> was invalidated in a failover: reconnecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>|
|"primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|"status": "INVALIDATED"|

//@# rejoinInstance() the invalidated PRIMARY
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>' rejoined the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>.|
|"primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",|
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",|
|"status": "ONLINE"|
|"address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",|
|"status": "ONLINE"|

//@# remove the invalidated instance while it's down (should fail)
|ERROR: Unable to connect to the target instance localhost:<<<__mysql_sandbox_port1>>>. Please make sure the instance is available and try again. If the instance is permanently not reachable, use the 'force' option to remove it from the replicaset metadata and skip reconfiguration of that instance.|
||Could not open connection to 'localhost:<<<__mysql_sandbox_port1>>>': Can't connect to MySQL server on '<<<libmysql_host_description('localhost', __mysql_sandbox_port1)>>>'

//@# add back the removed instance while it's down (should fail)
|ERROR: Unable to connect to the target instance 'localhost:<<<__mysql_sandbox_port1>>>'. Please verify the connection settings, make sure the instance is available and try again.|
||Could not open connection to 'localhost:<<<__mysql_sandbox_port1>>>': Can't connect to MySQL server on '<<<libmysql_host_description('localhost', __mysql_sandbox_port1)>>>'

//@# add back the removed instance after bringing it back up (should fail)
||<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is already a member of this replicaset. (MYSQLSH 51301)

//@# promote remaining secondary (should fail)
|ERROR: Could not connect to one or more SECONDARY instances. Use the 'invalidateErrorInstances' option to perform the failover anyway by skipping and invalidating unreachable instances.|
||One or more instances are unreachable (MYSQLSH 51161)

//@# promote remaining secondary with invalidateErrorInstances
|WARNING: Could not connect to SECONDARY instance: MySQL Error 2003: Could not open connection to '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>': Can't connect to MySQL server on '<<<libmysql_host_description(hostname_ip, __mysql_sandbox_port2)>>>'|
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> will be invalidated and must be removed from the replicaset.|
|NOTE: 1 instances will be skipped and invalidated during the failover|
|<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> was force-promoted to PRIMARY.|
|NOTE: Former PRIMARY <<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is now invalidated and must be removed from the replicaset.|
|Failover finished successfully.|

//@# connect to invalidated instances and try to removeInstance (should fail)
|WARNING: MYSQLSH 51121: Target <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> was invalidated in a failover: reconnecting to <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>|
|ERROR: <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>> is a PRIMARY and cannot be removed.|
||PRIMARY instance cannot be removed from the replicaset. (MYSQLSH 51302)

//@# setPrimary (should fail)
||<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> was invalidated by a failover (MYSQLSH 51121)

//@# setPrimary with new rs (should fail)
||<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> was invalidated by a failover (MYSQLSH 51121)

//@# forcePrimary while there's already a primary (should fail)
||<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> was invalidated by a failover (MYSQLSH 51121)

//@# forcePrimary with new rs while there's already a primary (should fail)
||PRIMARY still available (MYSQLSH 51116)

//@<OUT> status
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",
        "status": "AVAILABLE_PARTIAL",
        "statusText": "The PRIMARY instance is available, but one or more SECONDARY instances are not.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "fenced": false,
                "instanceErrors": [
                    "WARNING: Instance was INVALIDATED and must be removed from the replicaset.",
                    "ERROR: Instance is NOT a PRIMARY but super_read_only option is OFF. Accidental updates to this instance are possible and will cause inconsistencies in the replicaset."
                ],
                "instanceRole": null,
                "mode": null,
                "status": "INVALIDATED",
                "transactionSetConsistencyStatus": "OK"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "fenced": true,
                "instanceErrors": [
                    "WARNING: Instance was INVALIDATED and must be removed from the replicaset."
                ],
                "instanceRole": null,
                "mode": null,
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,'>>>
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for master to send event",
                    "replicationLag": null
                },
                "status": "INVALIDATED",
                "transactionSetConsistencyStatus": "OK"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> status with new object
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",
        "status": "AVAILABLE_PARTIAL",
        "statusText": "The PRIMARY instance is available, but one or more SECONDARY instances are not.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "fenced": false,
                "instanceErrors": [
                    "WARNING: Instance was INVALIDATED and must be removed from the replicaset.",
                    "ERROR: Instance is NOT a PRIMARY but super_read_only option is OFF. Accidental updates to this instance are possible and will cause inconsistencies in the replicaset."
                ],
                "instanceRole": null,
                "mode": null,
                "status": "INVALIDATED",
                "transactionSetConsistencyStatus": "OK"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "fenced": true,
                "instanceErrors": [
                    "WARNING: Instance was INVALIDATED and must be removed from the replicaset."
                ],
                "instanceRole": null,
                "mode": null,
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,'>>>
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for master to send event",
                    "replicationLag": null
                },
                "status": "INVALIDATED",
                "transactionSetConsistencyStatus": "OK"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@# remove invalidated secondary
|NOTE: <<<hostname_ip>>>:<<<__mysql_sandbox_port2>>> is invalidated, replication sync will be skipped.|
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was removed from the replicaset.|
|        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>", |
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>", |
|                "status": "INVALIDATED"|
|                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>", |
|                "instanceRole": "PRIMARY", |
|                "status": "ONLINE"|

//@ Try to re-add invalidated instance (should fail)
||<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> is already a member of this replicaset. (MYSQLSH 51301)

//@# Promote invalidated (should fail)
||<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>> was invalidated by a failover (MYSQLSH 51121)

//@# remove invalidated primary
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' was removed from the replicaset.|

//@# add back invalidated primary
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>.|

//@# add back other invalidated primary
|The instance '<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>' was added to the replicaset and is replicating from <<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>.|

//@<OUT> final status
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",
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
                    "receiverThreadState": "Waiting for master to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,'>>>
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for master to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port3>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}
