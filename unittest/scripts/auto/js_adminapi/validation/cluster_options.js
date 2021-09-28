//@ WL#11465: Initialization
||

//@ WL#11465: Create single-primary cluster with specific options
||

//@ WL#11465: Add instance 2 with specific options
||

//@ WL#11465: Add instance 3 with specific options
||

//@<OUT> WL#11465: Get the cluster options {VER(>=8.0.13)}
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
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
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.22)}
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
                    "value": "<<<__cfg_local_address1>>>",
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
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
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
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.22)}
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
                    "value": "<<<__cfg_local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "75",
                    "variable": "group_replication_member_weight"
                },
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds3>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.22)}
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
                    "value": "<<<__cfg_local_address3>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
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

//@<OUT> WL#11465: Get the cluster options {VER(>=5.7.24) && VER(<8.0.0)}
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
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
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
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
                    "value": "<<<__cfg_group_seeds1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address1>>>",
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
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
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
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "75",
                    "variable": "group_replication_member_weight"
                },
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": [
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
                    "value": "<<<__cfg_group_seeds3>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address3>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
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

//@ WL#11465: ArgumentErrors of cluster.options
||Invalid number of arguments, expected 0 to 1 but got 2 (ArgumentError)
||Argument #1 is expected to be a map (TypeError)
||Argument #1 is expected to be a map (TypeError)
||Invalid options: foo (ArgumentError)
||Option 'all' Bool expected, but value is String (TypeError)

//@<OUT> WL#11465: Get the cluster options using 'all' {VER(>=8.0.13)}
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
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
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.22)}
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
                    "value": "<<<__cfg_local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "50",
                    "variable": "group_replication_member_weight"
                },
                {
                    "value": "1",
                    "variable": "auto_increment_increment"
                },
                {
                    "value": "2",
                    "variable": "auto_increment_offset"
                },
?{VER(>=8.0.21)}
                {
                    "value": "[[*]]",
                    "variable": "group_replication_advertise_recovery_endpoints"
                },
?{}
                {
                    "value": "OFF",
                    "variable": "group_replication_allow_local_lower_version_join"
                },
                {
                    "value": "7",
                    "variable": "group_replication_auto_increment_increment"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_bootstrap_group"
                },
?{VER(>=8.0.17)}
                {
                    "value": "[[*]]",
                    "variable": "group_replication_clone_threshold"
                },
?{}
                {
                    "value": "GCS_DEBUG_NONE",
                    "variable": "group_replication_communication_debug_options"
                },
                {
                    "value": "10485760",
                    "variable": "group_replication_communication_max_message_size"
                },
?{VER(>=8.0.27)}
                {
                    "value": "XCOM",
                    "variable": "group_replication_communication_stack"
                },
?{}
                {
?{VER(>=8.0.27)}
                    "value": "300",
?{}
?{VER(<8.0.27)}
                    "value": "31536000",
?{}
                    "variable": "group_replication_components_stop_timeout"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_compression_threshold"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_enforce_update_everywhere_checks"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_applier_threshold"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_certifier_threshold"
                },
                {
                    "value": "10",
                    "variable": "group_replication_flow_control_hold_percent"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_max_quota"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_member_quota_percent"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_min_quota"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_min_recovery_quota"
                },
                {
                    "value": "QUOTA",
                    "variable": "group_replication_flow_control_mode"
                },
                {
                    "value": "1",
                    "variable": "group_replication_flow_control_period"
                },
                {
                    "value": "50",
                    "variable": "group_replication_flow_control_release_percent"
                },
                {
                    "value": "",
                    "variable": "group_replication_force_members"
                },
                {
                    "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
                    "variable": "group_replication_group_name"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_gtid_assignment_block_size"
                },
                {
                    "value": "1073741824",
                    "variable": "group_replication_message_cache_size"
                },
?{VER(>=8.0.27)}
                {
                    "value": "OFF",
                    "variable": "group_replication_paxos_single_leader"
                },
?{}
                {
                    "value": "0",
                    "variable": "group_replication_poll_spin_loops"
                },
                {
                    "value": "TRANSACTIONS_APPLIED",
                    "variable": "group_replication_recovery_complete_at"
                },
?{VER(>=8.0.18)}
                {
                    "value": "uncompressed",
                    "variable": "group_replication_recovery_compression_algorithms"
                },
?{}
                {
                    "value": "OFF",
                    "variable": "group_replication_recovery_get_public_key"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_public_key_path"
                },
                {
                    "value": "60",
                    "variable": "group_replication_recovery_reconnect_interval"
                },
                {
                    "value": "10",
                    "variable": "group_replication_recovery_retry_count"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_ca"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_capath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cert"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cipher"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crl"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crlpath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_key"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_recovery_ssl_verify_server_cert"
                },
?{VER(>=8.0.19)}
                {
                    "value": "",
                    "variable": "group_replication_recovery_tls_ciphersuites"
                },
?{}
?{VER(>=8.0.19) && VER(<8.0.28)}
                {
                    "value": "[[*]]",
                    "variable": "group_replication_recovery_tls_version"
                },
?{}
?{VER(>=8.0.28)}
                {
                    "value": "TLSv1.2,TLSv1.3",
                    "variable": "group_replication_recovery_tls_version"
                },
?{}
                {
                    "value": "ON",
                    "variable": "group_replication_recovery_use_ssl"
                },
?{VER(>=8.0.18)}
                {
                    "value": "3",
                    "variable": "group_replication_recovery_zstd_compression_level"
                },
?{}
                {
                    "value": "ON",
                    "variable": "group_replication_single_primary_mode"
                },
                {
                    "value": "REQUIRED",
                    "variable": "group_replication_ssl_mode"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_start_on_boot"
                },
?{VER(>=8.0.21)}
                {
                    "value": "MYSQL_MAIN",
                    "variable": "group_replication_tls_source"
                },
?{}
                {
                    "value": "150000000",
                    "variable": "group_replication_transaction_size_limit"
                },
                {
                    "value": "0",
                    "variable": "group_replication_unreachable_majority_timeout"
                },
?{VER(>=8.0.26)}
                {
                    "value": "<<<__gr_view_change_uuid>>>",
                    "variable": "group_replication_view_change_uuid"
                },
?{}
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
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
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.22)}
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
                    "value": "<<<__cfg_local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "75",
                    "variable": "group_replication_member_weight"
                },
                {
                    "value": "1",
                    "variable": "auto_increment_increment"
                },
                {
                    "value": "2",
                    "variable": "auto_increment_offset"
                },
?{VER(>=8.0.21)}
                {
                    "value": "[[*]]",
                    "variable": "group_replication_advertise_recovery_endpoints"
                },
?{}
                {
                    "value": "OFF",
                    "variable": "group_replication_allow_local_lower_version_join"
                },
                {
                    "value": "7",
                    "variable": "group_replication_auto_increment_increment"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_bootstrap_group"
                },
?{VER(>=8.0.17)}
                {
                    "value": "[[*]]",
                    "variable": "group_replication_clone_threshold"
                },
?{}
                {
                    "value": "GCS_DEBUG_NONE",
                    "variable": "group_replication_communication_debug_options"
                },
                {
                    "value": "10485760",
                    "variable": "group_replication_communication_max_message_size"
                },
?{VER(>=8.0.27)}
                {
                    "value": "XCOM",
                    "variable": "group_replication_communication_stack"
                },
?{}
                {
?{VER(>=8.0.27)}
                    "value": "300",
?{}
?{VER(<8.0.27)}
                    "value": "31536000",
?{}
                    "variable": "group_replication_components_stop_timeout"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_compression_threshold"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_enforce_update_everywhere_checks"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_applier_threshold"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_certifier_threshold"
                },
                {
                    "value": "10",
                    "variable": "group_replication_flow_control_hold_percent"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_max_quota"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_member_quota_percent"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_min_quota"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_min_recovery_quota"
                },
                {
                    "value": "QUOTA",
                    "variable": "group_replication_flow_control_mode"
                },
                {
                    "value": "1",
                    "variable": "group_replication_flow_control_period"
                },
                {
                    "value": "50",
                    "variable": "group_replication_flow_control_release_percent"
                },
                {
                    "value": "",
                    "variable": "group_replication_force_members"
                },
                {
                    "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
                    "variable": "group_replication_group_name"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_gtid_assignment_block_size"
                },
                {
                    "value": "1073741824",
                    "variable": "group_replication_message_cache_size"
                },
?{VER(>=8.0.27)}
                {
                    "value": "OFF",
                    "variable": "group_replication_paxos_single_leader"
                },
?{}
                {
                    "value": "0",
                    "variable": "group_replication_poll_spin_loops"
                },
                {
                    "value": "TRANSACTIONS_APPLIED",
                    "variable": "group_replication_recovery_complete_at"
                },
?{VER(>=8.0.18)}
                {
                    "value": "uncompressed",
                    "variable": "group_replication_recovery_compression_algorithms"
                },
?{}
                {
                    "value": "OFF",
                    "variable": "group_replication_recovery_get_public_key"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_public_key_path"
                },
                {
                    "value": "60",
                    "variable": "group_replication_recovery_reconnect_interval"
                },
                {
                    "value": "10",
                    "variable": "group_replication_recovery_retry_count"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_ca"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_capath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cert"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cipher"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crl"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crlpath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_key"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_recovery_ssl_verify_server_cert"
                },
?{VER(>=8.0.19)}
                {
                    "value": "",
                    "variable": "group_replication_recovery_tls_ciphersuites"
                },
?{}
?{VER(>=8.0.19) && VER(<8.0.28)}
                {
                    "value": "[[*]]",
                    "variable": "group_replication_recovery_tls_version"
                },
?{}
?{VER(>=8.0.28)}
                {
                    "value": "TLSv1.2,TLSv1.3",
                    "variable": "group_replication_recovery_tls_version"
                },
?{}
                {
                    "value": "ON",
                    "variable": "group_replication_recovery_use_ssl"
                },
?{VER(>=8.0.18)}
                {
                    "value": "3",
                    "variable": "group_replication_recovery_zstd_compression_level"
                },
?{}
                {
                    "value": "ON",
                    "variable": "group_replication_single_primary_mode"
                },
                {
                    "value": "REQUIRED",
                    "variable": "group_replication_ssl_mode"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_start_on_boot"
                },
?{VER(>=8.0.21)}
                {
                    "value": "MYSQL_MAIN",
                    "variable": "group_replication_tls_source"
                },
?{}
                {
                    "value": "150000000",
                    "variable": "group_replication_transaction_size_limit"
                },
                {
                    "value": "0",
                    "variable": "group_replication_unreachable_majority_timeout"
                },
?{VER(>=8.0.26)}
                {
                    "value": "<<<__gr_view_change_uuid>>>",
                    "variable": "group_replication_view_change_uuid"
                },
?{}
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds3>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.22)}
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
                    "value": "<<<__cfg_local_address3>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "value": "1",
                    "variable": "auto_increment_increment"
                },
                {
                    "value": "2",
                    "variable": "auto_increment_offset"
                },
?{VER(>=8.0.21)}
                {
                    "value": "[[*]]",
                    "variable": "group_replication_advertise_recovery_endpoints"
                },
?{}
                {
                    "value": "OFF",
                    "variable": "group_replication_allow_local_lower_version_join"
                },
                {
                    "value": "7",
                    "variable": "group_replication_auto_increment_increment"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_bootstrap_group"
                },
?{VER(>=8.0.17)}
                {
                    "value": "[[*]]",
                    "variable": "group_replication_clone_threshold"
                },
?{}
                {
                    "value": "GCS_DEBUG_NONE",
                    "variable": "group_replication_communication_debug_options"
                },
                {
                    "value": "10485760",
                    "variable": "group_replication_communication_max_message_size"
                },
?{VER(>=8.0.27)}
                {
                    "value": "XCOM",
                    "variable": "group_replication_communication_stack"
                },
?{}
                {
?{VER(>=8.0.27)}
                    "value": "300",
?{}
?{VER(<8.0.27)}
                    "value": "31536000",
?{}
                    "variable": "group_replication_components_stop_timeout"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_compression_threshold"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_enforce_update_everywhere_checks"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_applier_threshold"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_certifier_threshold"
                },
                {
                    "value": "10",
                    "variable": "group_replication_flow_control_hold_percent"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_max_quota"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_member_quota_percent"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_min_quota"
                },
                {
                    "value": "0",
                    "variable": "group_replication_flow_control_min_recovery_quota"
                },
                {
                    "value": "QUOTA",
                    "variable": "group_replication_flow_control_mode"
                },
                {
                    "value": "1",
                    "variable": "group_replication_flow_control_period"
                },
                {
                    "value": "50",
                    "variable": "group_replication_flow_control_release_percent"
                },
                {
                    "value": "",
                    "variable": "group_replication_force_members"
                },
                {
                    "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
                    "variable": "group_replication_group_name"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_gtid_assignment_block_size"
                },
                {
                    "value": "1073741824",
                    "variable": "group_replication_message_cache_size"
                },
?{VER(>=8.0.27)}
                {
                    "value": "OFF",
                    "variable": "group_replication_paxos_single_leader"
                },
?{}
                {
                    "value": "0",
                    "variable": "group_replication_poll_spin_loops"
                },
                {
                    "value": "TRANSACTIONS_APPLIED",
                    "variable": "group_replication_recovery_complete_at"
                },
?{VER(>=8.0.18)}
                {
                    "value": "uncompressed",
                    "variable": "group_replication_recovery_compression_algorithms"
                },
?{}
                {
                    "value": "OFF",
                    "variable": "group_replication_recovery_get_public_key"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_public_key_path"
                },
                {
                    "value": "60",
                    "variable": "group_replication_recovery_reconnect_interval"
                },
                {
                    "value": "10",
                    "variable": "group_replication_recovery_retry_count"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_ca"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_capath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cert"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cipher"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crl"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crlpath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_key"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_recovery_ssl_verify_server_cert"
                },
?{VER(>=8.0.19)}
                {
                    "value": "",
                    "variable": "group_replication_recovery_tls_ciphersuites"
                },
?{}
?{VER(>=8.0.19) && VER(<8.0.28)}
                {
                    "value": "[[*]]",
                    "variable": "group_replication_recovery_tls_version"
                },
?{}
?{VER(>=8.0.28)}
                {
                    "value": "TLSv1.2,TLSv1.3",
                    "variable": "group_replication_recovery_tls_version"
                },
?{}
                {
                    "value": "ON",
                    "variable": "group_replication_recovery_use_ssl"
                },
?{VER(>=8.0.18)}
                {
                    "value": "3",
                    "variable": "group_replication_recovery_zstd_compression_level"
                },
?{}
                {
                    "value": "ON",
                    "variable": "group_replication_single_primary_mode"
                },
                {
                    "value": "REQUIRED",
                    "variable": "group_replication_ssl_mode"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_start_on_boot"
                },
?{VER(>=8.0.21)}
                {
                    "value": "MYSQL_MAIN",
                    "variable": "group_replication_tls_source"
                },
?{}
                {
                    "value": "150000000",
                    "variable": "group_replication_transaction_size_limit"
                },
                {
                    "value": "0",
                    "variable": "group_replication_unreachable_majority_timeout"
                },
?{VER(>=8.0.26)}
                {
                    "value": "<<<__gr_view_change_uuid>>>",
                    "variable": "group_replication_view_change_uuid"
                },
?{}
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
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

//@<OUT> WL#11465: Get the cluster options using 'all' {VER(>=5.7.24) && VER(<8.0.0)}
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
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
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
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
                    "value": "<<<__cfg_group_seeds1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "50",
                    "variable": "group_replication_member_weight"
                },
                {
                    "value": "1",
                    "variable": "auto_increment_increment"
                },
                {
                    "value": "2",
                    "variable": "auto_increment_offset"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_allow_local_disjoint_gtids_join"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_allow_local_lower_version_join"
                },
                {
                    "value": "7",
                    "variable": "group_replication_auto_increment_increment"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_bootstrap_group"
                },
                {
?{VER(>=8.0.27)}
                    "value": "300",
?{}
?{VER(<8.0.27)}
                    "value": "31536000",
?{}
                    "variable": "group_replication_components_stop_timeout"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_compression_threshold"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_enforce_update_everywhere_checks"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_applier_threshold"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_certifier_threshold"
                },
                {
                    "value": "QUOTA",
                    "variable": "group_replication_flow_control_mode"
                },
                {
                    "value": "",
                    "variable": "group_replication_force_members"
                },
                {
                    "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
                    "variable": "group_replication_group_name"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_gtid_assignment_block_size"
                },
                {
                    "value": "0",
                    "variable": "group_replication_poll_spin_loops"
                },
                {
                    "value": "TRANSACTIONS_APPLIED",
                    "variable": "group_replication_recovery_complete_at"
                },
                {
                    "value": "60",
                    "variable": "group_replication_recovery_reconnect_interval"
                },
                {
                    "value": "10",
                    "variable": "group_replication_recovery_retry_count"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_ca"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_capath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cert"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cipher"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crl"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crlpath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_key"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_recovery_ssl_verify_server_cert"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_recovery_use_ssl"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_single_primary_mode"
                },
                {
                    "value": "REQUIRED",
                    "variable": "group_replication_ssl_mode"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_start_on_boot"
                },
                {
                    "value": "0",
                    "variable": "group_replication_transaction_size_limit"
                },
                {
                    "value": "0",
                    "variable": "group_replication_unreachable_majority_timeout"
                },
?{VER(>=8.0.26)}
                {
                    "value": "<<<__gr_view_change_uuid>>>",
                    "variable": "group_replication_view_change_uuid"
                },
?{}
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
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
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": null,
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "75",
                    "variable": "group_replication_member_weight"
                },
                {
                    "value": "1",
                    "variable": "auto_increment_increment"
                },
                {
                    "value": "2",
                    "variable": "auto_increment_offset"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_allow_local_disjoint_gtids_join"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_allow_local_lower_version_join"
                },
                {
                    "value": "7",
                    "variable": "group_replication_auto_increment_increment"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_bootstrap_group"
                },
                {
?{VER(>=8.0.27)}
                    "value": "300",
?{}
?{VER(<8.0.27)}
                    "value": "31536000",
?{}
                    "variable": "group_replication_components_stop_timeout"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_compression_threshold"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_enforce_update_everywhere_checks"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_applier_threshold"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_certifier_threshold"
                },
                {
                    "value": "QUOTA",
                    "variable": "group_replication_flow_control_mode"
                },
                {
                    "value": "",
                    "variable": "group_replication_force_members"
                },
                {
                    "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
                    "variable": "group_replication_group_name"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_gtid_assignment_block_size"
                },
                {
                    "value": "0",
                    "variable": "group_replication_poll_spin_loops"
                },
                {
                    "value": "TRANSACTIONS_APPLIED",
                    "variable": "group_replication_recovery_complete_at"
                },
                {
                    "value": "60",
                    "variable": "group_replication_recovery_reconnect_interval"
                },
                {
                    "value": "10",
                    "variable": "group_replication_recovery_retry_count"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_ca"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_capath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cert"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cipher"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crl"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crlpath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_key"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_recovery_ssl_verify_server_cert"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_recovery_use_ssl"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_single_primary_mode"
                },
                {
                    "value": "REQUIRED",
                    "variable": "group_replication_ssl_mode"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_start_on_boot"
                },
                {
                    "value": "0",
                    "variable": "group_replication_transaction_size_limit"
                },
                {
                    "value": "0",
                    "variable": "group_replication_unreachable_majority_timeout"
                },
?{VER(>=8.0.26)}
                {
                    "value": "<<<__gr_view_change_uuid>>>",
                    "variable": "group_replication_view_change_uuid"
                },
?{}
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": [
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
                    "value": "<<<__cfg_group_seeds3>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address3>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "value": "1",
                    "variable": "auto_increment_increment"
                },
                {
                    "value": "2",
                    "variable": "auto_increment_offset"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_allow_local_disjoint_gtids_join"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_allow_local_lower_version_join"
                },
                {
                    "value": "7",
                    "variable": "group_replication_auto_increment_increment"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_bootstrap_group"
                },
                {
?{VER(>=8.0.27)}
                    "value": "300",
?{}
?{VER(<8.0.27)}
                    "value": "31536000",
?{}
                    "variable": "group_replication_components_stop_timeout"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_compression_threshold"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_enforce_update_everywhere_checks"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_applier_threshold"
                },
                {
                    "value": "25000",
                    "variable": "group_replication_flow_control_certifier_threshold"
                },
                {
                    "value": "QUOTA",
                    "variable": "group_replication_flow_control_mode"
                },
                {
                    "value": "",
                    "variable": "group_replication_force_members"
                },
                {
                    "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
                    "variable": "group_replication_group_name"
                },
                {
                    "value": "1000000",
                    "variable": "group_replication_gtid_assignment_block_size"
                },
                {
                    "value": "0",
                    "variable": "group_replication_poll_spin_loops"
                },
                {
                    "value": "TRANSACTIONS_APPLIED",
                    "variable": "group_replication_recovery_complete_at"
                },
                {
                    "value": "60",
                    "variable": "group_replication_recovery_reconnect_interval"
                },
                {
                    "value": "10",
                    "variable": "group_replication_recovery_retry_count"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_ca"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_capath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cert"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_cipher"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crl"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_crlpath"
                },
                {
                    "value": "",
                    "variable": "group_replication_recovery_ssl_key"
                },
                {
                    "value": "OFF",
                    "variable": "group_replication_recovery_ssl_verify_server_cert"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_recovery_use_ssl"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_single_primary_mode"
                },
                {
                    "value": "REQUIRED",
                    "variable": "group_replication_ssl_mode"
                },
                {
                    "value": "ON",
                    "variable": "group_replication_start_on_boot"
                },
                {
                    "value": "0",
                    "variable": "group_replication_transaction_size_limit"
                },
                {
                    "value": "0",
                    "variable": "group_replication_unreachable_majority_timeout"
                },
?{VER(>=8.0.26)}
                {
                    "value": "<<<__gr_view_change_uuid>>>",
                    "variable": "group_replication_view_change_uuid"
                },
?{}
?{VER(<8.0.23)}
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
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
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

//@ Change the value of applierWorkerThreads of a member of the Cluster {VER(>=8.0.23)}
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' belongs to an InnoDB Cluster.|
|Configuring local MySQL instance listening at port <<<__mysql_sandbox_port2>>> for use in an InnoDB cluster...|
|This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port2>>>|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is valid to be used in an InnoDB cluster.|
|The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' is already ready to be used in an InnoDB cluster.|
|WARNING: The changes on the value of <<<__replica_keyword>>>_parallel_workers will only take place after the instance leaves and rejoins the Cluster.|
|Successfully set the value of <<<__replica_keyword>>>_parallel_workers.|

//@<OUT> Check the output of options after changing applierWorkerThreads {VER(>=8.0.23)}
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
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
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address1>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "50",
                    "variable": "group_replication_member_weight"
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
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
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "ABORT_SERVER",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds2>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address2>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "75",
                    "variable": "group_replication_member_weight"
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "10",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
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
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds3>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address3>>>",
                    "variable": "group_replication_local_address"
                },
                {
                    "option": "memberWeight",
                    "value": "25",
                    "variable": "group_replication_member_weight"
                },
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
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


//@ Kill instances 2 and 3
||

//@<OUT> WL#11465: Get the cluster options with 2 members down {VER(>=5.7.24) && VER(<8.0.0)}
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
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
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
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
                    "value": "<<<__cfg_group_seeds1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
                {
                    "option": "localAddress",
                    "value": "<<<__cfg_local_address1>>>",
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
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "shellConnectError": "MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "shellConnectError": "MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port3)>>>' ([[*]])"
            }
        }
    }
}

//@<OUT> WL#11465: Get the cluster options with 2 members down {VER(>=8.0.13)}
{
    "clusterName": "testCluster",
    "defaultReplicaSet": {
        "globalOptions": [
            {
                "option": "groupName",
                "value": "bbbbbbbb-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
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
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
                {
                    "option": "autoRejoinTries",
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
                {
                    "option": "consistency",
                    "value": "EVENTUAL",
                    "variable": "group_replication_consistency"
                },
                {
                    "option": "exitStateAction",
                    "value": "READ_ONLY",
                    "variable": "group_replication_exit_state_action"
                },
                {
                    "option": "expelTimeout",
                    "value": "<<<__default_gr_expel_timeout>>>",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "<<<__cfg_group_seeds1>>>",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
?{VER(>=8.0.22)}
                    "value": "AUTOMATIC",
?{}
?{VER(<8.0.22)}
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
                    "value": "<<<__cfg_local_address1>>>",
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
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "0",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "OFF",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
?{VER(>=8.0.23)}
                {
                    "value": "WRITESET",
                    "variable": "binlog_transaction_dependency_tracking"
                },
                {
                    "value": "LOGICAL_CLOCK",
                    "variable": "<<<__replica_keyword>>>_parallel_type"
                },
                {
                    "value": "4",
                    "variable": "<<<__replica_keyword>>>_parallel_workers"
                },
                {
                    "value": "ON",
                    "variable": "<<<__replica_keyword>>>_preserve_commit_order"
                },
?{}
                {
                    "value": "XXHASH64",
                    "variable": "transaction_write_set_extraction"
                }
            ],
            "<<<hostname>>>:<<<__mysql_sandbox_port2>>>": {
                "shellConnectError": "MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port2)>>>' ([[*]])"
            },
            "<<<hostname>>>:<<<__mysql_sandbox_port3>>>": {
                "shellConnectError": "MySQL Error 2003 (HY000): Can't connect to MySQL server on '<<<libmysql_host_description(hostname, __mysql_sandbox_port3)>>>' ([[*]])"
            }
        }
    }
}

//@ WL#11465: Finalization
||
