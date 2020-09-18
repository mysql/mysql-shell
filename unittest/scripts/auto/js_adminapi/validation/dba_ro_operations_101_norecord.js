//@ Initialization
||


//@ Get Cluster from master
|WARNING: No cluster change operations can be executed because the installed metadata version 1.0.1 is lower than the version required by Shell which is version 2.0.0. Upgrade the metadata to remove this restriction. See \? dba.upgradeMetadata for additional details.|

//@<OUT> Status from master
{
    "clusterName": "sample",
    "defaultReplicaSet": {
        "GRProtocolVersion": "[[*]]",
        "groupName": "<<<gr_uuid>>>",
        "groupViewId": "[[*]]",
        "name": "default",
        "primary": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
        "ssl": "REQUIRED",
        "status": "OK_NO_TOLERANCE",
        "statusText": "Cluster is NOT tolerant to any failures.",
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
?{VER(>=8.0.23)}
                "applierWorkerThreads": 4,
?{}
                "fenceSysVars": [],
                "memberId": "<<<uuid1>>>",
                "memberRole": "PRIMARY",
                "memberState": "ONLINE",
                "mode": "R/W",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "transactions": {
?{VER(>=8.0.17)}
                    "appliedCount": [[*]],
?{}
                    "checkedCount": [[*]],
                    "committedAllMembers": "[[*]]",
                    "conflictsDetectedCount": 0,
                    "connection": {
                        "lastHeartbeatTimestamp": "",
?{VER(>=8.0.17)}
                        "lastQueued": {
                            "endTimestamp": "[[*]]",
                            "immediateCommitTimestamp": "",
                            "immediateCommitToEndTime": null,
                            "originalCommitTimestamp": "",
                            "originalCommitToEndTime": null,
                            "queueTime": [[*]],
                            "startTimestamp": "[[*]]",
                            "transaction": "[[*]]"
                        },
?{}
                        "receivedHeartbeats": 0,
                        "receivedTransactionSet": "[[*]]",
                        "threadId": null
                    },
?{VER(>=8.0.23)}
                    "coordinator": {
                        "lastProcessed": {
                            "bufferTime": [[*]],
                            "endTimestamp": "[[*]]",
                            "immediateCommitTimestamp": "",
                            "immediateCommitToEndTime": null,
                            "originalCommitTimestamp": "",
                            "originalCommitToEndTime": null,
                            "startTimestamp": "[[*]]",
                            "transaction": "[[*]]"
                        },
                        "threadId": [[*]]
                    },
?{}
?{VER(>=8.0.17)}
                    "inApplierQueueCount": [[*]],
?{}
                    "inQueueCount": [[*]],
                    "lastConflictFree": "[[*]]",
?{VER(>=8.0.17)}
                    "proposedCount": [[*]],
                    "rollbackCount": [[*]],
?{}
                    "workers": [
                        {
${*}
?{VER(>=8.0.17)}
                            "lastApplied": {
                                "applyTime": [[*]],
                                "endTimestamp": "[[*]]",
                                "immediateCommitTimestamp": "",
                                "immediateCommitToEndTime": null,
                                "originalCommitTimestamp": "",
                                "originalCommitToEndTime": null,
                                "retries": 0,
                                "startTimestamp": "[[*]]",
                                "transaction": "<<<gr_uuid>>>:[[*]]"
                            },
?{}
?{VER(<8.0.17)}
                            "lastSeen": "[[*]]",
?{}
                            "threadId": [[*]]
?{VER(>=8.0.23)}
                        },
${*}
?{}
                        }
                    ]
                },
                "version": "[[*]]"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
?{VER(>=8.0.23)}
                "applierWorkerThreads": 4,
?{}
                "fenceSysVars": [
                    "read_only",
                    "super_read_only"
                ],
                "memberId": "<<<uuid2>>>",
                "memberRole": "SECONDARY",
                "memberState": "ONLINE",
                "mode": "R/O",
                "readReplicas": {},
                "role": "HA",
                "status": "ONLINE",
                "transactions": {
?{VER(>=8.0.17)}
                    "appliedCount": [[*]],
                    "checkedCount": [[*]],
                    "committedAllMembers": "",
                    "conflictsDetectedCount": 0,
?{}
                    "connection": {
                        "lastHeartbeatTimestamp": "",
?{VER(>=8.0.17)}
                        "lastQueued": {
                            "endTimestamp": "[[*]]",
                            "immediateCommitTimestamp": "",
                            "immediateCommitToEndTime": null,
                            "originalCommitTimestamp": "[[*]]",
                            "originalCommitToEndTime": [[*]],
                            "queueTime": [[*]],
                            "startTimestamp": "[[*]]",
                            "transaction": "[[*]]"
                        },
?{}
                        "receivedHeartbeats": 0,
                        "receivedTransactionSet": "[[*]]",
                        "threadId": null
                    },
?{VER(>=8.0.23)}
                    "coordinator": {
${*}
                        "threadId": [[*]]
                    },
?{}
?{VER(>=8.0.17)}
                    "inApplierQueueCount": [[*]],
                    "inQueueCount": [[*]],
                    "lastConflictFree": [[*]],
                    "proposedCount": [[*]],
                    "rollbackCount": [[*]],
?{}
                    "workers": [
                        {
${*}
?{VER(>=8.0.17)}
                            "lastApplied": {
                                "applyTime": [[*]],
                                "endTimestamp": "[[*]]",
                                "immediateCommitTimestamp": "",
                                "immediateCommitToEndTime": null,
                                "originalCommitTimestamp": "[[*]]",
                                "originalCommitToEndTime": [[*]],
                                "retries": 0,
                                "startTimestamp": "[[*]]",
                                "transaction": "<<<gr_uuid>>>:[[*]]"
                            },
?{}
?{VER(<8.0.17)}
                            "lastSeen": "[[*]]",
?{}
                            "threadId": [[*]]
?{VER(>=8.0.23)}
                        },
${*}
?{}
?{VER(<8.0.23)}
                        }
?{}
                    ]
                },
                "version": "[[*]]"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
    "metadataVersion": "<<<testutil.getInstalledMetadataVersion()>>>"
}

//@<OUT> Describe from master
{
    "clusterName": "sample",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>",
                "role": "HA"
            },
            {
                "address": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "label": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}

//@<OUT> Options from master
{
    "clusterName": "sample",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "<<<gr_uuid>>>",
                "variable": "group_replication_group_name"
            },
            {
                "option": "memberSslMode",
                "value": "REQUIRED",
                "variable": "group_replication_ssl_mode"
            },
            {
                "option": "disableClone",
                "value": [[*]]
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": [[*]],
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": [[*]],
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": [[*]],
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>1",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "<<<allowlist>>>",
?{}
?{VER(<8.0.22)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
?{VER(>=8.0.22)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.22)}
                    "value": "<<<allowlist>>>",
?{}
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>1",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "50",
                    "variable": "group_replication_member_weight"
                },
?{VER(<8.0.23)}
                {
                    "value": "COMMIT_ORDER",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "DATABASE",
                    "variable": "slave_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "slave_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "slave_preserve_commit_order"
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "slave_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "slave_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "slave_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": [[*]],
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": [[*]],
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": [[*]],
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<hostname>>>:<<<__mysql_sandbox_port1>>>1",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "<<<allowlist>>>",
?{}
?{VER(<8.0.22)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
?{VER(>=8.0.22)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.22)}
                    "value": "<<<allowlist>>>",
?{}
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<hostname>>>:<<<__mysql_sandbox_port2>>>1",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "50",
                    "variable": "group_replication_member_weight"
                },
?{VER(<8.0.23)}
                {
                    "value": "COMMIT_ORDER",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "DATABASE",
                    "variable": "slave_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "slave_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "slave_preserve_commit_order"
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "slave_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "slave_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "slave_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ]
        }
    }
}

//@ Get Cluster from slave
|WARNING: No cluster change operations can be executed because the installed metadata version 1.0.1 is lower than the version required by Shell which is version 2.0.0. Upgrade the metadata to remove this restriction. See \? dba.upgradeMetadata for additional details.|

//@ Finalization
||
