//@ WL#12011: Initialization
||

//@<ERR> WL#12011: FR2-04 - invalid value for interactive option.
Dba.createCluster: Option 'interactive' Bool expected, but value is String (TypeError)

//@<OUT> WL#12011: FR2-01 - interactive = true.
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

The MySQL InnoDB cluster is going to be setup in advanced Multi-Primary Mode.
Before continuing you have to confirm that you understand the requirements and
limitations of Multi-Primary Mode. For more information see
https://dev.mysql.com/doc/refman/en/group-replication-limitations.html before
proceeding.

I have read the MySQL InnoDB cluster manual and I understand the requirements
and limitations of advanced Multi-Primary Mode.

Confirm [y/N]:

//@<ERR> WL#12011: FR2-01 - interactive = true.
Dba.createCluster: Cancelled (RuntimeError)

//@<OUT> WL#12011: FR2-03 - no interactive option (default: non-interactive).
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Creating InnoDB cluster 'test' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ WL#12011: Finalization.
||

//@ WL#12049: Initialization
||

//@ WL#12049: Unsupported server version {VER(<5.7.24)}
||Option 'exitStateAction' not supported on target server version: '<<<__version>>>'

//@ WL#12049: Create cluster errors using exitStateAction option {VER(>=5.7.24)}
||Invalid value for exitStateAction, string value cannot be empty.
||Invalid value for exitStateAction, string value cannot be empty.
||Unable to set value ':' for 'exitStateAction': <<<hostname>>>:<<<__mysql_sandbox_port1>>>: Variable 'group_replication_exit_state_action' can't be set to the value of ':'
||Unable to set value 'AB' for 'exitStateAction': <<<hostname>>>:<<<__mysql_sandbox_port1>>>: Variable 'group_replication_exit_state_action' can't be set to the value of 'AB'
||Unable to set value '10' for 'exitStateAction': <<<hostname>>>:<<<__mysql_sandbox_port1>>>: Variable 'group_replication_exit_state_action' can't be set to the value of '10'

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
group_replication_enforce_update_everywhere_checks = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#12049: Dissolve cluster 6 {VER(>=8.0.12)}
||

//@ WL#12049: Initialize new instance
||

//@ WL#12049: Create cluster 2 {VER(>=8.0.12)}
||

//@<OUT> BUG#28701263: DEFAULT VALUE OF EXITSTATEACTION TOO DRASTIC {VER(>=8.0.12)}
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
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
group_replication_enforce_update_everywhere_checks = OFF
group_replication_exit_state_action = READ_ONLY
group_replication_force_members =
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_group_seeds =
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_member_weight = 75
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#11032: Dissolve cluster 6 {VER(>=8.0.11)}
||

//@ WL#11032: Initialize new instance
||

//@ WL#11032: Create cluster 2 {VER(>=8.0.11)}
||

//@<OUT> WL#11032: memberWeight must not be persisted on mysql >= 8.0.11 if not set {VER(>=8.0.12)}
group_replication_exit_state_action = READ_ONLY
group_replication_group_name = ca94447b-e6fc-11e7-b69d-4485005154dc
group_replication_local_address = <<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>
group_replication_recovery_use_ssl = ON
group_replication_single_primary_mode = ON
group_replication_ssl_mode = REQUIRED
group_replication_start_on_boot = ON

//@ WL#11032: Finalization
||

//@ WL#12067: Initialization
||

//@ WL#12067: TSF1_6 Unsupported server version {VER(<8.0.14)}
||Option 'consistency' not supported on target server version: '<<<__version>>>'

//@ WL#12067: Create cluster errors using consistency option {VER(>=8.0.14)}
||Invalid value for consistency, string value cannot be empty.
||Invalid value for consistency, string value cannot be empty.
||Unable to set value ':' for 'consistency': <<<hostname>>>:<<<__mysql_sandbox_port1>>>: Variable 'group_replication_consistency' can't be set to the value of ':'
||Unable to set value 'AB' for 'consistency': <<<hostname>>>:<<<__mysql_sandbox_port1>>>: Variable 'group_replication_consistency' can't be set to the value of 'AB'
||Unable to set value '10' for 'consistency': <<<hostname>>>:<<<__mysql_sandbox_port1>>>: Variable 'group_replication_consistency' can't be set to the value of '10'
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

//@ WL#12050: Create cluster errors using expelTimeout option {VER(>=8.0.13)}
// TSF1_3, TSF1_4, TSF1_6
||Option 'expelTimeout' Integer expected, but value is String (TypeError)
||Option 'expelTimeout' Integer expected, but value is String (TypeError)
||Option 'expelTimeout' is expected to be of type Integer, but is Float (TypeError)
||Option 'expelTimeout' is expected to be of type Integer, but is Bool (TypeError)
||Invalid value for expelTimeout, integer value must be in the range: [0, 3600] (ArgumentError)
||Invalid value for expelTimeout, integer value must be in the range: [0, 3600] (ArgumentError)

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
ERROR: Cannot use host '127.0.1.1' for instance '127.0.1.1:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported by the Group Replication communication layer. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.

//@<ERR> BUG#29246110: check instance error with non supported host.
Dba.checkInstanceConfiguration: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported by Group Replication. (RuntimeError)

//@<OUT> BUG#29246110: createCluster error with non supported host.
This instance reports its own address as 127.0.1.1:<<<__mysql_sandbox_port1>>>
ERROR: Cannot use host '127.0.1.1' for instance '127.0.1.1:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported by the Group Replication communication layer. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.

//@<ERR> BUG#29246110: createCluster error with non supported host.
Dba.createCluster: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported by Group Replication. (RuntimeError)

//@ BUG#29246110: create cluster succeed with supported host.
||

//@<OUT> BUG#29246110: add instance error with non supported host.
This instance reports its own address as 127.0.1.1:<<<__mysql_sandbox_port1>>>
ERROR: Cannot use host '127.0.1.1' for instance '127.0.1.1:<<<__mysql_sandbox_port1>>>' because it resolves to an IP address (127.0.1.1) that does not match a real network interface, thus it is not supported by the Group Replication communication layer. Change your system settings and/or set the MySQL server 'report_host' variable to a hostname that resolves to a supported IP address.

//@<ERR> BUG#29246110: add instance error with non supported host.
Cluster.addInstance: Invalid host/IP '127.0.1.1' resolves to '127.0.1.1' which is not supported by Group Replication. (RuntimeError)

//@ BUG#29246110: finalization
||

//@ WL#12066: Initialization {VER(>=8.0.16)}
||

//@ WL#12066: TSF1_4 Validate that an exception is thrown if the value specified is not an unsigned integer. {VER(>=8.0.16)}
||Dba.createCluster: Unable to set value '-1' for 'autoRejoinTries': <<<hostname>>>:<<<__mysql_sandbox_port1>>>: Variable 'group_replication_autorejoin_tries' can't be set to the value of '-1' (RuntimeError)

//@ WL#12066: TSF1_5 Validate that an exception is thrown if the value  is not in the range 0 to 2016. {VER(>=8.0.16)}
||Dba.createCluster: Unable to set value '2017' for 'autoRejoinTries': <<<hostname>>>:<<<__mysql_sandbox_port1>>>: Variable 'group_replication_autorejoin_tries' can't be set to the value of '2017' (RuntimeError)

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

//@ Create cluster async replication OK
||

//@ BUG#29305551: Finalization
||


//@ BUG#29361352: Initialization.
||

//@<OUT> BUG#29361352: no warning or prompt for multi-primary (interactive: true, multiPrimary: false).
A new InnoDB cluster will be created on instance 'localhost:<<<__mysql_sandbox_port1>>>'.

Validating instance configuration at localhost:<<<__mysql_sandbox_port1>>>...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

Instance configuration is suitable.
NOTE: Group Replication will communicate with other members using '<<<hostname>>>:<<<__mysql_sandbox_gr_port1>>>'. Use the localAddress option to override.

<<<(__version_num<80011)?"WARNING: Instance '"+hostname+":"+__mysql_sandbox_port1+"' cannot persist Group Replication configuration since MySQL version "+__version+" does not support the SET PERSIST command (MySQL version >= 8.0.11 required). Please use the <Dba>.configureLocalInstance() command locally to persist the changes.\n":""\>>>
Creating InnoDB cluster 'test' on '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'...

Adding Seed Instance...
Cluster successfully created. Use Cluster.addInstance() to add MySQL instances.
At least 3 instances are needed for the cluster to be able to withstand up to
one server failure.

//@ BUG#29361352: Finalization.
||

//@ BUG#28064729: Initialization.
||

//@ BUG#28064729: create a cluster.
||

//@ BUG#28064729: add an instance.
||

//@ BUG#28064729: Finalization.
||

//@ WL#12773: FR4 - The ipWhitelist shall not change the behavior defined by FR1
|mysql_innodb_cluster_11111, %|

//@ WL#13208: TS_FR1_2 validate errors for disableClone (only boolean values).
||Dba.createCluster: Option 'disableClone' Bool expected, but value is String (TypeError)
||Dba.createCluster: Option 'disableClone' Bool expected, but value is String (TypeError)
||Dba.createCluster: Option 'disableClone' is expected to be of type Bool, but is Array (TypeError)
||Dba.createCluster: Option 'disableClone' is expected to be of type Bool, but is Map (TypeError)

//@ WL#13208: TS_FR1_3 validate default for disableClone is false.
||

//@<OUT> WL#13208: TS_FR1_3 verify disableClone is false. {VER(>=8.0.13)}
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
                "option": "disableClone",
                "value": <<<(__version_num>=80017)?"false":"true">>>
            }
        ],
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
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
                    "value": "0",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "",
                    "variable": "group_replication_group_seeds"
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
                "option": "disableClone",
                "value": true
            }
        ],
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
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
                }
            ]
        }
    }
}

//@ WL#13208: TS_FR1_1 disableClone option is available (set it to a valid value: true). {VER(>=8.0.17)}
||

//@<OUT> WL#13208: TS_FR1_1 verify disableClone match the value set (true). {VER(>=8.0.17)}
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
                "option": "disableClone",
                "value": true
            }
        ],
        "topology": {
            "<<<hostname>>>:<<<__mysql_sandbox_port1>>>": [
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
                    "value": "0",
                    "variable": "group_replication_member_expel_timeout"
                },
                {
                    "option": "groupSeeds",
                    "value": "",
                    "variable": "group_replication_group_seeds"
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
                }
            ]
        }
    }
}

//@ WL#13208: TS_FR1_1 disableClone option is not supported for server version < 8.0.17. {VER(<8.0.17)}
||Dba.createCluster: Option 'disableClone' not supported on target server version: '<<<__version>>>' (RuntimeError)

//@ canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
|[::1]:<<<__mysql_sandbox_port1>>> = {"mysqlX": "[::1]:<<<__mysql_sandbox_x_port1>>>", "grLocal": "[::1]:<<<__mysql_sandbox_gr_port1>>>", "mysqlClassic": "[::1]:<<<__mysql_sandbox_port1>>>"}|

//@ canonical IPv6 addresses are not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot use host '::1' for instance '[::1]:<<<__mysql_sandbox_port1>>>' because it is an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14. Set the MySQL server 'report_host' variable to an IPv4 address or hostname that resolves an IPv4 address.|
||Dba.createCluster: Unsupported IP address '::1'. IPv6 is only supported by Group Replication on MySQL version >= 8.0.14. (RuntimeError)

//@ IPv6 local_address is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
|ERROR: Cannot create cluster on instance '127.0.0.1:<<<__mysql_sandbox_port1>>>': unsupported localAddress value.|
||Dba.createCluster: Cannot use value '[::1]:<<<__mysql_sandbox_gr_port1>>>' for option localAddress because it has an IPv6 address which is only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is <<<__version>>>. (ArgumentError)

//@ IPv6 on ipWhitelist is not supported below 8.0.14 WL#12758 {VER(< 8.0.14)}
||Dba.createCluster: Invalid value for ipWhitelist '::1': IPv6 not supported (version >= 8.0.14 required for IPv6 support). (ArgumentError)

//@ IPv6 on groupSeeds is not supported below 8.0.14 - 1 WL#12758 {VER(< 8.0.14)}
||Dba.createCluster: Instance does not support the following groupSeed value :'[::1]:<<<__mysql_sandbox_gr_port2>>>'. IPv6 addresses/hostnames are only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is <<<__version>>>. (ArgumentError)

//@ IPv6 on groupSeeds is not supported below 8.0.14 - 2 WL#12758 {VER(< 8.0.14)}
||Dba.createCluster: Instance does not support the following groupSeed values :'[::1]:<<<__mysql_sandbox_gr_port2>>>, [fe80::7e36:f49a:63c8:8ad6]:<<<__mysql_sandbox_gr_port1>>>'. IPv6 addresses/hostnames are only supported by Group Replication from MySQL version >= 8.0.14 and the target instance version is <<<__version>>>. (ArgumentError)
