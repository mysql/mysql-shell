
//@ WL#12049: Unsupported server version {VER(<5.7.24)}
||Option 'exitStateAction' not supported on target server version: '<<<__version>>>'

//@ WL#12049: Create cluster errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Unable to set value ':' for 'exitStateAction': Variable 'group_replication_exit_state_action' can't be set to the value of ':'
||Unable to set value 'AB' for 'exitStateAction': Variable 'group_replication_exit_state_action' can't be set to the value of 'AB'
||Unable to set value '10' for 'exitStateAction': Variable 'group_replication_exit_state_action' can't be set to the value of '10'

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (ABORT_SERVER) {VER(>=5.7.24)}
||

//@ WL#12049: Dissolve cluster 1 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (READ_ONLY) {VER(>=5.7.24)}
||

//@ WL#12049: Dissolve cluster 2 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (1) {VER(>=5.7.24)}
||

//@ WL#12049: Dissolve cluster 3 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster specifying a valid value for exitStateAction (0) {VER(>=5.7.24)}
||

//@ WL#12049: Dissolve cluster 4 {VER(>=5.7.24)}
||

//@ WL#12049: Create cluster {VER(>=8.0.12)}
||

//@<OUT> WL#12049: exitStateAction must be persisted on mysql >= 8.0.12 {VER(>=8.0.12)}
group_replication_bootstrap_group = OFF
?{VER(>=8.0.27)}
group_replication_communication_stack = MYSQL
?{}
group_replication_enforce_update_everywhere_checks = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
?{VER(>=8.0.27)}
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_port1>>>
?{}
?{VER(>=8.0.31)}
group_replication_paxos_single_leader = <<<__default_gr_paxos_single_leader>>>
?{}
?{VER(<8.0.27)}
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
?{}
group_replication_recovery_use_ssl = ON
group_replication_ssl_mode = REQUIRED

//@ WL#12049: Dissolve cluster 6 {VER(>=8.0.12)}
||

//@ WL#12049: Initialize new instance
||

//@ WL#12049: Create cluster 2 {VER(>=8.0.12)}
||

//@<OUT> BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
?{VER(>=8.0.27)}
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_port1>>>
?{}
?{VER(<8.0.27)}
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
?{}
?{VER(>=8.0.31)}
group_replication_paxos_single_leader = <<<__default_gr_paxos_single_leader>>>
?{}
group_replication_recovery_ssl_verify_server_cert = OFF
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON


//@ WL#12049: Finalization
||

//@ WL#11032: Initialization
||

//@ WL#11032: Unsupported server version {VER(<5.7.20)}
||Option 'memberWeight' not available for target server version.

//@ WL#11032: Create cluster errors using memberWeight option {VER(>=5.7.20)}
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Bool (TypeError)
||Option 'memberWeight' Integer expected, but value is String (TypeError)
||Option 'memberWeight' is expected to be of type Integer, but is Float (TypeError)

//@ WL#11032: Create cluster specifying a valid value for memberWeight (25) {VER(>=5.7.20)}
||

//@ WL#11032: Dissolve cluster 1 {VER(>=5.7.20)}
||

//@ WL#11032: Create cluster specifying a valid value for memberWeight (100) {VER(>=5.7.20)}
||

//@ WL#11032: Dissolve cluster 2 {VER(>=5.7.20)}
||

//@ WL#11032: Create cluster specifying a valid value for memberWeight (-50) {VER(>=5.7.20)}
||

//@ WL#11032: Dissolve cluster 3 {VER(>=5.7.20)}
||

//@ WL#11032: Create cluster {VER(>=8.0.11)}
||

//@<OUT> WL#11032: memberWeight must be persisted on mysql >= 8.0.11 {VER(>=8.0.12)}
group_replication_bootstrap_group = OFF
?{VER(>=8.0.27)}
group_replication_communication_stack = MYSQL
?{}
group_replication_enforce_update_everywhere_checks = OFF
?{VER(<8.0.16)}
group_replication_exit_state_action = READ_ONLY
?{}
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
?{VER(>=8.0.27)}
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_port1>>>
?{}
?{VER(<8.0.27)}
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
?{}
group_replication_member_weight = 75
?{VER(>=8.0.31)}
group_replication_paxos_single_leader = <<<__default_gr_paxos_single_leader>>>
?{}
group_replication_recovery_use_ssl = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#11032: Dissolve cluster 6 {VER(>=8.0.11)}
||

//@ WL#11032: Initialize new instance
||

//@ WL#11032: Create cluster 2 {VER(>=8.0.11)}
||

//@<OUT> WL#11032: memberWeight must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.12)}
?{VER(<8.0.16)}
group_replication_exit_state_action = READ_ONLY
?{}
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
?{VER(>=8.0.27)}
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_port1>>>
?{}
?{VER(<8.0.27)}
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
?{}
?{VER(>=8.0.31)}
group_replication_paxos_single_leader = <<<__default_gr_paxos_single_leader>>>
?{}
group_replication_recovery_use_ssl = ON
group_replication_ssl_mode = REQUIRED

//@ WL#11032: Finalization
||

//@ WL#12067: Initialization
||

//@ WL#12067: TSF1_6 Unsupported server version {VER(<8.0.14)}
||Option 'consistency' not supported on target server version: '<<<__version>>>'

//@ WL#12067: Create cluster errors using consistency option {VER(>=8.0.14)}
||Invalid value for 'consistency', string value cannot be empty.
||Invalid value for 'consistency', string value cannot be empty.
||Unable to set value ':' for 'consistency': Variable 'group_replication_consistency' can't be set to the value of ':'
||Unable to set value 'AB' for 'consistency': Variable 'group_replication_consistency' can't be set to the value of 'AB'
||Unable to set value '10' for 'consistency': Variable 'group_replication_consistency' can't be set to the value of '10'
||Option 'consistency' is expected to be of type String, but is Integer (TypeError)
||Cannot use the failoverConsistency and consistency options simultaneously. The failoverConsistency option is deprecated, please use the consistency option instead. (ArgumentError)


//@ WL#12067: TSF1_1 Create cluster using BEFORE_ON_PRIMARY_FAILOVER as value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 1 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_2 Create cluster using EVENTUAL as value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 2 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_1 Create cluster using 1 as value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 3 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_2 Create cluster using 0 as value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 4 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_3 Create cluster using no value for consistency {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 5 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_7 Create cluster using evenTual as value for consistency throws no exception (case insensitive) {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 6 {VER(>=8.0.14)}
||

//@ WL#12067: TSF1_8 Create cluster using Before_ON_PriMary_FailoveR as value for consistency throws no exception (case insensitive) {VER(>=8.0.14)}
||

//@ WL#12067: Dissolve cluster 7 {VER(>=8.0.14)}
||

//@ WL#12067: Initialize new instance {VER(>=8.0.14)}
||

//@ WL#12067: Create cluster 2 {VER(>=8.0.14)}
||

//@ WL#12067: Finalization
||

//@ WL#12050: Initialization
||

//@ WL#12050: TSF1_5 Unsupported server version {VER(<8.0.13)}
||Option 'expelTimeout' not supported on target server version: '<<<__version>>>'

//@ WL#12050: TSF1_1 Create cluster using 12 as value for expelTimeout {VER(>=8.0.13)}
||

//@ WL#12050: Dissolve cluster 1 {VER(>=8.0.13)}
||

//@ WL#12050: TSF1_2 Initialize new instance {VER(>=8.0.13)}
||

//@ WL#12050: TSF1_2 Create cluster using no value for expelTimeout, confirm it has the default value {VER(>=8.0.13)}
||

//@ WL#12050: Finalization
||

//@ BUG#25867733: Deploy instances (setting performance_schema value).
||

//@ BUG#25867733: createCluster error with performance_schema=off
|ERROR: Instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' has the performance_schema disabled (performance_schema=OFF). Instances must have the performance_schema enabled to for InnoDB Cluster usage.|performance_schema disabled on target instance. (RuntimeError)

//@ BUG#25867733: createCluster no error with performance_schema=on
||

//@ BUG#25867733: cluster.status() successful
||

//@ BUG#25867733: finalization
||

//@ BUG#29246110: Deploy instances, setting non supported host: 127.0.1.1.
||

//@<OUT> BUG#29246110: check instance error with non supported host.
This instance reports its own address as 127.0.1.1:<<<__mysql_sandbox_port1>>>
ERROR: Cannot use host '127.0.1.1' for instance '127.0.1.1:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.

//@<ERR> BUG#29246110: check instance error with non supported host.
Dba.checkInstanceConfiguration: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported. (RuntimeError)

//@<OUT> BUG#29246110: createCluster error with non supported host.
This instance reports its own address as 127.0.1.1:<<<__mysql_sandbox_port1>>>
ERROR: Cannot use host '127.0.1.1' for instance '127.0.1.1:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.

//@<ERR> BUG#29246110: createCluster error with non supported host.
Dba.createCluster: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported. (RuntimeError)

//@ BUG#29246110: create cluster succeed with supported host.
||

//@<OUT> BUG#29246110: add instance error with non supported host.
This instance reports its own address as 127.0.1.1:<<<__mysql_sandbox_port1>>>
ERROR: Cannot use host '127.0.1.1' for instance '127.0.1.1:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.

//@<ERR> BUG#29246110: add instance error with non supported host.
Cluster.addInstance: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported. (RuntimeError)

//@ BUG#29246110: finalization
||

//@ WL#12066: Initialization {VER(>=8.0.16)}
||

//@ WL#12066: TSF1_4 Validate that an exception is thrown if the value specified is not an unsigned integer. {VER(>=8.0.16)}
||Unable to set value '-1' for 'autoRejoinTries': Variable 'group_replication_autorejoin_tries' can't be set to the value of '-1' (RuntimeError)

//@ WL#12066: TSF1_5 Validate that an exception is thrown if the value  is not in the range 0 to 2016. {VER(>=8.0.16)}
||Unable to set value '2017' for 'autoRejoinTries': Variable 'group_replication_autorejoin_tries' can't be set to the value of '2017' (RuntimeError)

//@ WL#12066: TSF1_1 Validate that the functions [dba.]createCluster() and [cluster.]addInstance() support a new option named autoRejoinTries. {VER(>=8.0.16)}
|WARNING: The member will only proceed according to its exitStateAction if auto-rejoin fails (i.e. all retry attempts are exhausted).|

//@ WL#12066: Dissolve cluster {VER(>=8.0.16)}
||

//@ WL#12066: Finalization {VER(>=8.0.16)}
||

//@<OUT> BUG#29308037: Confirm that all replication users where removed
0

//@ BUG#29308037: Finalization
||

//@ BUG#29305551: Initialization
||

//@<OUT> Create cluster async replication (should fail)
ERROR: Cannot create cluster on instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' because it has asynchronous (source-replica) replication channel(s) configured. MySQL InnoDB Cluster does not support manually configured channels as they are not managed using the AdminAPI (e.g. when PRIMARY moves to another member) which may cause cause replication to break or even create split-brain scenarios (data loss). Use the 'force' option to skip this validation on a temporary scenario (e.g. migrating from a replication topology to InnoDB Cluster).

//@<ERR> Create cluster async replication (should fail)
Dba.createCluster: The instance '<<<hostname>>>:<<<__mysql_sandbox_port2>>>' has asynchronous replication configured. (RuntimeError)

//@ WL#12773: FR4 - The ipWhitelist shall not change the behavior defined by FR1
|mysql_innodb_cluster_11111, %|

//@ WL#13208: TS_FR1_2 validate errors for disableClone (only boolean values).
||Option 'disableClone' Bool expected, but value is String (TypeError)
||Option 'disableClone' Bool expected, but value is String (TypeError)
||Option 'disableClone' is expected to be of type Bool, but is Array (TypeError)
||Option 'disableClone' is expected to be of type Bool, but is Map (TypeError)

//@ WL#13208: TS_FR1_3 validate default for disableClone is false.
||

//@<OUT> WL#13208: TS_FR1_3 verify disableClone is false. {VER(>=8.3.0)}
{
    "clusterName": "test",
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
                "value": <<<(__version_num>=80017)?"false":"true">>>
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
                "value": "MYSQL",
                "variable": "group_replication_communication_stack"
            },
            {
                "option": "paxosSingleLeader",
                "value": "<<<__default_gr_paxos_single_leader>>>"
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": []
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
                    "value": "",
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
                    "value": "50",
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

//@<OUT> WL#13208: TS_FR1_3 verify disableClone is false. {VER(>=8.0.13) && VER(<8.3.0)}
{
    "clusterName": "test",
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
                "value": <<<(__version_num>=80017)?"false":"true">>>
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
                "value": "MYSQL",
                "variable": "group_replication_communication_stack"
?{}
?{VER(>=8.0.31)}
            },
            {
                "option": "paxosSingleLeader",
                "value": "<<<__default_gr_paxos_single_leader>>>"
?{}
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": []
        },
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
?{VER(>=8.0.16)}
                {
                    "option": "autoRejoinTries",
                    "value": "<<<__default_gr_auto_rejoin_tries>>>",
                    "variable": "group_replication_autorejoin_tries"
                },
?{}
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
                    "value": "",
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
                    "value": "50",
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

//@<OUT> WL#13208: TS_FR1_3 verify disableClone is false. {VER(>=5.7.24) && VER(<8.0.0)}
{
    "clusterName": "test",
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
                "value": "MYSQL",
                "variable": "group_replication_communication_stack"
?{}
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": []
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
                    "value": "",
                    "variable": "group_replication_group_seeds"
                },
                {
                    "option": "ipAllowlist",
                    "value": null,
                    "variable": "group_replication_ip_allowlist"
                },
?{VER(<8.3.0)}
                {
                    "option": "ipWhitelist",
                    "value": "AUTOMATIC",
                    "variable": "group_replication_ip_whitelist"
                },
?{}
                {
                    "option": "localAddress",
                    "value": "[[*]]",
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

//@ WL#13208: TS_FR1_1 disableClone option is available (set it to a valid value: true). {VER(>=8.0.17)}
||

//@<OUT> WL#13208: TS_FR1_1 verify disableClone match the value set (true). {VER(>=8.3.0)}
{
    "clusterName": "test",
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
                "value": "MYSQL",
                "variable": "group_replication_communication_stack"
            },
            {
                "option": "paxosSingleLeader",
                "value": "<<<__default_gr_paxos_single_leader>>>"
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": []
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
                    "value": "",
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
                    "value": "50",
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

//@<OUT> WL#13208: TS_FR1_1 verify disableClone match the value set (true). {VER(>=8.0.17) && VER(<8.3.0)}
{
    "clusterName": "test",
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
                "value": "MYSQL",
                "variable": "group_replication_communication_stack"
?{}
?{VER(>=8.0.31)}
            },
            {
                "option": "paxosSingleLeader",
                "value": "<<<__default_gr_paxos_single_leader>>>"
?{}
            }
        ],
        "tags": {
            ".global": [],
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": []
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
                    "value": "",
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
                    "value": "50",
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

//@ WL#13208: TS_FR1_1 disableClone option is not supported for server version < 8.0.17. {VER(<8.0.17)}
||Option 'disableClone' not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
|[::1]:<<<__mysql_sandbox_port1>>> = {"mysqlX": "[::1]:<<<__mysql_sandbox_x_port1>>>", "grLocal": "[::1]:<<<__mysql_sandbox_gr_port1>>>", "mysqlClassic": "[::1]:<<<__mysql_sandbox_port1>>>"}|

//If the target instance is >= 8.0.23, when ipWhitelist is used a deprecation warning must be printed
//@ IPv6 addresses are supported on localAddress, groupSeeds and ipWhitelist WL#12758 {VER(>=8.0.23)}
|WARNING: The ipWhitelist option is deprecated in favor of ipAllowlist. ipAllowlist will be set instead.|

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port1>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)

//@ IPv6 local_address is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot create cluster on instance '127.0.0.1:<<<__mysql_sandbox_port1>>>': unsupported localAddress value.|
||Cannot use value '[::1]:<<<__mysql_sandbox_gr_port1>>>' for option localAddress because it has an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is <<<__version>>>. (ArgumentError)

//@ IPv6 on ipWhitelist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
||Invalid value for ipWhitelist '::1': IPv6 not supported (version >= 8.0.14 required for IPv6 support). (ArgumentError)
