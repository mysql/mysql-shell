//@ BUG29265869 - Deploy sandboxes.
||

//@ BUG29265869 - Create cluster with custom GR settings.  {VER(>=8.0.16)}
||

//@ BUG29265869 - Create cluster with custom GR settings for 5.7. {VER(<8.0.0)}
||

//@ BUG29265869 - Add instance with custom GR settings. {VER(>=8.0.16)}
||

//@ BUG29265869 - Add instance with custom GR settings for 5.7. {VER(<8.0.0)}
||

//@ BUG29265869 - Persist GR settings for 5.7. {VER(<8.0.0)}
||

//@<OUT> BUG29265869 - Show initial cluster options.  {VER(>=8.0.17)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "<<<grp_name>>>",
                "variable": "group_replication_group_name"
            },
            {
                "option": "memberSslMode",
                "value": "REQUIRED",
                "variable": "group_replication_ssl_mode"
            },
            {
                "option": "disableClone",
                "value": false
            }
        ],
        "tags": {
            ".global": [],
            "<<<uri1>>>": [],
            "<<<uri2>>>": []
        },
        "topology": {
            "<<<uri1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "<<<consistency>>>",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "<<<exit_state>>>",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "<<<ip_white_list80>>>",
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
                    "value": "<<<ip_white_list80>>>",
?{}
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "<<<member_weight1>>>",
                    "variable": "group_replication_member_weight"
                },
                {
?{VER(>=8.0.23)}
                    "value": "WRITESET",
?{}
?{VER(<8.0.23)}
                    "value": "COMMIT_ORDER",
?{}
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
?{VER(>=8.0.23)}
                    "value": "LOGICAL_CLOCK",
?{}
?{VER(<8.0.23)}
                    "value": "DATABASE",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
?{VER(>=8.0.23)}
                    "value": "4",
?{}
?{VER(<8.0.23)}
                    "value": "0",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
?{VER(>=8.0.23)}
                    "value": "ON",
?{}
?{VER(<8.0.23)}
                    "value": "OFF",
?{}
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<uri2>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "<<<consistency>>>",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "<<<exit_state>>>",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "<<<ip_white_list80>>>",
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
                    "value": "<<<ip_white_list80>>>",
?{}
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "<<<member_weight2>>>",
                    "variable": "group_replication_member_weight"
                },
                {
?{VER(>=8.0.23)}
                    "value": "WRITESET",
?{}
?{VER(<8.0.23)}
                    "value": "COMMIT_ORDER",
?{}
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
?{VER(>=8.0.23)}
                    "value": "LOGICAL_CLOCK",
?{}
?{VER(<8.0.23)}
                    "value": "DATABASE",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
?{VER(>=8.0.23)}
                    "value": "4",
?{}
?{VER(<8.0.23)}
                    "value": "0",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
?{VER(>=8.0.23)}
                    "value": "ON",
?{}
?{VER(<8.0.23)}
                    "value": "OFF",
?{}
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ]
        }
    }
}

//@<OUT> BUG29265869 - Show initial cluster options.  {VER(<8.0.0)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "<<<grp_name>>>",
                "variable": "group_replication_group_name"
            },
            {
                "option": "memberSslMode",
                "value": "REQUIRED",
                "variable": "group_replication_ssl_mode"
            },
            {
                "option": "disableClone",
                "value": true
            }
        ],
        "tags": {
            ".global": [],
            "<<<uri1>>>": [],
            "<<<uri2>>>": []
        },
        "topology": {
            "<<<uri1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": null,
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": null,
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list57>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "50",
                    "variable": "group_replication_member_weight"
                },
                {
?{VER(>=8.0.23)}
                    "value": "WRITESET",
?{}
?{VER(<8.0.23)}
                    "value": "COMMIT_ORDER",
?{}
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
?{VER(>=8.0.23)}
                    "value": "LOGICAL_CLOCK",
?{}
?{VER(<8.0.23)}
                    "value": "DATABASE",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
?{VER(>=8.0.23)}
                    "value": "4",
?{}
?{VER(<8.0.23)}
                    "value": "0",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
?{VER(>=8.0.23)}
                    "value": "ON",
?{}
?{VER(<8.0.23)}
                    "value": "OFF",
?{}
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<uri2>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": null,
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": null,
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list57>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "75",
                    "variable": "group_replication_member_weight"
                },
                {
?{VER(>=8.0.23)}
                    "value": "WRITESET",
?{}
?{VER(<8.0.23)}
                    "value": "COMMIT_ORDER",
?{}
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
?{VER(>=8.0.23)}
                    "value": "LOGICAL_CLOCK",
?{}
?{VER(<8.0.23)}
                    "value": "DATABASE",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
?{VER(>=8.0.23)}
                    "value": "4",
?{}
?{VER(<8.0.23)}
                    "value": "0",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
?{VER(>=8.0.23)}
                    "value": "ON",
?{}
?{VER(<8.0.23)}
                    "value": "OFF",
?{}
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ]
        }
    }
}

//@ BUG29265869 - Kill all cluster members.
||

//@ BUG29265869 - Start the members again.
||

//@ BUG29265869 - connect to instance.
||

//@<OUT> BUG29265869 - Reboot cluster from complete outage and BUG30501978 no provision output shown. {VER(<8.0.11)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.
* Waiting for seed instance to become ONLINE...
<<<hostname>>>:<<<__mysql_sandbox_port1>>> was restored.
Rejoining '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist Group Replication configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureLocalInstance() command locally to persist the changes.

//@<OUT> BUG29265869 - Reboot cluster from complete outage and BUG30501978 no provision output shown. {VER(>=8.0.11)}
ONLINE
ONLINE

//@<OUT> BUG29265869 - Show cluster options after reboot. {VER(>=8.0.17)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "<<<grp_name>>>",
                "variable": "group_replication_group_name"
            },
            {
                "option": "memberSslMode",
                "value": "REQUIRED",
                "variable": "group_replication_ssl_mode"
            },
            {
                "option": "disableClone",
                "value": false
            }
        ],
        "tags": {
            ".global": [],
            "<<<uri1>>>": [],
            "<<<uri2>>>": []
        },
        "topology": {
            "<<<uri1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "<<<consistency>>>",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "<<<exit_state>>>",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "<<<ip_white_list80>>>",
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
                    "value": "<<<ip_white_list80>>>",
?{}
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "<<<member_weight1>>>",
                    "variable": "group_replication_member_weight"
                },
                {
?{VER(>=8.0.23)}
                    "value": "WRITESET",
?{}
?{VER(<8.0.23)}
                    "value": "COMMIT_ORDER",
?{}
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
?{VER(>=8.0.23)}
                    "value": "LOGICAL_CLOCK",
?{}
?{VER(<8.0.23)}
                    "value": "DATABASE",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
?{VER(>=8.0.23)}
                    "value": "4",
?{}
?{VER(<8.0.23)}
                    "value": "0",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
?{VER(>=8.0.23)}
                    "value": "ON",
?{}
?{VER(<8.0.23)}
                    "value": "OFF",
?{}
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<uri2>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "<<<consistency>>>",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "<<<exit_state>>>",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "<<<ip_white_list80>>>",
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
                    "value": "<<<ip_white_list80>>>",
?{}
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "<<<member_weight2>>>",
                    "variable": "group_replication_member_weight"
                },
                {
?{VER(>=8.0.23)}
                    "value": "WRITESET",
?{}
?{VER(<8.0.23)}
                    "value": "COMMIT_ORDER",
?{}
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
?{VER(>=8.0.23)}
                    "value": "LOGICAL_CLOCK",
?{}
?{VER(<8.0.23)}
                    "value": "DATABASE",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
?{VER(>=8.0.23)}
                    "value": "4",
?{}
?{VER(<8.0.23)}
                    "value": "0",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
?{VER(>=8.0.23)}
                    "value": "ON",
?{}
?{VER(<8.0.23)}
                    "value": "OFF",
?{}
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ]
        }
    }
}

//@<OUT> BUG29265869 - Show cluster options after reboot.  {VER(<8.0.0)}
{
    "clusterName": "test",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "<<<grp_name>>>",
                "variable": "group_replication_group_name"
            },
            {
                "option": "memberSslMode",
                "value": "REQUIRED",
                "variable": "group_replication_ssl_mode"
            },
            {
                "option": "disableClone",
                "value": true
            }
        ],
        "tags": {
            ".global": [],
            "<<<uri1>>>": [],
            "<<<uri2>>>": []
        },
        "topology": {
            "<<<uri1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": null,
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": null,
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list57>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "50",
                    "variable": "group_replication_member_weight"
                },
                {
?{VER(>=8.0.23)}
                    "value": "WRITESET",
?{}
?{VER(<8.0.23)}
                    "value": "COMMIT_ORDER",
?{}
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
?{VER(>=8.0.23)}
                    "value": "LOGICAL_CLOCK",
?{}
?{VER(<8.0.23)}
                    "value": "DATABASE",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
?{VER(>=8.0.23)}
                    "value": "4",
?{}
?{VER(<8.0.23)}
                    "value": "0",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
?{VER(>=8.0.23)}
                    "value": "ON",
?{}
?{VER(<8.0.23)}
                    "value": "OFF",
?{}
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<uri2>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": null,
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": null,
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<local_address1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "<<<ip_white_list57>>>",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "75",
                    "variable": "group_replication_member_weight"
                },
                {
?{VER(>=8.0.23)}
                    "value": "WRITESET",
?{}
?{VER(<8.0.23)}
                    "value": "COMMIT_ORDER",
?{}
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
?{VER(>=8.0.23)}
                    "value": "LOGICAL_CLOCK",
?{}
?{VER(<8.0.23)}
                    "value": "DATABASE",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
?{VER(>=8.0.23)}
                    "value": "4",
?{}
?{VER(<8.0.23)}
                    "value": "0",
?{}
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
?{VER(>=8.0.23)}
                    "value": "ON",
?{}
?{VER(<8.0.23)}
                    "value": "OFF",
?{}
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ]
        }
    }
}

//@<OUT> BUG#29305551: Reboot cluster from complete outage must fail if async replication is configured on the target instance
ERROR: Cannot bootstrap cluster on instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' because it has asynchronous (source-replica) replication channel(s) configured. MySQL InnoDB Cluster does not support manually configured channels as they are not managed using the AdminAPI (e.g. when PRIMARY moves to another member) which may cause cause replication to break or even create split-brain scenarios (data loss).

//@<ERR> BUG#29305551: Reboot cluster from complete outage must fail if async replication is configured on the target instance
Dba.rebootClusterFromCompleteOutage: The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' has asynchronous replication configured. (RuntimeError)

//@# Reboot cluster from complete outage, secondary runs async replication = should succeed, but rejoin fail
|<<<hostname>>>:<<<__mysql_sandbox_port1>>> was restored.|
|ERROR: Cannot rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (source-replica) replication channel(s) configured. MySQL InnoDB Cluster does not support manually configured channels as they are not managed using the AdminAPI (e.g. when PRIMARY moves to another member) which may cause cause replication to break or even create split-brain scenarios (data loss).|

//@# Reboot cluster from complete outage, secondary runs async replication = should succeed, but rejoin fail with channels stopped
|<<<hostname>>>:<<<__mysql_sandbox_port1>>> was restored.|
|ERROR: Cannot rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster because it has asynchronous (source-replica) replication channel(s) configured. MySQL InnoDB Cluster does not support manually configured channels as they are not managed using the AdminAPI (e.g. when PRIMARY moves to another member) which may cause cause replication to break or even create split-brain scenarios (data loss).|

//@# Reboot cluster from complete outage, seed runs async replication = should pass
|<<<hostname>>>:<<<__mysql_sandbox_port2>>> was restored.|

//@ BUG30501978 - Reboot cluster from complete outage fails with informative message saying current session is not the most up to date
||The active session instance (<<<hostname>>>:<<<__mysql_sandbox_port1>>>) isn't the most updated in comparison with the ONLINE instances of the Cluster's metadata. Please use the most up to date instance: '<<<hostname>>>:<<<__mysql_sandbox_port2>>>'. (RuntimeError)

//@ BUG30501978 - Reboot cluster from complete outage fails with informative message saying there is a gtid mismatch
||Conflicting transaction sets between <<<hostname>>>:<<<__mysql_sandbox_port1>>> and <<<hostname>>>:<<<__mysql_sandbox_port2>>>

//@ BUG30501978 - Reboot cluster from complete outage fails with informative message saying to run rejoinInstance
|ERROR: A GTID set check of the MySQL instance at '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' determined that it is missing transactions that were purged from all cluster members.|
|Unable to rejoin instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' to the cluster but the dba.rebootClusterFromCompleteOutage() operation will continue.|
