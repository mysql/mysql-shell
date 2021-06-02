//@ Setup
||

//@ Errors (should fail)
||Argument #1 is expected to be a map (TypeError)
||Option 'extended' Integer expected, but value is String (TypeError)
||Invalid value '-1' for option 'extended'. It must be an integer in the range [0, 2]. (ArgumentError)
||Invalid value '3' for option 'extended'. It must be an integer in the range [0, 2]. (ArgumentError)
||Invalid options: bla (ArgumentError)

//@<OUT> Regular, extended:0
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
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
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> Regular, extended:1
{
    "metadataVersion": "<<<testutil.getInstalledMetadataVersion()>>>",
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "fenceSysVars": [],
                "fenced": false,
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "serverUuid": "5ef81566-9395-11e9-87e9-111111111111",
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "fenceSysVars": [
                    "read_only",
                    "super_read_only"
                ],
                "fenced": true,
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierQueuedTransactionSet": "",
                    "applierQueuedTransactionSetSize": 0,
                    "applierState": "ON",
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,\n                    "coordinatorState": "ON",\n                    "coordinatorThreadState": "'+__replica_keyword_capital+' has read all relay log; waiting for more updates",'>>>
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "receiverTimeSinceLastMessage": "[[*]]",
                    "replicationLag": null,
                    "source": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>"
                },
                "serverUuid": "5ef81566-9395-11e9-87e9-222222222222",
                "status": "ONLINE",
                "transactionSetConsistencyStatus": "OK"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> Regular, extended:2
{
    "metadataVersion": "<<<testutil.getInstalledMetadataVersion()>>>",
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "fenceSysVars": [],
                "fenced": false,
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "serverUuid": "5ef81566-9395-11e9-87e9-111111111111",
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "fenceSysVars": [
                    "read_only",
                    "super_read_only"
                ],
                "fenced": true,
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierQueuedTransactionSet": "",
                    "applierQueuedTransactionSetSize": 0,
                    "applierState": "ON",
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,\n                    "coordinatorState": "ON",\n                    "coordinatorThreadState": "'+__replica_keyword_capital+' has read all relay log; waiting for more updates",'>>>
                    "options": {
                        "connectRetry": 60,
                        "delay": 0,
                        "heartbeatPeriod": 30,
                        "retryCount": 86400
                    },
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "receiverTimeSinceLastMessage": "[[*]]",
                    "replicationLag": null,
                    "source": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>"
                },
                "serverUuid": "5ef81566-9395-11e9-87e9-222222222222",
                "status": "ONLINE",
                "transactionSetConsistencyStatus": "OK"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> Replication delay, extended:0
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,'>>>
                    "options": {
                        "delay": 42
                    },
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> Heartbeat interval, extended:2
{
    "metadataVersion": "<<<testutil.getInstalledMetadataVersion()>>>",
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "fenceSysVars": [],
                "fenced": false,
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "serverUuid": "5ef81566-9395-11e9-87e9-111111111111",
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "fenceSysVars": [
                    "read_only",
                    "super_read_only"
                ],
                "fenced": true,
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierQueuedTransactionSet": "",
                    "applierQueuedTransactionSetSize": 0,
                    "applierState": "ON",
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",\n                    "applierWorkerThreads": 1,':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,\n                    "coordinatorState": "ON",\n                    "coordinatorThreadState": "'+__replica_keyword_capital+' has read all relay log; waiting for more updates",'>>>
//@<OUT> Heartbeat interval, extended:2
                    "options": {
                        "connectRetry": 60,
                        "delay": 0,
                        "heartbeatPeriod": 123,
                        "retryCount": 86400
                    },
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "receiverTimeSinceLastMessage": "[[*]]",
                    "replicationLag": null,
                    "source": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>"
                },
                "serverUuid": "5ef81566-9395-11e9-87e9-222222222222",
                "status": "ONLINE",
                "transactionSetConsistencyStatus": "OK"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> Worker threads, extended:0
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": "Waiting for an event from Coordinator",
                    "applierWorkerThreads": 3,
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> BUG#32015164: instanceErrors must report missing parallel-appliers
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "instanceErrors": [
                    "NOTE: The required parallel-appliers settings are not enabled on the instance. Use dba.configureReplicaSetInstance() to fix it."
                ],
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",\n                    "applierWorkerThreads": 1,':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,'>>>
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> BUG#32015164: status should be fine now
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "AVAILABLE",
        "statusText": "All instances available.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "instanceRole": "PRIMARY",
                "mode": "R/W",
                "status": "ONLINE"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": "Waiting for an event from Coordinator",
                    "applierWorkerThreads": 4,
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> Primary is RO, should show as error
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "UNAVAILABLE",
        "statusText": "PRIMARY instance is not available, but there is at least one SECONDARY that could be force-promoted.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "fenced": true,
                "instanceErrors": [
                    "ERROR: Instance is a PRIMARY but is READ-ONLY: read_only=ON, super_read_only=ON"
                ],
                "instanceRole": "PRIMARY",
                "mode": "R/O",
                "status": "ERROR"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": "<<<__replica_keyword_capital>>> has read all relay log; waiting for more updates",
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> Replication stopped in a secondary
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "UNAVAILABLE",
        "statusText": "PRIMARY instance is not available, but there is at least one SECONDARY that could be force-promoted.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "fenced": true,
                "instanceErrors": [
                    "ERROR: Instance is a PRIMARY but is READ-ONLY: read_only=ON"
                ],
                "instanceRole": "PRIMARY",
                "mode": "R/O",
                "status": "ERROR"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "fenced": true,
                "instanceErrors": [
                    "ERROR: Replication is stopped."
                ],
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "OFF",
                    "applierThreadState": "",
                    "expectedSource": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                    "receiverStatus": "OFF",
                    "receiverThreadState": "",
                    "replicationLag": null,
                    "source": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>"
                },
                "status": "OFFLINE",
                "transactionSetConsistencyStatus": "OK"
            }
        },
        "type": "ASYNC"
    }
}

//@<OUT> Replication running in a primary
{
    "replicaSet": {
        "name": "myrs",
        "primary": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
        "status": "UNAVAILABLE",
        "statusText": "PRIMARY instance is not available, but there is at least one SECONDARY that could be force-promoted.",
        "topology": {
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>",
                "fenced": true,
                "instanceErrors": [
                    "ERROR: Instance is a PRIMARY but is READ-ONLY: read_only=ON",
                    "ERROR: Instance is a PRIMARY but is replicating from another instance."
                ],
                "instanceRole": "PRIMARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": <<<(__version_num<80023)?'"Slave has read all relay log; waiting for more updates",':'"Waiting for an event from Coordinator",\n                    "applierWorkerThreads": 4,'>>>
                    "receiverStatus": "ON",
                    "receiverThreadState": "[[*]]",
                    "replicationLag": null
                },
                "status": "ERROR"
            },
            "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname_ip>>>:<<<__mysql_sandbox_port2>>>",
                "instanceRole": "SECONDARY",
                "mode": "R/O",
                "replication": {
                    "applierStatus": "APPLIED_ALL",
                    "applierThreadState": "<<<__replica_keyword_capital>>> has read all relay log; waiting for more updates",
                    "receiverStatus": "ON",
                    "receiverThreadState": "Waiting for <<<__source_keyword>>> to send event",
                    "replicationLag": null
                },
                "status": "ONLINE"
            }
        },
        "type": "ASYNC"
    }
}

//@# A secondary is down
|        "status": "UNAVAILABLE",|
|                "status": "ERROR"|
|                "status": "UNREACHABLE"|

//@# Primary is down
|ERROR: Unable to connect to the PRIMARY of the replicaset myrs: MySQL Error 2003: Could not open connection to '<<<hostname_ip>>>:<<<__mysql_sandbox_port1>>>': Can't connect to MySQL server on '<<<libmysql_host_description(hostname_ip, __mysql_sandbox_port1)>>>'|
|Cluster change operations will not be possible unless the PRIMARY can be reached.|
|If the PRIMARY is unavailable, you must either repair it or perform a forced failover.|
|See \help forcePrimaryInstance for more information.|
|WARNING: MYSQLSH 51118: PRIMARY instance is unavailable|
|        "status": "UNAVAILABLE",|
|                "status": "UNREACHABLE"|
|                "status": "ERROR"|

