//@<OUT> Status from master
{
    "clusterName": "sample",
    "defaultReplicaSet": {
        "GRProtocolVersion": "[[*]]",
        "clusterErrors": [
?{VER(>=8.0.27) && VER(<8.3.0)}
            "WARNING: The Cluster's group_replication_view_change_uuid is not stored in the Metadata. Please use <Cluster>.rescan() to update the metadata.",
?{}
            "WARNING: Cluster's transaction size limit is not registered in the metadata. Use cluster.rescan() to update the metadata."
        ],
        "communicationStack": "XCOM",
        "groupName": "<<<gr_uuid>>>",
?{VER(>=8.0.26)}
        "groupViewChangeUuid": "[[*]]",
?{}
        "groupViewId": "[[*]]",
        "name": "default",
?{VER(>=8.0.31)}
        "paxosSingleLeader": "[[*]]",
?{}
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
${*}
                "memberId": "<<<uuid1>>>",
                "memberRole": "PRIMARY",
                "memberState": "ONLINE",
                "mode": "R/W",
                "readReplicas": {},
?{VER(>=8.0.11)}
                "replicationLag": [[*]],
?{}
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
                            "immediateCommitTimestamp": "[[*]]",
                            "immediateCommitToEndTime": [[*]],
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
                        "lastProcessed": {
                            "bufferTime": [[*]],
                            "endTimestamp": "[[*]]",
                            "immediateCommitTimestamp": "[[*]]",
                            "immediateCommitToEndTime": [[*]],
                            "originalCommitTimestamp": "[[*]]",
                            "originalCommitToEndTime": [[*]],
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
                                "immediateCommitTimestamp": "[[*]]",
                                "immediateCommitToEndTime": [[*]],
                                "originalCommitTimestamp": "[[*]]",
                                "originalCommitToEndTime": [[*]],
                                "retries": 0,
                                "startTimestamp": "[[*]]",
?{}
?{VER(>=8.0.17) && VER(<8.0.25)}
                                "transaction": "<<<gr_uuid>>>:[[*]]"
?{}
?{VER(>=8.0.25) && VER(<8.3.0)}
                                "transaction": "<<<gr_view_change_uuid>>>:[[*]]"
?{}
?{VER(>=8.3.0)}
                                "transaction": ""
?{}
?{VER(>=8.0.17)}
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
${*}
                "memberId": "<<<uuid2>>>",
                "memberRole": "SECONDARY",
                "memberState": "ONLINE",
                "mode": "R/O",
                "readReplicas": {},
?{VER(>=8.0.11)}
                "replicationLag": [[*]],
?{}
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
                            "immediateCommitTimestamp": "[[*]]",
                            "immediateCommitToEndTime": [[*]],
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
                                "immediateCommitTimestamp": "[[*]]",
                                "immediateCommitToEndTime": [[*]],
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
                "option": "transactionSizeLimit",
                "value": "[[*]]",
                "variable": "group_replication_transaction_size_limit"
            },
            {
                "option": "disableClone",
                "value": [[*]]
            }, 
            {
                "option": "replicationAllowedHost", 
                "value": null
            },
            {
                "option": "memberAuthType",
                "value": null
            },
            {
                "option": "certIssuer",
                "value": null
?{VER(>=8.0.27)}
            },
            {
                "option": "communicationStack",
                "value": "XCOM",
                "variable": "group_replication_communication_stack"
?{}
?{VER(>=8.0.31)}
            },
            {
                "option": "paxosSingleLeader",
                "value": [[*]]
?{}
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
?{VER(>=8.4.0)}
                    "value": "OFFLINE_MODE",
?{}
?{VER(<8.4.0)}
                    "value": "READ_ONLY",
?{}
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
?{VER(>=8.0.23)}
                    "value": "<<<allowlist>>>",
?{}
?{VER(<8.0.23)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
?{(VER(>=8.1.0) && VER(<8.3.0)) || VER(<8.0.37)}
                {
                    "option": "ipWhitelist",
?{}
?{((VER(>=8.1.0) && VER(<8.3.0)) || VER(<8.0.37)) && VER(>=8.0.23)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.23)}
                    "value": "<<<allowlist>>>",
                    "variable": "group_replication_ip_whitelist"
                },
?{}
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
                {
                    "option": "certSubject",
                    "value": ""
?{VER(<8.0.23)}
                },
                {
                    "value": "COMMIT_ORDER",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "DATABASE",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
?{}
?{VER(>=8.0.23) && VER(<8.4.0)}
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
?{}
?{VER(>=8.0.23) && VER(<8.3.0)}
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
?{}
?{VER(>=8.4.0)}
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
?{}
?{VER(<8.3.0)}
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
?{}
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
?{VER(>=8.4.0)}
                    "value": "OFFLINE_MODE",
?{}
?{VER(<8.4.0)}
                    "value": "READ_ONLY",
?{}
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
?{VER(>=8.0.23)}
                    "value": "<<<allowlist>>>",
?{}
?{VER(<8.0.23)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
?{(VER(>=8.1.0) && VER(<8.3.0)) || VER(<8.0.37)}
                {
                    "option": "ipWhitelist",
?{}
?{((VER(>=8.1.0) && VER(<8.3.0)) || VER(<8.0.37)) && VER(>=8.0.23)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.23)}
                    "value": "<<<allowlist>>>",
                    "variable": "group_replication_ip_whitelist"
                },
?{}
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
                {
                    "option": "certSubject",
                    "value": ""
?{VER(<8.0.23)}
                },
                {
                    "value": "COMMIT_ORDER",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "DATABASE",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
?{}
?{VER(>=8.0.23) && VER(<8.4.0)}
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
?{}
?{VER(>=8.0.23) && VER(<8.3.0)}
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
?{}
?{VER(>=8.4.0)}
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
?{}
?{VER(<8.3.0)}
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
?{}
                }
            ]
        }
    }
}
