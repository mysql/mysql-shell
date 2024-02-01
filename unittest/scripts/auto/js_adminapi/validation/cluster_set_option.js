//@<OUT> WL#11465: Create single-primary cluster
Creating a Classic session to 'root@localhost:<<<__mysql_sandbox_port1>>>'

//@ WL#11465: ArgumentErrors of setOption
||Invalid number of arguments, expected 2 but got 0 (ArgumentError)
||Invalid number of arguments, expected 2 but got 1 (ArgumentError)
||Option 'foobar' not supported. (ArgumentError)

//@ WL#11465: Error when executing setOption on a cluster with 1 or more members not ONLINE
||This operation requires all the cluster members to be ONLINE (RuntimeError)

//@<ERR> WL#11465: Error when executing setOption on a cluster with no visible quorum {VER(>=8.0.14)}
Cluster.setOption: There is no quorum to perform the operation (MYSQLSH 51011)

//@<ERR> WL#11465: Error when executing setOption on a cluster with no visible quorum 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
Cluster.setOption: There is no quorum to perform the operation (MYSQLSH 51011)

//@ WL#11465: Re-create the cluster
||

//@ WL#11465: setOption clusterName with invalid value for cluster-name
||Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (0_a)
||_1234567890::_1234567890123456789012345678901234567890123456789012345678901234: The Cluster name can not be greater than 63 characters. (ArgumentError)
||Cluster name may only contain alphanumeric characters, '_', '-', or '.' and may not start with a number (::) (ArgumentError)

//@<OUT> WL#11465: setOption clusterName
Setting the value of 'clusterName' to 'newName' in the Cluster ...

Successfully set the value of 'clusterName' to 'newName' in the Cluster: 'cluster'.

//@<OUT> WL#11465: Verify clusterName changed correctly
newName

//@<OUT> WL#11465: setOption memberWeight {VER(>=8.0.0)}
Setting the value of 'memberWeight' to '25' in all cluster members ...

Successfully set the value of 'memberWeight' to '25' in the 'newName' cluster.

//@<OUT> WL#11465: setOption memberWeight 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
Setting the value of 'memberWeight' to '25' in all cluster members ...

Successfully set the value of 'memberWeight' to '25' in the 'newName' cluster.

//@<ERR> WL#11465: setOption exitStateAction with invalid value
Cluster.setOption: Variable 'group_replication_exit_state_action' can't be set to the value of 'ABORT' (MYSQLSH 1231)

//@<OUT> WL#11465: setOption exitStateAction {VER(>=8.0.0)}
Setting the value of 'exitStateAction' to 'ABORT_SERVER' in all cluster members ...

Successfully set the value of 'exitStateAction' to 'ABORT_SERVER' in the 'newName' cluster.

//@<OUT> WL#11465: setOption exitStateAction 5.7 {VER(>=5.7.24) && VER(<8.0.0)}
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
WARNING: Instance '<<<hostname>>>:<<<__mysql_sandbox_port3>>>' cannot persist configuration since MySQL version <<<__version>>> does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the dba.configureInstance() command locally, using the 'mycnfPath' option, to persist the changes.
Setting the value of 'exitStateAction' to 'ABORT_SERVER' in all cluster members ...

Successfully set the value of 'exitStateAction' to 'ABORT_SERVER' in the 'newName' cluster.

//@<OUT> WL#11465: setOption consistency {VER(>=8.0.14)}
Setting the value of 'consistency' to 'BEFORE_ON_PRIMARY_FAILOVER' in all cluster members ...

Successfully set the value of 'consistency' to 'BEFORE_ON_PRIMARY_FAILOVER' in the 'newName' cluster.

//@<OUT> WL#11465: setOption expelTimeout {VER(>=8.0.14)}
Setting the value of 'expelTimeout' to '3500' in all cluster members ...

Successfully set the value of 'expelTimeout' to '3500' in the 'newName' cluster.

//@<OUT> WL#12066: TSF6_1 setOption autoRejoinTries {VER(>=8.0.16)}
WARNING: Each cluster member will only proceed according to its exitStateAction if auto-rejoin fails (i.e. all retry attempts are exhausted).

Setting the value of 'autoRejoinTries' to '2016' in all cluster members ...

Successfully set the value of 'autoRejoinTries' to '2016' in the 'newName' cluster.

//@ WL#12066: TSF2_4 setOption autoRejoinTries doesn't accept negative values {VER(>=8.0.16)}
||Variable 'group_replication_autorejoin_tries' can't be set to the value of '-1' (MYSQLSH 1231)

//@ WL#12066: TSF2_5 setOption autoRejoinTries doesn't accept values out of range {VER(>=8.0.16)}
||Variable 'group_replication_autorejoin_tries' can't be set to the value of '2017' (MYSQLSH 1231)

//@ WL#13208: TS_FR2 verify disableClone cannot be set with setOption() to false in a 5.7 cluster {VER(>=5.7.24) && VER(<8.0.0)}
||Option 'disableClone' not supported on Cluster. (RuntimeError)

//@ WL#13208: TS_FR2 verify disableClone cannot be set with setOption() to true in a 5.7 cluster {VER(>=5.7.24) && VER(<8.0.0)}
||Option 'disableClone' not supported on Cluster. (RuntimeError)

//@<OUT> WL#13208: TS_FR2_1 verify disableClone can be set with setOption() to false. {VER(>=8.0.17)}
Setting the value of 'disableClone' to 'false' in the Cluster ...

Successfully set the value of 'disableClone' to 'false' in the Cluster: 'newName'.

//@<OUT> WL#13208: TS_FR2_2 verify disableClone is false with options(). {VER(>=8.3.0)}
{
    "clusterName": "newName",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "[[*]]",
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
                "value": false
            },
            {
                "option": "replicationAllowedHost",
                "value": "%"
            },
            {
                "option": "memberAuthType",
                "value": "PASSWORD"
            },
            {
                "option": "certIssuer",
                "value": ""
            },
            {
                "option": "communicationStack",
                "value": "XCOM",
                "variable": "group_replication_communication_stack"
            },
            {
                "option": "paxosSingleLeader",
                "value": "<<<__default_gr_paxos_single_leader>>>"
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                }
            ]
        }
    }
}

//@<OUT> WL#13208: TS_FR2_2 verify disableClone is false with options(). {VER(>=8.0.17) && VER(<8.3.0)}
{
    "clusterName": "newName",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "[[*]]",
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
                "value": false
            },
            {
                "option": "replicationAllowedHost",
                "value": "%"
            },
            {
                "option": "memberAuthType",
                "value": "PASSWORD"
            },
            {
                "option": "certIssuer",
                "value": ""
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
                "value": "<<<__default_gr_paxos_single_leader>>>"
?{}
?{VER(<8.0.27)}
?{}
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.23)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.23)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
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
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.23)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.23)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
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
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.23)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.23)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
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

//@<OUT> WL#13208: TS_FR2_1 verify disableClone can be set with setOption() to true. {VER(>=8.0.17)}
Setting the value of 'disableClone' to 'true' in the Cluster ...

Successfully set the value of 'disableClone' to 'true' in the Cluster: 'newName'.


//@<OUT> WL#13208: TS_FR2_2 verify disableClone is true with options(). {VER(>=8.3.0)}
{
    "clusterName": "newName",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "[[*]]",
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
                "value": true
            },
            {
                "option": "replicationAllowedHost",
                "value": "%"
            },
            {
                "option": "memberAuthType",
                "value": "PASSWORD"
            },
            {
                "option": "certIssuer",
                "value": ""
            },
            {
                "option": "communicationStack",
                "value": "XCOM",
                "variable": "group_replication_communication_stack"
            },
            {
                "option": "paxosSingleLeader",
                "value": "<<<__default_gr_paxos_single_leader>>>"
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                }
            ]
        }
    }
}

//@<OUT> WL#13208: TS_FR2_2 verify disableClone is true with options(). {VER(>=8.0.17) && VER(<8.3.0)}
{
    "clusterName": "newName",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "[[*]]",
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
                "value": true
            },
            {
                "option": "replicationAllowedHost",
                "value": "%"
            },
            {
                "option": "memberAuthType",
                "value": "PASSWORD"
            },
            {
                "option": "certIssuer",
                "value": ""
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
                "value": "<<<__default_gr_paxos_single_leader>>>"
?{}
?{VER(<8.0.27)}
?{}
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.23)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.23)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
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
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.23)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.23)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
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
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "2016",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "BEFORE_ON_PRIMARY_FAILOVER",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "3500",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "[[*]]",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.23)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.23)}
                    "value": null,
?{}
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "[[*]]",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "option": "certSubject",
                    "value": ""
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
