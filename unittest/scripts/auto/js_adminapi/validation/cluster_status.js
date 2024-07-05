//@<ERR> Error when executing status on a cluster with the topology mode different than GR
The InnoDB Cluster topology type (Multi-Primary) does not match the current Group Replication configuration (Single-Primary). Please use <cluster>.rescan() or change the Group Replication configuration accordingly. (RuntimeError)

//@ Error when executing status on a cluster that its name is not registered in the metadata
||The InnoDB Cluster topology type (Multi-Primary) does not match the current Group Replication configuration (Single-Primary). Please use <cluster>.rescan() or change the Group Replication configuration accordingly. (RuntimeError)

//@# memberState=OFFLINE
|"<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {|
|},|
|"<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {|
|    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",|
|    "instanceErrors": [|
|        "NOTE: group_replication is stopped."|
|    ],|
|    "memberState": "OFFLINE",|
|    "mode": "n/a",|
|    "readReplicas": {},|
|    "role": "HA",|
|    "status": "(MISSING)"|
|},|

//@# memberState=ERROR
|"<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {|
|},|
|"<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {|
|    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", |
|    "instanceErrors": [|
|        "ERROR: applier thread of Group Replication Applier channel stopped with an error: [[*]]Error 'Can't create database 'foobar'; database exists' on query. Default database: 'foobar'. Query: 'create schema foobar' (1007) at [[*]]",|
|        "ERROR: group_replication has stopped with an error."|
|    ], |
|    "memberState": "ERROR", |
|    "mode": "n/a", |
|    "readReplicas": {}, |
|    "role": "HA", |
|    "status": "(MISSING)"|
|}|
|"<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {|
|},|
|"<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {|
|    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", |
|    "applierChannel": {|
|        "applierLastErrors": [|
|            {|
|                "error": "[[*]]Error 'Can't create database 'foobar'; database exists' on query. Default database: 'foobar'. Query: 'create schema foobar'", |
|                "errorNumber": 1007, |
|                "errorTimestamp": "[[*]]"|
|            }|
|        ], |
|        "applierQueuedTransactionSetSize": 1, |
|        "applierStatus": "ERROR", |
|        "applierThreadState": "", |
|        "receiverStatus": "ON", |
|        "receiverThreadState": "", |
|        "source": null|
|    }, |
|    "instanceErrors": [|
|        "ERROR: applier thread of Group Replication Applier channel stopped with an error: [[*]]Error 'Can't create database 'foobar'; database exists' on query. Default database: 'foobar'. Query: 'create schema foobar' (1007) at [[*]]", |
|        "ERROR: group_replication has stopped with an error."|
|    ], |
|    "memberState": "ERROR", |
|    "mode": "n/a", |
|    "readReplicas": {}, |
|    "role": "HA", |
|    "status": "(MISSING)"|
|},|

//@ instanceError with an instance that has its UUID changed
|"<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {|
|},|
|"<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {|
|    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", |
|    "instanceErrors": [|
|        "WARNING: server_uuid for instance has changed from its last known value. Use cluster.rescan() to update the metadata."|
|    ], |
|    "mode": "R/O", |
|    "readReplicas": {}, |
|    "role": "HA", |
|    "status": "ONLINE", |
|    "version": "[[*]]"|
|}, |
|"<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {|
|},|

//@ instanceError mismatch would not be reported correctly if the member is not in the group
|"<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {|
|},|
|"<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {|
|    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", |
|    "instanceErrors": [|
|        "NOTE: group_replication is stopped.",|
|        "WARNING: server_uuid for instance has changed from its last known value. Use cluster.rescan() to update the metadata."|
|    ], |
|    "memberState": "OFFLINE",|
|    "mode": "n/a", |
|    "readReplicas": {}, |
|    "role": "HA", |
|    "status": "(MISSING)", |
|    "version": "[[*]]"|
|}, |
|"<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {|
|},|

//@<OUT> Status cluster
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@ BUG#32015164: instanceErrors must report missing parallel-appliers
|"<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {|
|},|
|"<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {|
|    "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>", |
|    "instanceErrors": [|
|        "NOTE: The required parallel-appliers settings are not enabled on the instance. Use dba.configureInstance() to fix it."|
|    ], |
|    "mode": "R/O", |
|    "readReplicas": {}, |
|    "role": "HA", |
|    "status": "ONLINE", |
|    "version": "[[*]]"|
|}, |
|"<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {|
|},|

//@<OUT> BUG#32015164: status should be fine now
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> WL#13084 - TSF2_1: status show accurate mode based on super_read_only.
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "instanceErrors": [
                    "WARNING: Instance is NOT a PRIMARY but super_read_only option is OFF."
                ],
                "memberRole": "SECONDARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Status cluster with 2 instances having one of them a non-default label
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "zzLabel": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Status cluster after adding R/O instance back
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "zzLabel": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Status from a read-only member
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK",
        "statusText": "Cluster is ONLINE and can tolerate up to ONE failure.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "zzLabel": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>"
}

//@<OUT> Status with a new primary
{
    "clusterName": "cluster",
    "defaultReplicaSet": {
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port3>>>",
                "memberRole": "SECONDARY",
                "mode": "R/O",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            },
            "zzLabel": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "memberRole": "PRIMARY",
                "mode": "R/W",
                "readReplicas": {},<<<(__version_num>=80011) ?  "\n                \"replicationLag\": [[*]],":"">>>
                "role": "HA",
                "status": "ONLINE",
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>"
}
